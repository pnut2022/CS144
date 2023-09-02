#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  auto pushed_size = min( data.length(), capacity_ - buffer_.size() );
  buffer_.insert(
    buffer_.cend(), data.begin(), data.begin() + static_cast<string::difference_type>( pushed_size ) );
  write_count_ += pushed_size;
}

void Writer::close()
{
  input_ended_flag_ = true;
}

void Writer::set_error()
{
  error_ = true;
}

bool Writer::is_closed() const
{
  return input_ended_flag_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return write_count_;
}

string_view Reader::peek() const
{
  return { buffer_.begin(), buffer_.end() };
}

bool Reader::is_finished() const
{
  return input_ended_flag_ && buffer_.empty();
}

bool Reader::has_error() const
{
  return error_;
}

void Reader::pop( uint64_t len )
{
  buffer_.erase( buffer_.cbegin(), buffer_.cbegin() + static_cast<string::difference_type>( len ) );
  read_count_ += len;
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  return read_count_;
}
