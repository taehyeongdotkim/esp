#include "mbed.h"

// ----- PIN MAP -----
PwmOut pwm1(D6);          // JP1a-3 PWM1
PwmOut pwm2(D9);          // JP1a-6 PWM2
DigitalOut bipolar1(D2);  // JP1a-1 Bipolar1 (set for unipolar)
DigitalOut bipolar2(D3);  // JP1a-4 Bipolar2 (set for unipolar)
DigitalOut dir1(D4);      // JP1a-2 Direction1
DigitalOut dir2(D7);      // JP1a-5 Direction2
DigitalOut enable(D8);    // JP1a-7 Enable

// Potentiometer (Arduino A0)
AnalogIn pot(A0);

// ----- PWM settings -----
static const float PWM_FREQ_HZ  = 20000.0f;
static const float PWM_PERIOD_S = 1.0f / PWM_FREQ_HZ;

static inline float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// Motor board PWM is negative logic: low duty -> fast (handbook)
// So write (1 - speed)
static void write_speed_inverted(PwmOut &pwm, float speed01) {
    speed01 = clamp01(speed01);
    pwm.write(1.0f - speed01);
}

static void set_motor_unipolar(int motor, float speedSigned) {
    int forward = (speedSigned >= 0.0f) ? 1 : 0;
    float mag = (speedSigned >= 0.0f) ? speedSigned : -speedSigned;
    mag = clamp01(mag);

    if (motor == 1) {
        dir1 = forward;
        write_speed_inverted(pwm1, mag);
    } else {
        dir2 = forward;
        write_speed_inverted(pwm2, mag);
    }
}

int main() {
    enable = 0;

    pwm1.period(PWM_PERIOD_S);
    pwm2.period(PWM_PERIOD_S);

    // stop
    write_speed_inverted(pwm1, 0.0f);
    write_speed_inverted(pwm2, 0.0f);

    // UNIPOLAR select (per handbook usage)
    bipolar1 = 0;
    bipolar2 = 0;

    // forward default
    dir1 = 1;
    dir2 = 1;

    enable = 1;

    // simple smoothing to reduce jitter
    float speed_filt = 0.0f;

    while (1) {
        float speed = pot.read();              // 0.0 ~ 1.0
        speed_filt = 0.85f * speed_filt + 0.15f * speed;

        // forward only, speed from pot
        set_motor_unipolar(1, +speed_filt);
        set_motor_unipolar(2, +speed_filt);

        wait(0.02f); // 50 Hz update
    }
}