// servo_interp.c
// 轻量级舵机插值核心实现
#include <math.h>
#include "servo_interp.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------
// 角度->PWM 映射示例
// ---------------------
// servo_interp.c
int angle_to_pwm_180(float angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    // 0.5ms..2.5ms -> 500..2500 @ 1MHz
    float pwm = (angle * (2500.0f - 500.0f) / 180.0f) + 500.0f;
    return (int)lroundf(pwm);
}

int angle_to_pwm_270(float angle)
{
    if (angle < 0) angle = 0;
    if (angle > 270) angle = 270;
    float pwm = (angle * (2500.0f - 500.0f) / 270.0f) + 500.0f;
    return (int)lroundf(pwm);
}


// ---------------------
// 内部：缓动函数
// t \in [0,1] -> s \in [0,1]
// ---------------------
static inline float ease_apply(servo_ease_t e, float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    switch (e) {
        default:
        case EASE_LINEAR:
            return t;
        case EASE_COSINE_S:
            // 0.5*(1 - cos(pi*t))，平滑起停
            return 0.5f * (1.0f - cosf((float)M_PI * t));
        case EASE_CUBIC_S:
            return t < 0.5f ? 4*t*t*t : 1 - powf(-2*t + 2, 3) / 2;
    }
}

// ---------------------
// 单通道初始化与移动
// ---------------------
void servo_channel_init(servo_channel_t* ch,
                        float init_deg,
                        float angle_min_deg,
                        float angle_max_deg,
                        float tick_hz,
                        servo_ease_t ease,
                        int (*angle_to_pwm)(float),
                        volatile uint32_t* ccr_reg)
{
    ch->angle_min_deg = angle_min_deg;
    ch->angle_max_deg = angle_max_deg;
    if (init_deg < angle_min_deg) init_deg = angle_min_deg;
    if (init_deg > angle_max_deg) init_deg = angle_max_deg;
    ch->current_deg = init_deg;
    ch->start_deg = init_deg;
    ch->target_deg = init_deg;
    ch->tick_hz = tick_hz;
    ch->ease = ease;
    ch->ticks_total = 0;
    ch->tick_now = 0;
    ch->angle_to_pwm = angle_to_pwm;
    ch->ccr_reg = ccr_reg;

    if (ch->ccr_reg && ch->angle_to_pwm) {
        *ch->ccr_reg = (uint32_t)ch->angle_to_pwm(ch->current_deg);
    }
}

void servo_channel_move(servo_channel_t* ch,
                        float target_deg,
                        uint32_t duration_ms,
                        servo_ease_t ease)
{
    if (target_deg < ch->angle_min_deg) target_deg = ch->angle_min_deg;
    if (target_deg > ch->angle_max_deg) target_deg = ch->angle_max_deg;

    ch->start_deg  = ch->current_deg;
    ch->target_deg = target_deg;
    ch->ease       = ease;

    // 将ms转换为tick
    float ticks_f = (ch->tick_hz * (float)duration_ms) / 1000.0f;
    if (ticks_f < 1.0f) ticks_f = 1.0f;
    ch->ticks_total = (uint32_t)lroundf(ticks_f);
    ch->tick_now = 0;
}

// 每tick推进
void servo_channel_tick(servo_channel_t* ch)
{
    if (ch->ticks_total == 0) {
        // 空闲，确保PWM输出当前角度
        if (ch->ccr_reg && ch->angle_to_pwm) {
            *ch->ccr_reg = (uint32_t)ch->angle_to_pwm(ch->current_deg);
        }
        return;
    }

    if (ch->tick_now >= ch->ticks_total) {
        ch->current_deg = ch->target_deg;
        ch->ticks_total = 0;
        ch->tick_now = 0;
    } else {
        float t = (float)ch->tick_now / (float)ch->ticks_total; // 0~1
        float s = ease_apply(ch->ease, t);
        ch->current_deg = ch->start_deg + (ch->target_deg - ch->start_deg) * s;
        ch->tick_now++;
    }

    if (ch->ccr_reg && ch->angle_to_pwm) {
        *ch->ccr_reg = (uint32_t)ch->angle_to_pwm(ch->current_deg);
    }
}

// ---------------------
// 播放器：两个通道 + 关键帧
// ---------------------
void servo_player_init(servo_player_t* p,
                       servo_channel_t base,
                       servo_channel_t arm,
                       const servo_waypoint_t* list,
                       uint16_t count,
                       uint8_t loop)
{
    p->base = base;
    p->arm  = arm;
    p->waypoints = list;
    p->count = count;
    p->index = 0;
    p->loop = loop;
    p->active = (count > 0);

    if (p->active) {
        const servo_waypoint_t* w = &p->waypoints[0];
        servo_channel_move(&p->base, w->base_deg, w->duration_ms, w->ease);
        servo_channel_move(&p->arm,  w->arm_deg,  w->duration_ms, w->ease);
    }
}

void servo_player_tick(servo_player_t* p)
{
    if (!p->active || p->count == 0) {
        // 仍然维持输出
        servo_channel_tick(&p->base);
        servo_channel_tick(&p->arm);
        return;
    }

    // 推进两路
    servo_channel_tick(&p->base);
    servo_channel_tick(&p->arm);

    // 若两路都到段末，则切到下一个关键帧
    if (p->base.ticks_total == 0 && p->arm.ticks_total == 0) {
        p->index++;
        if (p->index >= p->count) {
            if (p->loop) p->index = 0;
            else { p->active = 0; return; }
        }
        const servo_waypoint_t* w = &p->waypoints[p->index];
        servo_channel_move(&p->base, w->base_deg, w->duration_ms, w->ease);
        servo_channel_move(&p->arm,  w->arm_deg,  w->duration_ms, w->ease);
    }
}
