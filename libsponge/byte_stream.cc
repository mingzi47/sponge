#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _capacity{capacity} {}

size_t ByteStream::write(const string &data) {
    const size_t res = std::min(data.size(), _capacity - _buffer_size);
    _write_cnt += res;
    _buffer_size += res;
    _buffer.append(Buffer{std::move(std::string{}.assign(data.begin(), data.begin() + res))});
    return res;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t peek_len = std::min(len, _buffer.size());
    // return std::string{}.assign(_buffer.begin(), _buffer.begin() + peek_len);
    std::string tmp = _buffer.concatenate();
    return std::string{}.assign(tmp.begin() , tmp.begin() + peek_len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    const size_t pop_len = std::min(len, _buffer.size());
    _pop_cnt += pop_len;
    _buffer_size -= pop_len;
    _buffer.remove_prefix(pop_len);
    return;
}

void ByteStream::end_input() { _stream_end = true; }

bool ByteStream::input_ended() const { return _stream_end; }

size_t ByteStream::buffer_size() const { return _buffer_size; }

bool ByteStream::buffer_empty() const { return _buffer_size == 0; }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _write_cnt; }

size_t ByteStream::bytes_read() const { return _pop_cnt; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer_size; }
