#ifndef _MultiChannelADC_H
#define _MultiChannelADC_H

#include "gpaddon.h"
#include "GamepadEnums.h"
#include "BoardConfig.h"
#include "enums.pb.h"
#include "types.h"

#ifndef MULTI_CHANNEL_ADC_ENABLED
#define MULTI_CHANNEL_ADC_ENABLED 0
#endif

// RP2040 ADC: 12-bit, 4 external channels (GPIO 26-29)
#define MULTI_ADC_PIN_OFFSET 26
#define MULTI_ADC_MAX_VALUE ((1 << 12) - 1)

// ============================================================
// Pin assignments: 4 hall-effect linear keys
// ============================================================
// ADC0 (GPIO26): Steering LEFT key
#ifndef MULTI_ADC_STEER_LEFT_PIN
#define MULTI_ADC_STEER_LEFT_PIN -1
#endif

// ADC1 (GPIO27): Steering RIGHT key
#ifndef MULTI_ADC_STEER_RIGHT_PIN
#define MULTI_ADC_STEER_RIGHT_PIN -1
#endif

// ADC2 (GPIO28): Throttle key
#ifndef MULTI_ADC_THROTTLE_PIN
#define MULTI_ADC_THROTTLE_PIN -1
#endif

// ADC3 (GPIO29): Brake key
#ifndef MULTI_ADC_BRAKE_PIN
#define MULTI_ADC_BRAKE_PIN -1
#endif

// ============================================================
// Hall sensor calibration defaults
//
// Hall linear keys: ADC reads a "rest" value when unpressed,
// and shifts toward "active" value when fully pressed.
// Activation = |current - rest| / |active - rest|
//
// These defaults assume the ADC value INCREASES when pressed.
// Adjust per your actual hardware measurement.
// ============================================================
#ifndef MULTI_ADC_STEER_LEFT_REST
#define MULTI_ADC_STEER_LEFT_REST 512
#endif
#ifndef MULTI_ADC_STEER_LEFT_ACTIVE
#define MULTI_ADC_STEER_LEFT_ACTIVE 3584
#endif

#ifndef MULTI_ADC_STEER_RIGHT_REST
#define MULTI_ADC_STEER_RIGHT_REST 512
#endif
#ifndef MULTI_ADC_STEER_RIGHT_ACTIVE
#define MULTI_ADC_STEER_RIGHT_ACTIVE 3584
#endif

#ifndef MULTI_ADC_THROTTLE_REST
#define MULTI_ADC_THROTTLE_REST 512
#endif
#ifndef MULTI_ADC_THROTTLE_ACTIVE
#define MULTI_ADC_THROTTLE_ACTIVE 3584
#endif

#ifndef MULTI_ADC_BRAKE_REST
#define MULTI_ADC_BRAKE_REST 512
#endif
#ifndef MULTI_ADC_BRAKE_ACTIVE
#define MULTI_ADC_BRAKE_ACTIVE 3584
#endif

// ============================================================
// Signal processing
// ============================================================
#ifndef MULTI_ADC_DEADZONE
#define MULTI_ADC_DEADZONE 3
#endif

#ifndef MULTI_ADC_SMOOTHING_FACTOR
#define MULTI_ADC_SMOOTHING_FACTOR 100
#endif

#ifndef MULTI_ADC_OVERSAMPLING
#define MULTI_ADC_OVERSAMPLING 4
#endif

#ifndef MULTI_ADC_AUTO_CALIBRATE_REST
#define MULTI_ADC_AUTO_CALIBRATE_REST 1
#endif

#define MultiChannelADCName "MultiChannelADC"

typedef struct {
    Pin_t gpio_pin;
    uint8_t adc_input;
    bool enabled;

    uint16_t rest_value;
    uint16_t active_value;

    uint16_t raw_value;
    float activation;
    float ema_value;
} hall_channel_t;

class MultiChannelADCInput : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void process();
    virtual void preprocess() {}
    virtual void postprocess(bool sent) {}
    virtual void reinit();
    virtual std::string name() { return MultiChannelADCName; }

private:
    uint16_t readADCOversampled(uint8_t adc_input, uint8_t samples);
    float computeActivation(hall_channel_t &ch);
    float applyEMA(float current, float previous, float factor);
    float applyDeadzone(float value, float deadzone);

    hall_channel_t steerLeft;
    hall_channel_t steerRight;
    hall_channel_t throttle;
    hall_channel_t brake;

    float smoothing_factor;
    float deadzone;
    uint8_t oversampling;
};

#endif
