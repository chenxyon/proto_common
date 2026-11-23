/**
 * @file     crc16.h
 * @brief    CRC16-CCITT — class 封装
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
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * 本组件特殊：工具类，无状态，用 static 方法 + delete 构造函数
 */
#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

class Crc16 {
public:
    /* CRC16-CCITT（多项式 0x1021, 初始 0xFFFF）*/
    static uint16_t calc(const uint8_t* data, size_t len);

    /* 给完整帧算 CRC（语义化别名）*/
    static uint16_t frame(const uint8_t* frame, size_t len) { return calc(frame, len); }

private:
    Crc16() = delete;    /* 禁止实例化 */
};

#endif /* CRC16_H */
