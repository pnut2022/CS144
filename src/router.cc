#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  items_.emplace_back( route_prefix, prefix_length, next_hop, interface_num );
}

void Router::route_dgram( std::optional<InternetDatagram> dgram_opt )
{
  if ( !dgram_opt.has_value() ) {
    return;
  }
  auto& dgram = dgram_opt.value();
  if ( dgram.header.ttl <= 1 ) {
    return;
  }
  dgram.header.ttl--;
  dgram.header.compute_checksum();
  auto dst_ip = dgram.header.dst;
  auto it = longest_prefix_match( dst_ip );
  if ( it == items_.end() ) {
    return;
  }
  auto& target_interface = interface( it->interface_num );
  target_interface.send_datagram( dgram, it->next_hop.value_or( Address::from_ipv4_numeric( dst_ip ) ) );
}

void Router::route()
{
  for ( auto& cur_interface : interfaces_ ) {
    auto received_dgram = cur_interface.maybe_receive();
    route_dgram( std::move( received_dgram ) );
  }
}

std::vector<Router::RouteItem>::iterator Router::longest_prefix_match( uint32_t dst_ip )
{
  auto res = items_.end();
  auto max_length = -1;
  for ( auto it = items_.begin(); it != items_.end(); ++it ) {
    auto len = match_length( dst_ip, it->route_prefix, it->prefix_length );
    if ( len > max_length ) {
      max_length = len;
      res = it;
    }
  }
  return res;
}

int Router::match_length( uint32_t src_ip, uint32_t tgt_ip, uint8_t tgt_len )
{
  if ( tgt_len == 0 ) {
    return 0;
  }

  if ( tgt_len > 32 ) {
    return -1;
  }

  uint8_t const len = 32U - tgt_len;
  src_ip = src_ip >> len;
  tgt_ip = tgt_ip >> len;
  return src_ip == tgt_ip ? tgt_len : -1;
}