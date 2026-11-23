/**
 * @file     crc16.cpp
 * @brief    CRC16-CCITT 实现
 */
#include "crc16.h"

uint16_t Crc16::calc(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x8000)
                ? ((crc << 1) ^ 0x1021)
                : (crc << 1);
        }
    }
    return crc;
}

#include "proto_common_version.h"
#include "version_manager.h"
COMPONENT_VERSION_REGISTER(proto_common_crc, PROTO_COMMON_VERSION_STRING)
