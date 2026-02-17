#include "mbed.h"

// EDIT THESE PINS TO MATCH YOUR WIRING (JP1a -> Nucleo)

// PWM outputs (must be PWM-capable pins)
PwmOut pwm1(D6);   // -> JP1a pin 3  (PWM 1)
PwmOut pwm2(D9);   // -> JP1a pin 6  (PWM 2)

// Mode select (bipolar/unipolar)
DigitalOut bipolar1(D2); // -> JP1a pin 1 (Bipolar 1)
DigitalOut bipolar2(D3); // -> JP1a pin 4 (Bipolar 2)

// Direction (only used in unipolar mode)
DigitalOut dir1(D4);     // -> JP1a pin 2 (Direction 1)
DigitalOut dir2(D7);     // -> JP1a pin 5 (Direction 2)

// Enable (global enable for the driver)
DigitalOut enable(D8);   // -> JP1a pin 7 (Enable)

// ------------------------
// PWM settings
// ------------------------
static const float PWM_FREQ_HZ = 20000.0f;          // 20 kHz (above audible range)
static const float PWM_PERIOD_S = 1.0f / PWM_FREQ_HZ;

static inline float clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

/*
  Motor board PWM is negative logic (handbook):
  - "low duty cycle makes the motor run fast"
  So we invert:
    speed 0.0 (stop) -> output HIGH all the time -> pwm.write(1.0)
    speed 1.0 (full) -> output LOW  all the time -> pwm.write(0.0)
*/
static void write_speed_inverted(PwmOut &pwm, float speed01)
{
    speed01 = clamp01(speed01);
    pwm.write(1.0f - speed01);
}

/*
  Command motor in UNIPOLAR mode with signed speed:
    speedSigned in [-1.0, +1.0]
    sign -> direction pin
    magnitude -> PWM
  If direction is wrong on your hardware, just flip the dir assignment below.
*/
static void set_motor_unipolar(int motor, float speedSigned)
{
    int forward = (speedSigned >= 0.0f) ? 1 : 0;
    float mag = (speedSigned >= 0.0f) ? speedSigned : -speedSigned;
    mag = clamp01(mag);

    if (motor == 1) {
        dir1 = forward;                 // change to (!forward) if direction is reversed
        write_speed_inverted(pwm1, mag);
    } else {
        dir2 = forward;                 // change to (!forward) if direction is reversed
        write_speed_inverted(pwm2, mag);
    }
}

int main()
{
    // Start safe: disable driver and command stop
    enable = 0;

    // Set PWM frequency
    pwm1.period(PWM_PERIOD_S);
    pwm2.period(PWM_PERIOD_S);

    // Stop motors (in negative logic: write 1.0)
    write_speed_inverted(pwm1, 0.0f);
    write_speed_inverted(pwm2, 0.0f);

    // Select UNIPOLAR mode (handbook indicates bipolar pins tied LOW for unipolar)
    bipolar1 = 0;
    bipolar2 = 0;

    // Choose an initial direction
    dir1 = 1;
    dir2 = 1;

    // Enable the driver
    enable = 1;

    while (1) {
        // Ramp forward 0 -> 100%
        for (float s = 0.0f; s <= 1.0001f; s += 0.05f) {
            set_motor_unipolar(1, +s);
            set_motor_unipolar(2, +s);
            wait(0.10f);
        }
        wait(0.5f);

        // Ramp down 100% -> 0
        for (float s = 1.0f; s >= -0.0001f; s -= 0.05f) {
            set_motor_unipolar(1, +s);
            set_motor_unipolar(2, +s);
            wait(0.10f);
        }
        wait(0.5f);

        // Ramp reverse 0 -> 60% (gentler than full reverse)
        for (float s = 0.0f; s <= 0.6001f; s += 0.05f) {
            set_motor_unipolar(1, -s);
            set_motor_unipolar(2, -s);
            wait(0.10f);
        }
        wait(0.5f);

        // Stop again
        set_motor_unipolar(1, 0.0f);
        set_motor_unipolar(2, 0.0f);
        wait(1.0f);
    }
}
