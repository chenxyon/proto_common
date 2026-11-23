/**
 * @file     proto_frame.cpp
 * @brief    ProtoFrame 编解码 + ProtoDecoder 流式状态机
 */
#include "proto_frame.h"
#include "crc16.h"
#include <Arduino.h>
#include <string.h>

/* ===== 辅助 ===== */
static inline uint8_t splitSrcDst(uint8_t s, uint8_t d) {
    return static_cast<uint8_t>(((s << 4) & 0xF0) | (d & 0x0F));
}
static inline uint8_t getSrc(uint8_t sd) { return (sd >> 4) & 0x0F; }
static inline uint8_t getDst(uint8_t sd) { return sd & 0x0F; }

/* ===== ProtoFrame ===== */

void ProtoFrame::makeDisplay(uint8_t from, uint8_t to, const char* text, uint16_t textLen)
{
    src = from; dst = to; cmd = static_cast<uint8_t>(ChipCmd::IO_DISPLAY);
    uint16_t n = (textLen > 0) ? textLen : (text ? (uint16_t)strlen(text) : 0);
    if (n > ProtoConst::PAYLOAD_MAX) n = ProtoConst::PAYLOAD_MAX;
    len = 1 + n;
    if (n > 0 && text) memcpy(payload, text, n);
    crc = 0;
}

bool ProtoFrame::encode(uint8_t* outBuf, uint16_t& outLen) const
{
    if (!outBuf) return false;

    uint16_t plen = payloadLen();
    uint16_t total = ProtoConst::HEADER_SIZE + plen + ProtoConst::CRC_SIZE;
    if (total > ProtoConst::FRAME_MAX) return false;

    outBuf[0] = ProtoConst::SYNC_H;
    outBuf[1] = ProtoConst::SYNC_L;
    outBuf[2] = splitSrcDst(src, dst);
    outBuf[3] = (len >> 8) & 0xFF;
    outBuf[4] = len & 0xFF;
    outBuf[5] = cmd;

    if (plen > 0) memcpy(&outBuf[6], payload, plen);

    uint16_t crc = Crc16::calc(outBuf, ProtoConst::HEADER_SIZE + plen);
    outBuf[ProtoConst::HEADER_SIZE + plen]     = (crc >> 8) & 0xFF;
    outBuf[ProtoConst::HEADER_SIZE + plen + 1] = crc & 0xFF;

    outLen = total;
    return true;
}

bool ProtoFrame::build(uint8_t src, uint8_t dst, uint8_t cmd,
                       const uint8_t* payload, uint16_t payload_len,
                       uint8_t* out_buf, uint16_t* out_len)
{
    if (!out_buf || !out_len) return false;
    ProtoFrame f;
    f.src = src; f.dst = dst; f.cmd = cmd; f.len = 1 + payload_len;
    if (payload_len > 0 && payload) memcpy(f.payload, payload, payload_len);
    return f.encode(out_buf, *out_len);
}

bool ProtoFrame::decode(const uint8_t* rawBuf, size_t rawLen, ProtoFrame& out)
{
    if (!rawBuf || rawLen < ProtoConst::HEADER_SIZE + ProtoConst::CRC_SIZE) return false;
    if (rawBuf[0] != ProtoConst::SYNC_H || rawBuf[1] != ProtoConst::SYNC_L) return false;

    uint8_t sd = rawBuf[2];
    uint8_t src = getSrc(sd);
    uint8_t dst = getDst(sd);

    uint16_t lenField = (static_cast<uint16_t>(rawBuf[3]) << 8) | rawBuf[4];
    if (lenField < 1 || lenField > (ProtoConst::PAYLOAD_MAX + 1)) return false;
    uint16_t plen = lenField - 1;
    uint16_t total = ProtoConst::HEADER_SIZE + plen + ProtoConst::CRC_SIZE;
    if (rawLen < total) return false;

    uint16_t crcCalc = Crc16::calc(rawBuf, ProtoConst::HEADER_SIZE + plen);
    uint16_t crcRecv = (static_cast<uint16_t>(rawBuf[ProtoConst::HEADER_SIZE + plen]) << 8)
                     | rawBuf[ProtoConst::HEADER_SIZE + plen + 1];
    if (crcCalc != crcRecv) return false;

    out.src = src; out.dst = dst; out.len = lenField;
    out.cmd = rawBuf[5]; out.crc = crcRecv;
    if (plen > 0) memcpy(out.payload, &rawBuf[6], plen);
    return true;
}

