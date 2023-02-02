#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstddef>
#include <cstdint>
#include <string>
#include <set>
#include <optional>
#include <unordered_map>

struct SubString {
    std::string _data{};
    size_t _begin{0};
    size_t _size{0};

    SubString() = default;
    SubString(const std::string &data, const size_t begin, const size_t size)
      : _data{data}, _begin{begin}, _size{size} {}

    bool operator<(const SubString &rhs) const { return _begin < rhs._begin; }

    std::optional<size_t> merge(const SubString &other) {
        SubString lhs{}, rhs{};
        if (_begin > other._begin) {
            lhs = other;
            rhs = *this;
        } else {
            lhs = *this;
            rhs = other;
        }
        // can't merge
        if (lhs._begin + lhs._size < rhs._begin) return std::nullopt; 
        // \in
        if (lhs._begin + lhs._size >= rhs._begin + rhs._size) {
          *this = lhs;
          return rhs._size;
        }
        const size_t res = _begin + _size - rhs._begin;
        lhs._data += rhs._data.substr((lhs._begin + lhs._size) - rhs._begin);
        lhs._size = lhs._data.size();
        *this = lhs;
        return res;
    }
};

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    std::set<SubString> _buffer{};
    size_t _unassembled_bytes{0};
    size_t _push_pos{0};
    bool _is_eof{false};
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    SubString cut_substring(const std::string &data, const size_t index);
    void merge_substring(SubString &ssd);

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
