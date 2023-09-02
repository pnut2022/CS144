#include "wrapping_integers.hh"

#include <iostream>
#include <limits>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { static_cast<uint32_t>( ( n + static_cast<uint64_t>( zero_point.raw_value_ ) ) % ( 1LL << 32 ) ) };
}

uint64_t getdiff( uint64_t a, uint64_t b )
{
  if ( a <= b ) {
    return b - a;
  }
  return a - b;
}

uint64_t get_nearest_num( uint64_t raw, uint64_t checkpoint )
{
  uint64_t gap = numeric_limits<uint64_t>::max();
  uint64_t ans {};

  auto update_ans = [&ans, &gap, raw, checkpoint]( uint64_t x ) {
    auto now = x * ( 1LL << 32 ) + raw;
    const uint64_t gapnow = getdiff( now, checkpoint );
    if ( gapnow < gap ) {
      gap = gapnow;
      ans = now;
    }
    return gapnow;
  };

  const uint64_t n = checkpoint / ( 1LL << 32 );

  if ( n > 0 ) {
    update_ans( n - 1 );
  }
  update_ans( n );
  update_ans( n + 1 );

  return ans;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  if ( raw_value_ >= zero_point.raw_value_ ) {
    return get_nearest_num( raw_value_ - zero_point.raw_value_, checkpoint );
  }
  return get_nearest_num( static_cast<uint64_t>( raw_value_ ) + ( 1LL << 32 ) - zero_point.raw_value_, checkpoint );
}
