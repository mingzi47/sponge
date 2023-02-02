#include "stream_reassembler.hh"
#include <cstddef>
#include <optional>
#include <queue>
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if (index >= _push_pos + _capacity) return;

    SubString ssd = cut_substring(data, index);
    merge_substring(ssd);

    while (_unassembled_bytes > 0 && _output.remaining_capacity() > 0) {
        auto iter = _buffer.begin();
        if (iter->_begin != _push_pos) break;
        const size_t push_len = _output.write(iter->_data);
        _unassembled_bytes -= push_len;
        _push_pos += push_len;
        if (iter->_size == push_len) {
            _buffer.erase(iter);
        } else {
            SubString unass{std::string{}.assign(iter->_data.begin() + _push_pos, iter->_data.end())
            ,_push_pos, iter->_size - push_len};
            _buffer.erase(iter);
            _buffer.insert(unass);
        }
    }
    if (index + data.length() <= _push_pos + _capacity && eof) {
        _is_eof |= eof;
    } 
    if (_unassembled_bytes == 0 && _is_eof) {
        _output.end_input();
    }
}

SubString StreamReassembler::cut_substring(const std::string &data, const size_t index) {
    // \in
    SubString res{};
    if (index + data.size() <= _push_pos) return res;
    // 
    else if (index < _push_pos) {
        const size_t len = _push_pos - index;
        res._begin = _push_pos;
        res._data.assign(data.begin() + len, data.end());
        res._size = res._data.size();
    } else {
        res._begin = index;
        res._data = data;
        res._size = data.size();
    }
    _unassembled_bytes += res._size;
    return res;
}

void StreamReassembler::merge_substring(SubString &ssd) {
    if (ssd._size == 0) return; 
    do {
        // next
        auto iter = _buffer.lower_bound(ssd);
        std::optional<size_t> res{};
        while (iter != _buffer.end() && (res = ssd.merge(*iter)) != std::nullopt) {
            _unassembled_bytes -= res.value();
            _buffer.erase(iter);
            iter = _buffer.lower_bound(ssd);
        }
        // pre
        if (iter == _buffer.begin()) break;
        iter--;

        while (iter != _buffer.end() && (res = ssd.merge(*iter)) != std::nullopt) {
            _unassembled_bytes -=  res.value();
            _buffer.erase(iter);
            iter = _buffer.lower_bound(ssd);
            if (iter == _buffer.begin()) break;
            iter--;
        }
    } while(false);
    _buffer.insert(ssd);
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
