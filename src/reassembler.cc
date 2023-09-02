#include "reassembler.hh"

using namespace std;

uint64_t Reassembler::get_start_index( map<uint64_t, string>::iterator const& it, uint64_t end_index )
{
  if ( it == substrs_.cend() ) {
    return end_index;
  }

  return it->first;
}

uint64_t Reassembler::get_end_index( map<uint64_t, string>::iterator const& it )
{
  return it->first + it->second.length();
}

string Reassembler::splite_string( uint64_t begin, uint64_t end, uint64_t first, string data )
{
  const uint64_t begin_index = max( begin, first );
  const uint64_t end_index = min( end, data.length() + first );
  if ( begin_index >= end_index ) {
    return data;
  }

  const uint64_t length = end_index - begin_index;
  storaged_size_ += length;
  if ( length == data.length() ) {
    substrs_.insert( { begin_index, move( data ) } );
    return {};
  }

  substrs_.insert( { begin_index, data.substr( begin_index - first, length ) } );
  return data;
}

void Reassembler::split_data_to_substrs( uint64_t begin_index,
                                         uint64_t end_index,
                                         uint64_t first_index,
                                         string data )
{
  string data_now
    = splite_string( begin_index, get_start_index( substrs_.begin(), end_index ), first_index, move( data ) );
  if ( data_now.empty() ) {
    return;
  }

  for ( auto it = substrs_.begin(); it != substrs_.end(); it++ ) {
    data_now = splite_string(
      get_end_index( it ), get_start_index( next( it ), end_index ), first_index, move( data_now ) );
    if ( data_now.empty() ) {
      return;
    }
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  if ( output.is_closed() ) {
    return;
  }

  if ( is_last_substring ) {
    close_index_ = first_index + data.length();
    close_flag_ = true;
  }

  const uint64_t begin_index = output.bytes_pushed();
  const uint64_t end_index = begin_index + output.available_capacity();

  split_data_to_substrs( begin_index, end_index, first_index, move( data ) );

  for ( auto it = substrs_.begin(); it != substrs_.end() && it->first == output.bytes_pushed(); ) {
    storaged_size_ -= it->second.length();
    output.push( move( it->second ) );
    it = substrs_.erase( it );
  }

  if ( close_flag_ && output.bytes_pushed() == close_index_ ) {
    output.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  return storaged_size_;
}
