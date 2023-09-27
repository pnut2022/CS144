#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

#include <iostream>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , zero_isn_ { isn_ }
  , abs_isn_ { isn_.unwrap( zero_isn_, 0 ) }
  , initial_RTO_ms_( initial_RTO_ms )
  , now_RTO_ms_ { initial_RTO_ms }
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return cached_size_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retrans_times_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  if ( need_retrans_ ) {
    if ( time_passed_now_ < now_RTO_ms_ ) {
      return {};
    }

    if ( window_size_ != 0 ) {
      now_RTO_ms_ *= 2;
    }

    time_passed_now_ = 0;

    return !tcp_keep_msgs_.empty() ? tcp_keep_msgs_.front() : optional<TCPSenderMessage> {};
  }

  if ( tcpmsgs_.empty() ) {
    return {};
  }

  auto msg = tcpmsgs_.front();
  tcpmsgs_.pop_front();
  tcp_keep_msgs_.push_back( msg );

  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  auto act_win_size = window_size_;
  if ( act_win_size == 0 ) {
    act_win_size = 1;
  }

  while ( cached_size_ < act_win_size && !closed_ ) {
    auto buf = outbound_stream.peek();
    TCPSenderMessage tcpmsg;
    tcpmsg.seqno = isn_;

    if ( !started_ ) {
      tcpmsg.SYN = true;
      started_ = true;
    }

    uint64_t const payload_len
      = std::min( buf.length(), std::min( TCPConfig::MAX_PAYLOAD_SIZE, act_win_size - cached_size_ - tcpmsg.SYN ) );

    tcpmsg.payload = std::string( buf.begin(), payload_len );

    outbound_stream.pop( payload_len );

    uint64_t act_len = payload_len + tcpmsg.SYN;

    tcpmsg.FIN = outbound_stream.is_finished() && ( act_len < act_win_size - cached_size_ );

    act_len += tcpmsg.FIN;

    if ( act_len == 0 ) {
      return;
    }

    closed_ = tcpmsg.FIN;
    tcpmsgs_.push_back( std::move( tcpmsg ) );
    cached_size_ += act_len;
    isn_ = isn_ + act_len;
    abs_isn_ += act_len;
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  return { isn_, {}, {}, {} };
}

uint64_t TCPSender::Seq( Wrap32 const& wrap32 ) const noexcept
{
  return wrap32.unwrap( zero_isn_, abs_isn_ );
}

uint64_t TCPSender::EndSeq( TCPSenderMessage const& msg ) const noexcept
{
  return Seq( msg.seqno ) + msg.sequence_length();
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size_ = msg.window_size;

  if ( !msg.ackno.has_value() ) {
    return;
  }

  if ( tcp_keep_msgs_.empty() || Seq( msg.ackno.value() ) > EndSeq( tcp_keep_msgs_.back() ) ) {
    return;
  }

  while ( !tcp_keep_msgs_.empty() && ( Seq( msg.ackno.value() ) >= EndSeq( tcp_keep_msgs_.front() ) ) ) {
    cached_size_ -= tcp_keep_msgs_.front().sequence_length();
    tcp_keep_msgs_.pop_front();
    retrans_times_ = 0;
    time_passed_now_ = 0;
    now_RTO_ms_ = initial_RTO_ms_;
    need_retrans_ = false;
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  time_passed_now_ += ms_since_last_tick;
  if ( ( time_passed_now_ >= now_RTO_ms_ ) ) {
    need_retrans_ = true;
    retrans_times_++;
  }
}