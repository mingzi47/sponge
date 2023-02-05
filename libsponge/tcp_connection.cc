#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

bool TCPConnection::active() const { return _active; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    // DUMMY_CODE(seg); 
    if (not _active) return;
    _time_since_last_segment_received = 0;

    if (is_syn_sent() and seg.header().ack and seg.payload().size() > 0) {
        return;
    }

    bool sent_empty = false;
    if (_sender.next_seqno_absolute() > 0 and seg.header().ack) {
        if (not _sender.ack_received(seg.header().ackno, seg.header().win)) {
            sent_empty = true;
        }
    }

    const bool recv_flag = _receiver.segment_received(seg);
    if (not recv_flag) {
        sent_empty = true;
    }

    if (seg.header().syn and _sender.next_seqno_absolute() == 0) {
        connect();
        return;
    }
    if (seg.header().rst) {
        if (is_syn_sent() && !seg.header().ack) {
            return;
        }
        unclean_shutdown(false);
        return;
    }
    if (seg.length_in_sequence_space() > 0) {
        sent_empty = true;
    }
    if (sent_empty) {
        if (_receiver.ackno().has_value() and _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }
    push_segments(false);
}

size_t TCPConnection::write(const string &data) {
    const size_t res = _sender.stream_in().write(data);
    push_segments(false);
    return res;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (not _active) return;
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        unclean_shutdown(true);
    }
    push_segments(false);
}

void TCPConnection::end_input_stream() { 
    _sender.stream_in().end_input(); 
    push_segments(false);
}

void TCPConnection::connect() {
    push_segments(true);
}

bool TCPConnection::clean_shutdown() {
    if (_receiver.stream_out().input_ended() and not (_sender.stream_in().eof())) {
        _linger_after_streams_finish = false;
    }
    if (_sender.stream_in().eof() and _sender.bytes_in_flight() == 0 and _receiver.stream_out().input_ended()) {
        if (not _linger_after_streams_finish or time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
    return not _active;
}

void TCPConnection::unclean_shutdown(bool rst_sent) {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
    if (rst_sent) {
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        push_segments(false, rst_sent);
    }
}

bool TCPConnection::push_segments(bool syn_sent, bool rst_sent) {
    _sender.fill_window(syn_sent or is_syn_recv());
    while (!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if (rst_sent) {
            rst_sent = false;
            seg.header().rst = true;
        }
        _segments_out.push(seg);
    }
    clean_shutdown();
    return true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            unclean_shutdown(true);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
