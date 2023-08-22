#pragma once

#include "byte_stream.hh"

#include <map>
#include <string>

class Reassembler
{
  std::map<std::uint64_t, std::string> substrs_ {};
  std::uint64_t storaged_size_ {};
  std::uint64_t close_index_ {};
  bool close_flag_ {};

  std::string SpliteFinish( std::uint64_t begin, std::uint64_t end, std::uint64_t first, std::string data );
  void SplitAndInsertToSubStrs( std::uint64_t begin, std::uint64_t end, std::uint64_t first, std::string data );

  std::uint64_t BeginIndex( std::map<uint64_t, std::string>::iterator const& it, std::uint64_t end_index );
  static std::uint64_t EndIndex( std::map<uint64_t, std::string>::iterator const& it );

public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
};
