// servo_interp.h
// 轻量级舵机插值核心：支持线性/余弦S曲线，两路示例（基座/手臂）。
// 适配示例：STM32F10x，TIM4->CCR3/CCR4 输出 PWM。
// Author: ChatGPT (GPT-5 Thinking)

#ifndef SERVO_INTERP_H
#define SERVO_INTERP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------------------
// PWM 映射（角度->PWM脉宽，单位: 1/72MHz 定时器计数）
// 若你已在项目中实现，可直接用自己的函数替换。
// -------------------------------------
int angle_to_pwm_180(float angle_deg);
int angle_to_pwm_270(float angle_deg);

// -------------------------------------
// 基本缓动类型（插值曲线）
// -------------------------------------
typedef enum {
    EASE_LINEAR = 0,     // 线性
    EASE_COSINE_S,       // 余弦S曲线（平滑加减速）
    EASE_CUBIC_S,    // 新增
} servo_ease_t;

// -------------------------------------
// 单个通道（一个舵机）的插值状态
// -------------------------------------
typedef struct {
    // 角度限制与当前/目标
    float angle_min_deg;
    float angle_max_deg;
    float current_deg;
    float start_deg;     // 当前段起点
    float target_deg;    // 当前段终点

    // 时间参数
    uint32_t ticks_total;   // 本段总tick数
    uint32_t tick_now;      // 已进行的tick数
    float tick_hz;          // 调度频率（Hz），用于将ms转tick等

    // 缓动类型
    servo_ease_t ease;

    // 角度->PWM 映射
    int (*angle_to_pwm)(float);

    // 硬件输出寄存器地址（例如 &TIM4->CCR3）
    volatile uint32_t* ccr_reg;
} servo_channel_t;

// -------------------------------------
// 路径/关键帧（两个舵机同时执行）
// -------------------------------------
typedef struct {
    float base_deg;      // 基座目标角
    float arm_deg;       // 手臂目标角
    uint32_t duration_ms;
    servo_ease_t ease;   // 两路同一缓动（简化）
} servo_waypoint_t;

typedef struct {
    // 两个通道（示例：基座270°、手臂180°）
    servo_channel_t base;
    servo_channel_t arm;

    // 播放列表
    const servo_waypoint_t* waypoints;
    uint16_t count;
    uint16_t index;         // 当前目标点
    uint8_t  loop;          // 是否循环
    uint8_t  active;        // 是否在执行
} servo_player_t;

// -------------------------------------
// API
// -------------------------------------
void servo_channel_init(servo_channel_t* ch,
                        float init_deg,
                        float angle_min_deg,
                        float angle_max_deg,
                        float tick_hz,
                        servo_ease_t ease,
                        int (*angle_to_pwm)(float),
                        volatile uint32_t* ccr_reg);

void servo_channel_move(servo_channel_t* ch,
                        float target_deg,
                        uint32_t duration_ms,
                        servo_ease_t ease);

// 每tick调用一次（建议在定时器中断里以固定频率调用）
void servo_channel_tick(servo_channel_t* ch);

// 播放器：一次性配置两路 + 关键帧
void servo_player_init(servo_player_t* p,
                       servo_channel_t base,
                       servo_channel_t arm,
                       const servo_waypoint_t* list,
                       uint16_t count,
                       uint8_t loop);

// 每tick调用：内部推进关键帧并驱动两路舵机
void servo_player_tick(servo_player_t* p);

#ifdef __cplusplus
}
#endif

#endif // SERVO_INTERP_H
