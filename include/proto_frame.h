/**
 * @file     proto_frame.h
 * @brief    跨芯片通信统一帧格式 V1.0（C++ 封装，二进制完全兼容）
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║        永久全局规则（所有组件强制约束，后续不得违反）       ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║ 1. 组件必须用 class 封装（禁止纯 C 函数 / namespace 作 API）║
 * ║ 2. 配置通过构造函数 Config 结构体注入（禁止硬编码）         ║
 * ║ 3. 组件零板级依赖（禁止 include board_config.h / pin）      ║
 * ║ 4. 状态封装在实例里（禁止全局单例 / static 全局变量）       ║
 * ║ 5. 禁止 Arduino.h 冲突宏（RESET→SOFT_RESET / DISPLAY→IO_DISPLAY / ERROR→BAD_CMD）║
 * ║ 6. 兼容旧 API 用 #define CMD_XXX 别名                      ║
 * ║ 7. 组件目录结构：include/ + src/（PlatformIO 自动识别）     ║
 * ║ 8. 板子侧只做组装：main.cpp 实例化 + 传 Config              ║
 * ║ 9. GPIO 扩展优先：同类 GPIO ≥ 2 个时，优先用类似串行扩展器       ║
 * ║    （如 74HC595/165、PCA9685、CD4021/4094、MCP23017/ADS1115 等）║
 * ║    芯片型号+数量+节约引脚数在注释中列出，由用户审核后实施        ║
 * ║ 10. 版本强制：所有自定义组件必须接入 version_manager，       ║
 * ║     包含 xxx_version.h + version.txt + COMPONENT_VERSION_   ║
 * ║     REGISTER() 宏调用，禁止跳过。板子固件也必须注册自身版本   ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * 帧结构（跟旧 C 版 100% 一致）：
 *   ┌────────┬──────────┬──────────┬──────────────┬──────────────┬────────┐
 *   │ SYNC   │ SRC/DST  │ LEN      │ CMD          │ PAYLOAD      │ CRC16  │
 *   │ 2B     │ 1B       │ 2B       │ 1B           │ ≤ 239B       │ 2B     │
 *   │ 0xAA55 │ 高4=源   │ CMD+PAY  │              │              │        │
 *   │        │ 低4=目   │          │              │              │        │
 *   └────────┴──────────┴──────────┴──────────────┴──────────────┴────────┘
 *
 * 总帧长 = 6B header + payload + 2B CRC（不含 SYNC）
 *
 * 修改：2026-11-21 C++ 重构（class ProtoFrame + class ProtoDecoder）
 */
#ifndef PROTO_FRAME_H
#define PROTO_FRAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "chip_addr.h"

/* ===== 帧常量（公开，因为二进制格式）===== */
namespace ProtoConst {
    static constexpr uint8_t  SYNC_H          = 0xAA;
    static constexpr uint8_t  SYNC_L          = 0x55;
    static constexpr uint16_t SYNC             = 0xAA55;

    static constexpr uint8_t  HEADER_SIZE     = 6;    /* SYNC(2) + SRC_DST(1) + LEN(2) + CMD(1) */
    static constexpr uint8_t  CRC_SIZE         = 2;
    static constexpr uint8_t  PAYLOAD_MAX      = 239;
    static constexpr uint16_t FRAME_MAX        = HEADER_SIZE + PAYLOAD_MAX + CRC_SIZE;  /* 247B */
    static constexpr uint8_t  PROTO_VER        = 0x01;
}

/* ===== ProtoFrame：协议帧容器 ===== */
class ProtoFrame {
public:
    /* 字段 */
    uint8_t  src   = 0;
    uint8_t  dst   = 0;
    uint16_t len   = 0;        /* CMD(1) + PAYLOAD(N)，不包含 header/crc */
    uint8_t  cmd   = 0;
    uint8_t  payload[ProtoConst::PAYLOAD_MAX] = {};
    uint16_t crc   = 0;

    /* ===== 便捷构造 ===== */
    void makePing(uint8_t from, uint8_t to) {
        src = from; dst = to; cmd = static_cast<uint8_t>(ChipCmd::PING); len = 1;
        payload[0] = 0; crc = 0;
    }
    void makePong(uint8_t from, uint8_t to) {
        src = from; dst = to; cmd = static_cast<uint8_t>(ChipCmd::PONG); len = 1;
        payload[0] = 0; crc = 0;
    }
    void makeReset(uint8_t from, uint8_t to) {
        src = from; dst = to; cmd = static_cast<uint8_t>(ChipCmd::SOFT_RESET); len = 1;
        payload[0] = 0; crc = 0;
    }
    void makeAck(uint8_t from, uint8_t to) {
        src = from; dst = to; cmd = static_cast<uint8_t>(ChipCmd::ACK); len = 1;
        payload[0] = 0; crc = 0;
    }
    void makeDisplay(uint8_t from, uint8_t to, const char* text, uint16_t textLen = 0);

    /* ===== payload 访问 ===== */
    uint16_t payloadLen() const { return (len > 1) ? (len - 1) : 0; }
    uint16_t totalLen()   const { return ProtoConst::HEADER_SIZE + payloadLen() + ProtoConst::CRC_SIZE; }

    /* ===== 编码/解码 ===== */
    /* 编码：把帧序列化成 wire 格式 */
    bool encode(uint8_t* outBuf, uint16_t& outLen) const;

    /* 解码（一次性，完整帧已在 buf 里）*/
    static bool decode(const uint8_t* rawBuf, size_t rawLen, ProtoFrame& out);

    /* 直接给 C 函数调用（兼容旧 API 签名）*/
    static bool build(uint8_t src, uint8_t dst, uint8_t cmd,
                      const uint8_t* payload, uint16_t payload_len,
                      uint8_t* out_buf, uint16_t* out_len);
    static bool parse(const uint8_t* buf, size_t buf_len,
                      ProtoFrame& out, size_t* consumed);

    /* ===== 调试 ===== */
    void print(const char* tag) const;
};

/* ===== ProtoDecoder：流式状态机 ===== */
class ProtoDecoder {
public:
    enum class State : uint8_t {
        WAIT_SYNC_H = 0, WAIT_SYNC_L, WAIT_SRC_DST,
        WAIT_LEN_H, WAIT_LEN_L, WAIT_CMD,
        WAIT_PAYLOAD, WAIT_CRC_H, WAIT_CRC_L
    };

    ProtoDecoder() { reset(); }
    void reset();

    /* 喂入一个字节，返回 true 表示解析出完整帧 */
    bool feed(uint8_t byte, ProtoFrame& out);

private:
    State    state_ = State::WAIT_SYNC_H;
    uint16_t payloadRemaining_ = 0;
    uint16_t bytesCollected_   = 0;

    ProtoFrame frameBuf_;    /* 积累中 */
};

#/* ===== C 风格兼容宏（供旧 chip2/chip4/chip1 旧代码用）===== */
#define PROTO_PAYLOAD_MAX      ProtoConst::PAYLOAD_MAX
#define PROTO_FRAME_MAX        ProtoConst::FRAME_MAX
#define PROTO_FRAME_HEADER_SIZE ProtoConst::HEADER_SIZE

#endif /* PROTO_FRAME_H */
