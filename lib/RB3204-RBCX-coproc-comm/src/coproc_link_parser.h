#pragma once

#include <array>

#include "nanopb/pb.h"

#include "coproc_codec.h"

namespace rb {

template <typename MessageType, const pb_msgdesc_t* FieldDesc>
class CoprocLinkParser {
public:
    CoprocLinkParser(rb::CoprocCodec& codec)
        : m_codec(codec)
        , m_buf_index(0)
        , m_frame_len(0)
        , m_state(AwaitingStart) {
        memset(&m_last_msg, 0, sizeof(MessageType));
    }

    ~CoprocLinkParser() {}

    bool add(uint8_t byte) {
        // Zero always starts new frame
        if (byte == 0) {
            m_state = AwaitingLength;
        } else if (m_state == AwaitingLength) {
            m_state = ReceivingFrame;
            m_frame_len = byte;
            m_buf_index = 0;
        } else if (m_state == ReceivingFrame) {
            m_buf[m_buf_index++] = byte;
            if (m_buf_index == m_frame_len) {
                m_state = AwaitingStart;
                return m_codec.decode(m_buf.data(), m_buf_index, FieldDesc, &m_last_msg);
            }
        }
        return false;
    }

    const MessageType& lastMessage() const { return m_last_msg; };

private:
    enum State {
        AwaitingStart,
        AwaitingLength,
        ReceivingFrame,
    };

    std::array<uint8_t, 255> m_buf;
    MessageType m_last_msg;
    CoprocCodec& m_codec;
    size_t m_buf_index;
    size_t m_frame_len;
    State m_state;
};
};
