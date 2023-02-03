#include "wrapping_integers.hh"
#include <bits/stdint-uintn.h>
#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    const uint64_t max_seqno = uint64_t{1} << 32;
    if (n < max_seqno - isn.raw_value()) {
        return WrappingInt32{isn + n};
    }
    n -= (max_seqno - isn.raw_value());
    const uint32_t res = n % max_seqno;
    return WrappingInt32{res};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // DUMMY_CODE(n, isn, checkpoint);
    const uint64_t max_seqno = uint64_t{1} << 32;
    const int32_t count = (checkpoint < max_seqno ? 0 : checkpoint / max_seqno);
    uint64_t res{0};
    if (n.raw_value() >= isn.raw_value()) {
        res = n.raw_value() - isn.raw_value();
    } else {
        res = max_seqno - isn.raw_value() + n.raw_value();
    }
    res += (max_seqno * count);
    if (res > checkpoint && res >= max_seqno) {
        if (res - checkpoint > max_seqno / 2) res -= max_seqno;
    } else if (res < checkpoint) {
        if (checkpoint - res > max_seqno / 2) res += max_seqno;
    }
    return res;
}
