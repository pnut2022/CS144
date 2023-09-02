#include <iostream>
#include <limits>

#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if ( !zero_point_ && !message.SYN ) {
    return;
  }

  if ( !zero_point_ ) {
    zero_point_ = message.seqno + 1;
  }

  reassembler.insert( ( message.seqno + message.SYN ).unwrap( zero_point_.value(), inbound_stream.bytes_pushed() ),
                      message.payload.release(),
                      message.FIN,
                      inbound_stream );
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  return { !zero_point_
             ? optional<Wrap32> {}
             : Wrap32::wrap( inbound_stream.bytes_pushed() + inbound_stream.is_closed(), zero_point_.value() ),
           static_cast<uint16_t>(
             min<uint64_t>( numeric_limits<uint16_t>::max(), inbound_stream.available_capacity() ) ) };
}
