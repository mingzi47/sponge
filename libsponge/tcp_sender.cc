#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"

#include <asm-generic/socket.h>
#include <bits/stdint-uintn.h>
#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timeout{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _byte_in_flight; }

void TCPSender::fill_window() {
    if (!_syn_flag) {
        TCPSegment seg{};
        seg.header().syn = true;
        send_segment(seg);
        _syn_flag = true;
        return;
    }

    const size_t win_size = _windows_size ? _windows_size : 1;
    size_t remain_win{};
    while ((remain_win = win_size - _next_seqno + _recv_ackno) > 0 && !_fin_flag) {
        TCPSegment seg{};
        const uint16_t length = std::min(TCPConfig::MAX_PAYLOAD_SIZE, remain_win);
        std::string data = _stream.read(length);
        seg.payload() = Buffer{std::move(data)};

        const bool eof = _stream.eof() && seg.length_in_sequence_space() < win_size;
        if (eof) {
            seg.header().fin = true;
            _fin_flag = true;
        }

        const bool empty_segment = seg.length_in_sequence_space() == 0;
        if (empty_segment) return;
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    const uint64_t abs_seqno = unwrap(ackno, _isn, _recv_ackno);

    if (abs_seqno > _next_seqno) return false;
    _windows_size = window_size;

    if (abs_seqno <= _recv_ackno) return true;
    _recv_ackno = abs_seqno;

    _timeout = _initial_retransmission_timeout;
    _consecutive_retransmissions = 0;

    while (!_segments_outstanding.empty()) {
        TCPSegment& seg = _segments_outstanding.front();
        const bool ac_segments = unwrap(seg.header().seqno, _isn, _recv_ackno) 
            + seg.length_in_sequence_space() <= abs_seqno;
        if (ac_segments) {
            _byte_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
        } else break;
    }

    fill_window();
    // 事件 ： 如果当前有未被确认的报文段，开始定时器
    if (!_segments_outstanding.empty()) {
        _time_running = true;
        _timer = 0;
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer += ms_since_last_tick; // ms_since_last_tick 过去了多长时间

    const bool timeout_resend = _timer >= _timeout && !_segments_outstanding.empty();

    // 事件：定时器超时
    if (timeout_resend) {
        _segments_out.push(_segments_outstanding.front());
        _consecutive_retransmissions++;
        _timeout *= 2;
        _time_running = true;
        _timer = 0;
    }

    if (_segments_outstanding.empty()) {
        _time_running = false;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment res{};
    res.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(res);
}

void TCPSender::send_segment(TCPSegment& seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    _byte_in_flight += seg.length_in_sequence_space();
    _segments_out.push(seg);
    _segments_outstanding.push(seg);
    // 事件： 从上面的应用程序接收到数据，定时器没有启动
    if (!_time_running) {
        _time_running = true;
        _timer = 0;
    }
}