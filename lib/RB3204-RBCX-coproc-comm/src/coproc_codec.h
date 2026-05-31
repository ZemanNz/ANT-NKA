#pragma once

#include "nanopb/pb.h"
#include <array>
#include <stddef.h>
#include <stdint.h>

namespace rb {

class CoprocCodec {

    static constexpr size_t MaxPayloadSize = 254;
    std::array<uint8_t, MaxPayloadSize> m_pb_enc_arena;
    std::array<uint8_t, MaxPayloadSize> m_pb_dec_arena;

public:
    static constexpr size_t MaxFrameSize = 257;

    size_t encode(const pb_msgdesc_t* fields, const void* src_struct, uint8_t* buf, size_t size);
    size_t encodeWithHeader(const pb_msgdesc_t* fields, const void* src_struct, uint8_t* buf, size_t size);

    bool decode(const uint8_t* buf, size_t size, const pb_msgdesc_t* fields, void* dest_struct);
};

};
