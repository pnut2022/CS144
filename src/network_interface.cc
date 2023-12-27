#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t ip = next_hop.ipv4_numeric();

  auto cached_ip_it = cached_ip_.find( ip );
  if ( cached_ip_it != cached_ip_.end() ) {
    EthernetFrame e_frame {
      EthernetHeader { cached_ip_it->second.addr, ethernet_address_, EthernetHeader::TYPE_IPv4 },
      serialize( dgram ) };
    out_frame_.push( std::move( e_frame ) );
    return;
  }

  auto waiting_arps_it = waiting_arps_.find( ip );
  if ( waiting_arps_it != waiting_arps_.end() ) {
    waiting_arps_it->second.cached_data.push_back( dgram );
    return;
  }

  ARPMessage arp_req {};
  arp_req.opcode = ARPMessage::OPCODE_REQUEST;
  arp_req.sender_ethernet_address = ethernet_address_;
  arp_req.sender_ip_address = ip_address_.ipv4_numeric();
  arp_req.target_ip_address = ip;
  EthernetFrame e_frame { EthernetHeader { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP },
                          serialize( arp_req ) };
  out_frame_.push( std::move( e_frame ) );

  ArpWaiter arp_waiter { { dgram } };

  waiting_arps_.insert( { ip, std::move( arp_waiter ) } );
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return {};
  }

  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( !parse( dgram, frame.payload ) ) {
      return {};
    }
    return dgram;
  }

  ARPMessage arpmsg;
  if ( !parse( arpmsg, frame.payload ) || arpmsg.target_ip_address != ip_address_.ipv4_numeric() ) {
    return {};
  }

  cached_ip_.insert( { arpmsg.sender_ip_address, IpWaiter { arpmsg.sender_ethernet_address } } );
  if ( arpmsg.opcode == ARPMessage::OPCODE_REQUEST ) {
    ARPMessage reply_msg;
    reply_msg.opcode = ARPMessage::OPCODE_REPLY;
    reply_msg.sender_ethernet_address = ethernet_address_;
    reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
    reply_msg.target_ethernet_address = arpmsg.sender_ethernet_address;
    reply_msg.target_ip_address = arpmsg.sender_ip_address;
    EthernetFrame reply_frame { { arpmsg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP },
                                serialize( reply_msg ) };
    out_frame_.push( std::move( reply_frame ) );
  }

  if ( arpmsg.opcode == ARPMessage::OPCODE_REPLY ) {
    auto it = waiting_arps_.find( arpmsg.sender_ip_address );
    if ( it == waiting_arps_.end() ) {
      return {};
    }

    for ( auto const& data : it->second.cached_data ) {
      EthernetFrame e_frame {
        EthernetHeader { arpmsg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_IPv4 },
        serialize( data ) };
      out_frame_.push( std::move( e_frame ) );
    }

    waiting_arps_.erase( arpmsg.sender_ip_address );
  }

  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for ( auto it = cached_ip_.begin(); it != cached_ip_.end(); ) {
    it->second.waiting_time += ms_since_last_tick;
    if ( it->second.waiting_time >= kIpTtl ) {
      it = cached_ip_.erase( it );
    } else {
      ++it;
    }
  }

  for ( auto it = waiting_arps_.begin(); it != waiting_arps_.end(); ) {
    it->second.waiting_time += ms_since_last_tick;
    if ( it->second.waiting_time >= kArpTtl ) {
      it = waiting_arps_.erase( it );
    } else {
      ++it;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if ( out_frame_.empty() ) {
    return {};
  }
  auto frame = out_frame_.front();
  out_frame_.pop();
  return frame;
}
