#include "reassembler.hh"

using namespace std;

std::uint64_t Reassembler::BeginIndex( std::map<uint64_t, std::string>::iterator const& it,
                                       std::uint64_t end_index )
{
  if ( it == substrs_.cend() ) {
    return end_index;
  }

  return it->first;
}

std::uint64_t Reassembler::EndIndex( std::map<uint64_t, std::string>::iterator const& it )
{
  return it->first + it->second.length();
}

std::string Reassembler::SpliteFinish( std::uint64_t begin,
                                       std::uint64_t end,
                                       std::uint64_t first,
                                       std::string data )
{
  const std::uint64_t begin_index = std::max( begin, first );
  const std::uint64_t end_index = std::min( end, data.length() + first );
  if ( begin_index >= end_index ) {
    return data;
  }

  const std::uint64_t length = end_index - begin_index;
  storaged_size_ += length;
  if ( length == data.length() ) {
    substrs_.insert( { begin_index, std::move( data ) } );
    return {};
  }

  substrs_.insert( { begin_index, data.substr( begin_index - first, length ) } );
  return data;
}

void Reassembler::SplitAndInsertToSubStrs( std::uint64_t begin_index,
                                           std::uint64_t end_index,
                                           std::uint64_t first_index,
                                           std::string data )
{
  std::string data_now;
  data_now = SpliteFinish( begin_index, BeginIndex( substrs_.begin(), end_index ), first_index, std::move( data ) );
  if ( data_now.empty() ) {
    return;
  }

  for ( auto it = substrs_.begin(); it != substrs_.end(); it++ ) {
    data_now
      = SpliteFinish( EndIndex( it ), BeginIndex( std::next( it ), end_index ), first_index, std::move( data_now ) );
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

    const std::uint64_t begin_index = output.bytes_pushed();
    const std::uint64_t end_index = begin_index + output.available_capacity();

    SplitAndInsertToSubStrs( begin_index, end_index, first_index, std::move( data ) );

    for ( auto it = substrs_.begin(); it != substrs_.end() && it->first == output.bytes_pushed(); ) {
      storaged_size_ -= it->second.length();
      output.push( std::move( it->second ) );
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
