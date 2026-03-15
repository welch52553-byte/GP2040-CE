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

// RP2040 has 4 external ADC channels: ADC0 (GPIO26) ~ ADC3 (GPIO29)
// plus ADC4 for internal temperature sensor
#define MULTI_ADC_MAX_CHANNELS 4
#define MULTI_ADC_PIN_OFFSET 26
#define MULTI_ADC_RESOLUTION_BITS 12
#define MULTI_ADC_MAX_VALUE ((1 << MULTI_ADC_RESOLUTION_BITS) - 1)

#ifndef MULTI_ADC_CH0_PIN
#define MULTI_ADC_CH0_PIN -1
#endif

#ifndef MULTI_ADC_CH1_PIN
#define MULTI_ADC_CH1_PIN -1
#endif

#ifndef MULTI_ADC_CH2_PIN
#define MULTI_ADC_CH2_PIN -1
#endif

#ifndef MULTI_ADC_CH3_PIN
#define MULTI_ADC_CH3_PIN -1
#endif

#ifndef MULTI_ADC_TEMP_ENABLED
#define MULTI_ADC_TEMP_ENABLED 0
#endif

#ifndef MULTI_ADC_SMOOTHING_ENABLED
#define MULTI_ADC_SMOOTHING_ENABLED 1
#endif

#ifndef MULTI_ADC_SMOOTHING_FACTOR
#define MULTI_ADC_SMOOTHING_FACTOR 50
#endif

#ifndef MULTI_ADC_DEADZONE_INNER
#define MULTI_ADC_DEADZONE_INNER 5
#endif

#ifndef MULTI_ADC_DEADZONE_OUTER
#define MULTI_ADC_DEADZONE_OUTER 95
#endif

#ifndef MULTI_ADC_AUTO_CALIBRATE
#define MULTI_ADC_AUTO_CALIBRATE 1
#endif

#ifndef MULTI_ADC_SAMPLE_RATE_US
#define MULTI_ADC_SAMPLE_RATE_US 100
#endif

#ifndef MULTI_ADC_OVERSAMPLING
#define MULTI_ADC_OVERSAMPLING 4
#endif

enum class ADCChannelMapping : uint8_t {
    NONE = 0,
    LEFT_STICK_X,
    LEFT_STICK_Y,
    RIGHT_STICK_X,
    RIGHT_STICK_Y,
    LEFT_TRIGGER,
    RIGHT_TRIGGER,
    RAW_OUTPUT,
};

#ifndef MULTI_ADC_CH0_MAPPING
#define MULTI_ADC_CH0_MAPPING ADCChannelMapping::LEFT_STICK_X
#endif

#ifndef MULTI_ADC_CH1_MAPPING
#define MULTI_ADC_CH1_MAPPING ADCChannelMapping::LEFT_STICK_Y
#endif

#ifndef MULTI_ADC_CH2_MAPPING
#define MULTI_ADC_CH2_MAPPING ADCChannelMapping::RIGHT_STICK_X
#endif

#ifndef MULTI_ADC_CH3_MAPPING
#define MULTI_ADC_CH3_MAPPING ADCChannelMapping::RIGHT_STICK_Y
#endif

#define MultiChannelADCName "MultiChannelADC"

typedef struct {
    Pin_t gpio_pin;
    uint8_t adc_input;
    ADCChannelMapping mapping;
    bool enabled;

    uint16_t raw_value;
    float normalized_value;
    uint16_t center_value;
    float ema_value;

    bool auto_calibrate;
    uint16_t cal_min;
    uint16_t cal_max;
    uint16_t cal_center;
} adc_channel_t;

typedef struct {
    float temperature_c;
    bool enabled;
} adc_temp_sensor_t;

class MultiChannelADCInput : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void process();
    virtual void preprocess() {}
    virtual void postprocess(bool sent) {}
    virtual void reinit();
    virtual std::string name() { return MultiChannelADCName; }

    uint16_t getRawValue(uint8_t channel);
    float getNormalizedValue(uint8_t channel);
    float getTemperature();
    uint8_t getActiveChannelCount();

private:
    uint16_t readADCChannel(uint8_t adc_input);
    uint16_t readADCOversampled(uint8_t adc_input, uint8_t samples);
    float normalizeValue(uint16_t raw, uint16_t center, uint16_t cal_min, uint16_t cal_max);
    float applyEMA(float current, float previous, float factor);
    float applyDeadzone(float value, float inner, float outer);

    adc_channel_t channels[MULTI_ADC_MAX_CHANNELS];
    adc_temp_sensor_t temp_sensor;

    uint8_t active_channels;
    float smoothing_factor;
    float inner_deadzone;
    float outer_deadzone;
    uint8_t oversampling;
    uint32_t last_sample_time;
    uint32_t sample_interval_us;
};

#endif
