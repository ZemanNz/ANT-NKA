#include "nanopb/pb_decode.h"
#include "nanopb/pb_encode.h"

#include "cobs/cobs.h"
#include "coproc_codec.h"

namespace rb {

size_t CoprocCodec::encodeWithHeader(const pb_msgdesc_t* fields, const void* src_struct, uint8_t* buf, size_t size) {
    if (size <= 2)
        return 0;

    const auto len = encode(fields, src_struct, buf + 2, size - 2);
    if (len == 0)
        return 0;

    buf[0] = 0;
    buf[1] = len;
    return len + 2;
}

size_t CoprocCodec::encode(const pb_msgdesc_t* fields, const void* src_struct, uint8_t* buf, size_t size) {
    auto ostream = pb_ostream_from_buffer(m_pb_enc_arena.data(), m_pb_enc_arena.size());
    if (!pb_encode(&ostream, fields, src_struct)) {
        // TODO: how to report errors?
        return 0;
    }

    const auto cobs_res = cobs_encode(buf, size, m_pb_enc_arena.data(), ostream.bytes_written);
    if (cobs_res.status != COBS_ENCODE_OK) {
        // TODO: how to report errors?
        return 0;
    }
    if (cobs_res.out_len > 0xFF) {
        // TODO: how to report errors?
        return 0;
    }

    return cobs_res.out_len;
}

bool CoprocCodec::decode(const uint8_t* buf, size_t size, const pb_msgdesc_t* fields, void* dest_struct) {
    auto cobs_res = cobs_decode(m_pb_dec_arena.data(), m_pb_dec_arena.size(), buf, size);
    if (cobs_res.status != COBS_DECODE_OK) {
        // TODO: how to report errors?
        return false;
    }

    auto istream = pb_istream_from_buffer(m_pb_dec_arena.data(), cobs_res.out_len);
    return pb_decode(&istream, fields, dest_struct);
}

};
