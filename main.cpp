#include "mbed.h"
#include "C12832.h"
#include "QEI.h"

// LCD (Application Shield fixed pins)
C12832 lcd(D11, D13, D12, D7, D10);

// Two potentiometers (raw 0.0 ~ 1.0)
AnalogIn pot1(A0);
AnalogIn pot2(A1);

// Motor driver (JP1a) on Morpho pins
PwmOut     pwm1(PC_8);     // PWM1 -> PC8 (JP1a pin 3)
PwmOut     pwm2(PC_9);     // PWM2 -> PC9 (JP1a pin 6)
DigitalOut dir1(PA_12);    // DIR1 -> PA12 (ignored in bipolar)
DigitalOut dir2(PC_6);     // DIR2 -> PC6 (ignored in bipolar)
DigitalOut enable(PC_5);   // ENABLE -> PC5 (JP1a pin 7)
DigitalOut bipolar1(PB_12);// BIPOLAR1 -> PB12 (JP1a pin 1)
DigitalOut bipolar2(PB_13);// BIPOLAR2 -> PB13 (JP1a pin 4)

// Encoders (Morpho)
// Left A/B  -> PB14 / PB15
// Right A/B -> PC10 / PC12
// Index not used -> NC
static const int ENC_PULSES_PER_REV = 256;
QEI encL(PB_14, PB_15, NC, ENC_PULSES_PER_REV, QEI::X4_ENCODING);
QEI encR(PC_10, PC_12, NC, ENC_PULSES_PER_REV, QEI::X4_ENCODING);

static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// Convert pot reading into bipolar effective duty (0..1), 0.5 = stop.
// Then invert for the board because PWM input is negative logic.
static void pot_to_bipolar_pwm(float pot,
                               float &effective_duty, float &pwm_out,
                               char &dir_char, float &speed_mag) {
    pot = clamp01(pot);

    const float dead_lo = 0.4f;
    const float dead_hi = 0.6f;

    if (pot >= dead_lo && pot <= dead_hi) {
        dir_char = 'S';
        speed_mag = 0.0f;
        effective_duty = 0.5f;
    } else if (pot < dead_lo) {
        dir_char = 'B';
        speed_mag = clamp01((dead_lo - pot) / dead_lo);
        effective_duty = 0.5f - 0.5f * speed_mag;
    } else {
        dir_char = 'F';
        speed_mag = clamp01((pot - dead_hi) / (1.0f - dead_hi));
        effective_duty = 0.5f + 0.5f * speed_mag;
    }

    pwm_out = 1.0f - effective_duty;
}

int main() {
    lcd.cls();

    // Keep disabled while configuring for safe startup.
    enable = 0;

    // Force bipolar mode for both channels.
    bipolar1 = 1;
    bipolar2 = 1;

    // Direction pins are ignored in bipolar mode.
    dir1 = 0;
    dir2 = 0;

    // PWM frequency = 20kHz
    pwm1.period_us(50);
    pwm2.period_us(50);

    // Stop in bipolar mode: effective duty 0.5 -> pwm_out 0.5
    pwm1.write(0.5f);
    pwm2.write(0.5f);

    // Reset encoder counts at start.
    encL.reset();
    encR.reset();

    enable = 1;

    int32_t prevL = (int32_t)encL.getPulses();
    int32_t prevR = (int32_t)encR.getPulses();

    while (1) {
        float r1 = pot1.read();
        float r2 = pot2.read();

        float duty1, out1, mag1;
        float duty2, out2, mag2;
        char d1, d2;

        pot_to_bipolar_pwm(r1, duty1, out1, d1, mag1);
        pot_to_bipolar_pwm(r2, duty2, out2, d2, mag2);

        pwm1.write(out1);
        pwm2.write(out2);

        int32_t L = (int32_t)encL.getPulses();
        int32_t R = (int32_t)encR.getPulses();

        int32_t dL = L - prevL;
        int32_t dR = R - prevR;
        prevL = L;
        prevR = R;

        lcd.cls();
        lcd.locate(0, 0);
        lcd.printf("P1:%1.3f %c%1.2f", r1, d1, mag1);

        lcd.locate(0, 8);
        lcd.printf("L:%ld d:%ld", (long)L, (long)dL);

        lcd.locate(0, 16);
        lcd.printf("P2:%1.3f %c%1.2f", r2, d2, mag2);

        lcd.locate(0, 24);
        lcd.printf("R:%ld d:%ld", (long)R, (long)dR);

        wait_ms(100);
    }
}
