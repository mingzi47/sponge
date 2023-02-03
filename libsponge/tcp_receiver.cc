#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <bits/stdint-uintn.h>
#include <optional>
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    const bool issyn = seg.header().syn;
    const bool isfin = seg.header().fin;
    const bool refuse = (_isn != nullopt && issyn) || (_fin && isfin) || (_isn == nullopt && !issyn);

    if (refuse) return false;
    if (issyn) {
        _isn = seg.header().seqno;
        _pos = 1;
    }

    if (isfin) _fin = true;

    const uint64_t abs_seqno = unwrap(seg.header().seqno, _isn.value(), _pos);
    const uint64_t index = abs_seqno + (issyn ? 1 : 0);

    const bool inbound = abs_seqno + seg.length_in_sequence_space() <= _pos || abs_seqno >= _pos + window_size();

    if (!issyn && !isfin && inbound) 
        return false;

    _reassembler.push_substring(seg.payload().copy(), index - 1, isfin); // SYN
    _pos = _reassembler.head_pos() + 1;

    if (_reassembler.input_ended()) {   // FIN 
        _pos++;
    }

    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    return _pos == 0 ? std::nullopt : optional<WrappingInt32>{wrap(_pos, _isn.value())};
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