bool ProtoFrame::parse(const uint8_t* buf, size_t buf_len, ProtoFrame& out, size_t* consumed)
{
    if (consumed) *consumed = 0;
    bool ok = decode(buf, buf_len, out);
    if (ok && consumed) *consumed = out.totalLen();
    return ok;
}

void ProtoFrame::print(const char* tag) const
{
    Serial.printf("[%s] src=0x%02X dst=0x%02X cmd=0x%02X len=%u payloadLen=%u\n",
                  tag ? tag : "?", src, dst, cmd, len, payloadLen());
}

/* ===== ProtoDecoder ===== */

void ProtoDecoder::reset()
{
    state_ = State::WAIT_SYNC_H;
    bytesCollected_ = 0;
    payloadRemaining_ = 0;
    memset(&frameBuf_, 0, sizeof(frameBuf_));
}

bool ProtoDecoder::feed(uint8_t byte, ProtoFrame& out)
{
    switch (state_) {
    case State::WAIT_SYNC_H:
        if (byte == ProtoConst::SYNC_H) state_ = State::WAIT_SYNC_L;
        return false;
    case State::WAIT_SYNC_L:
        if (byte == ProtoConst::SYNC_L) state_ = State::WAIT_SRC_DST;
        else if (byte == ProtoConst::SYNC_H) { }
        else state_ = State::WAIT_SYNC_H;
        return false;
    case State::WAIT_SRC_DST:
        frameBuf_.src = getSrc(byte);
        frameBuf_.dst = getDst(byte);
        state_ = State::WAIT_LEN_H;
        return false;
    case State::WAIT_LEN_H:
        frameBuf_.len = static_cast<uint16_t>(byte) << 8;
        state_ = State::WAIT_LEN_L;
        return false;
    case State::WAIT_LEN_L:
        frameBuf_.len |= byte;
        state_ = State::WAIT_CMD;
        return false;
    case State::WAIT_CMD:
        frameBuf_.cmd = byte;
        payloadRemaining_ = frameBuf_.len - 1;
        bytesCollected_ = 0;
        state_ = (payloadRemaining_ == 0) ? State::WAIT_CRC_H : State::WAIT_PAYLOAD;
        return false;
    case State::WAIT_PAYLOAD:
        frameBuf_.payload[bytesCollected_++] = byte;
        if (--payloadRemaining_ == 0) state_ = State::WAIT_CRC_H;
        return false;
    case State::WAIT_CRC_H:
        frameBuf_.crc = static_cast<uint16_t>(byte) << 8;
        state_ = State::WAIT_CRC_L;
        return false;
    case State::WAIT_CRC_L: {
        frameBuf_.crc |= byte;

        uint8_t tmp[ProtoConst::HEADER_SIZE + ProtoConst::PAYLOAD_MAX];
        tmp[0] = ProtoConst::SYNC_H; tmp[1] = ProtoConst::SYNC_L;
        tmp[2] = splitSrcDst(frameBuf_.src, frameBuf_.dst);
        tmp[3] = (frameBuf_.len >> 8) & 0xFF;
        tmp[4] = frameBuf_.len & 0xFF;
        tmp[5] = frameBuf_.cmd;
        if (bytesCollected_ > 0) memcpy(&tmp[6], frameBuf_.payload, bytesCollected_);

        uint16_t crcCalc = Crc16::calc(tmp, ProtoConst::HEADER_SIZE + bytesCollected_);
        bool ok = (crcCalc == frameBuf_.crc);
        if (ok) out = frameBuf_;

        reset();
        return ok;
    }
    }
    reset();
    return false;
}

#include "proto_common_version.h"
#include "version_manager.h"
COMPONENT_VERSION_REGISTER(proto_common, PROTO_COMMON_VERSION_STRING)
