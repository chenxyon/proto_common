/**
 * @file     chip_addr.h
 * @brief    芯片地址 + 命令集（全项目共用）
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║              永久全局规则（组件开发强制约束）               ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║ 1. 组件必须用 class 封装（禁止纯 C 函数 / namespace）       ║
 * ║ 2. 配置通过构造函数 Config 结构体注入（禁止硬编码）         ║
 * ║ 3. 组件零板级依赖（禁止 include board_config.h / pin）      ║
 * ║ 4. 状态封装在实例里（禁止全局单例 / static 全局变量）       ║
 * ║ 5. 禁止 include Arduino.h 里冲突的宏（RESET/DISPLAY/ERROR）║
 * ║ 6. 兼容旧 API 的用 #define CMD_XXX 别名（不改调用方）      ║
 * ║ 7. 组件目录结构：include/ + src/（PlatformIO 自动识别）     ║
 * ║ 8. 板子侧只做组装：main.cpp 里实例化组件 + 传 Config        ║
 * ║ 9. GPIO 扩展优先：同类 GPIO ≥ 2 个时，优先用类似串行扩展器       ║
 * ║    （如 74HC595/165、PCA9685、CD4021/4094、MCP23017/ADS1115 等）║
 * ║    芯片型号+数量+节约引脚数在注释中列出，由用户审核后实施        ║
 * ║ 10. 版本强制：所有自定义组件必须接入 version_manager，       ║
 * ║     包含 xxx_version.h + version.txt + COMPONENT_VERSION_   ║
 * ║     REGISTER() 宏调用，禁止跳过。板子固件也必须注册自身版本   ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * 地址分配（v1.0，冻结后改版本号）：
 *   0x01 = 主控总线
 *   0x02 = AI 核
 *   0x03 = IO 核
 *   0x04 = 传感器核
 *
 * 修改：2026-11-21 C++ 重构 + 永久全局规则写入
 */
#ifndef CHIP_ADDR_H
#define CHIP_ADDR_H

#include <stdint.h>

namespace ChipAddr {
    /* 芯片地址 */
    static constexpr uint8_t MASTER    = 0x01;
    static constexpr uint8_t AI        = 0x02;
    static constexpr uint8_t IO        = 0x03;
    static constexpr uint8_t SENSOR    = 0x04;
    static constexpr uint8_t BROADCAST = 0xFF;

    static constexpr uint8_t MIN = 0x01;
    static constexpr uint8_t MAX = 0x04;

    inline bool valid(uint8_t a) { return a >= MIN && a <= MAX; }
}

enum class ChipCmd : uint8_t {
    /* 通用 */
    PING     = 0x01,
    PONG     = 0x02,
    ACK      = 0x03,

    /* AI 通路 */
    AI_WAKE   = 0x10,
    ASR_RESULT = 0x11,
    AI_REPLY   = 0x12,

    /* TTS 通路 */
    TTS_SAY   = 0x20,
    TTS_PCM   = 0x21,
    TTS_DONE  = 0x22,

    /* IO 通路（注意：DISPLAY 改名 IO_DISPLAY，Arduino.h 有 #define DISPLAY！）*/
    IO_DISPLAY = 0x30,
    BUTTON    = 0x31,
    MIC_DATA  = 0x32,

    /* 传感器通路 */
    SENSOR_READ  = 0x40,
    SENSOR_DATA  = 0x41,
    PIR_MOTION   = 0x42,

    /* 系统（注意：RESET/ERROR 改名，Arduino.h 有 #define！）*/
    SOFT_RESET = 0xF0,
    BAD_CMD    = 0xFF,
};

/* ===== C 风格兼容宏（供 chip2/chip4 旧代码用，Phase 2 前不改）===== */
#define CHIP_ADDR_MASTER     ChipAddr::MASTER
#define CHIP_ADDR_AI         ChipAddr::AI
#define CHIP_ADDR_IO         ChipAddr::IO
#define CHIP_ADDR_SENSOR     ChipAddr::SENSOR
#define CHIP_ADDR_BROADCAST  ChipAddr::BROADCAST

#define CMD_PING          ((uint8_t)ChipCmd::PING)
#define CMD_PONG          ((uint8_t)ChipCmd::PONG)
#define CMD_ACK           ((uint8_t)ChipCmd::ACK)
#define CMD_AI_WAKE       ((uint8_t)ChipCmd::AI_WAKE)
#define CMD_ASR_RESULT    ((uint8_t)ChipCmd::ASR_RESULT)
#define CMD_AI_REPLY      ((uint8_t)ChipCmd::AI_REPLY)
#define CMD_TTS_SAY       ((uint8_t)ChipCmd::TTS_SAY)
#define CMD_TTS_PCM       ((uint8_t)ChipCmd::TTS_PCM)
#define CMD_TTS_DONE      ((uint8_t)ChipCmd::TTS_DONE)
#define CMD_IO_DISPLAY    ((uint8_t)ChipCmd::IO_DISPLAY)
/* 兼容旧宏名 DISPLAY（Arduino.h 冲突） */
#define CMD_DISPLAY       CMD_IO_DISPLAY
#define CMD_BUTTON        ((uint8_t)ChipCmd::BUTTON)
#define CMD_MIC_DATA      ((uint8_t)ChipCmd::MIC_DATA)
#define CMD_SENSOR_READ   ((uint8_t)ChipCmd::SENSOR_READ)
#define CMD_SENSOR_DATA   ((uint8_t)ChipCmd::SENSOR_DATA)
#define CMD_PIR_MOTION    ((uint8_t)ChipCmd::PIR_MOTION)
#define CMD_SOFT_RESET    ((uint8_t)ChipCmd::SOFT_RESET)
/* 兼容旧宏名 */
#define CMD_RESET         CMD_SOFT_RESET
#define CMD_ERROR         ((uint8_t)ChipCmd::BAD_CMD)

#endif /* CHIP_ADDR_H */
