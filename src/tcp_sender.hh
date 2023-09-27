#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class TCPSender
{
  Wrap32 isn_;
  Wrap32 zero_isn_;
  uint64_t abs_isn_;

  uint64_t initial_RTO_ms_;
  uint64_t now_RTO_ms_;

  uint64_t time_passed_now_ {};

  bool need_retrans_ {};

  std::deque<TCPSenderMessage> tcpmsgs_ {};
  std::deque<TCPSenderMessage> tcp_keep_msgs_ {};

  uint64_t cached_size_ {};
  uint64_t window_size_ { 1 };

  uint32_t retrans_times_ {};

  bool started_ { false };
  bool closed_ { false };

  uint64_t Seq( Wrap32 const& wrap32 ) const noexcept;
  uint64_t EndSeq( TCPSenderMessage const& msg ) const noexcept;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
