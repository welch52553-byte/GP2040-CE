#include "addons/multi_channel_adc.h"
#include "storagemanager.h"
#include "drivermanager.h"
#include "helper.h"

#include "hardware/adc.h"
#include "pico/time.h"

#include <algorithm>
#include <cmath>

bool MultiChannelADCInput::available() {
    return MULTI_CHANNEL_ADC_ENABLED;
}

void MultiChannelADCInput::setup() {
    adc_init();

    active_channels = 0;
    smoothing_factor = MULTI_ADC_SMOOTHING_FACTOR / 1000.0f;
    inner_deadzone = MULTI_ADC_DEADZONE_INNER / 100.0f;
    outer_deadzone = MULTI_ADC_DEADZONE_OUTER / 100.0f;
    oversampling = MULTI_ADC_OVERSAMPLING;
    sample_interval_us = MULTI_ADC_SAMPLE_RATE_US;
    last_sample_time = 0;

    Pin_t pin_config[MULTI_ADC_MAX_CHANNELS] = {
        MULTI_ADC_CH0_PIN,
        MULTI_ADC_CH1_PIN,
        MULTI_ADC_CH2_PIN,
        MULTI_ADC_CH3_PIN,
    };

    ADCChannelMapping mapping_config[MULTI_ADC_MAX_CHANNELS] = {
        MULTI_ADC_CH0_MAPPING,
        MULTI_ADC_CH1_MAPPING,
        MULTI_ADC_CH2_MAPPING,
        MULTI_ADC_CH3_MAPPING,
    };

    for (uint8_t i = 0; i < MULTI_ADC_MAX_CHANNELS; i++) {
        channels[i].gpio_pin = pin_config[i];
        channels[i].mapping = mapping_config[i];
        channels[i].enabled = false;
        channels[i].raw_value = 0;
        channels[i].normalized_value = 0.5f;
        channels[i].center_value = MULTI_ADC_MAX_VALUE / 2;
        channels[i].ema_value = 0.5f;
        channels[i].auto_calibrate = MULTI_ADC_AUTO_CALIBRATE;
        channels[i].cal_min = 0;
        channels[i].cal_max = MULTI_ADC_MAX_VALUE;
        channels[i].cal_center = MULTI_ADC_MAX_VALUE / 2;

        if (isValidPin(channels[i].gpio_pin) &&
            channels[i].gpio_pin >= MULTI_ADC_PIN_OFFSET &&
            channels[i].gpio_pin <= (MULTI_ADC_PIN_OFFSET + MULTI_ADC_MAX_CHANNELS - 1)) {

            channels[i].adc_input = channels[i].gpio_pin - MULTI_ADC_PIN_OFFSET;
            adc_gpio_init(channels[i].gpio_pin);
            channels[i].enabled = true;
            active_channels++;

            if (channels[i].auto_calibrate) {
                channels[i].center_value = readADCOversampled(channels[i].adc_input, 16);
                channels[i].cal_center = channels[i].center_value;
            }
        }
    }

    temp_sensor.enabled = MULTI_ADC_TEMP_ENABLED;
    temp_sensor.temperature_c = 0.0f;
    if (temp_sensor.enabled) {
        adc_set_temp_sensor_enabled(true);
    }
}

void MultiChannelADCInput::process() {
    uint32_t now = time_us_32();
    if (sample_interval_us > 0 && (now - last_sample_time) < sample_interval_us) {
        return;
    }
    last_sample_time = now;

    Gamepad *gamepad = Storage::getInstance().GetGamepad();

    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    uint32_t joystickMax = GAMEPAD_JOYSTICK_MAX;
    if (DriverManager::getInstance().getDriver() != nullptr) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
        joystickMax = joystickMid * 2;
    }

    for (uint8_t i = 0; i < MULTI_ADC_MAX_CHANNELS; i++) {
        if (!channels[i].enabled) continue;

        uint16_t raw = readADCOversampled(channels[i].adc_input, oversampling);
        channels[i].raw_value = raw;

        if (channels[i].auto_calibrate) {
            if (raw < channels[i].cal_min) channels[i].cal_min = raw;
            if (raw > channels[i].cal_max) channels[i].cal_max = raw;
        }

        float normalized = normalizeValue(
            raw,
            channels[i].cal_center,
            channels[i].cal_min,
            channels[i].cal_max
        );

        if (MULTI_ADC_SMOOTHING_ENABLED) {
            normalized = applyEMA(normalized, channels[i].ema_value, smoothing_factor);
            channels[i].ema_value = normalized;
        }

        normalized = applyDeadzone(normalized, inner_deadzone, outer_deadzone);
        channels[i].normalized_value = std::clamp(normalized, 0.0f, 1.0f);

        uint16_t output = static_cast<uint16_t>(
            std::min(
                static_cast<uint32_t>(joystickMax * channels[i].normalized_value),
                static_cast<uint32_t>(0xFFFF)
            )
        );

        switch (channels[i].mapping) {
            case ADCChannelMapping::LEFT_STICK_X:
                gamepad->state.lx = output;
                break;
            case ADCChannelMapping::LEFT_STICK_Y:
                gamepad->state.ly = output;
                break;
            case ADCChannelMapping::RIGHT_STICK_X:
                gamepad->state.rx = output;
                break;
            case ADCChannelMapping::RIGHT_STICK_Y:
                gamepad->state.ry = output;
                break;
            case ADCChannelMapping::LEFT_TRIGGER:
                gamepad->state.lt = static_cast<uint8_t>(
                    channels[i].normalized_value * 255.0f
                );
                break;
            case ADCChannelMapping::RIGHT_TRIGGER:
                gamepad->state.rt = static_cast<uint8_t>(
                    channels[i].normalized_value * 255.0f
                );
                break;
            case ADCChannelMapping::RAW_OUTPUT:
            case ADCChannelMapping::NONE:
            default:
                break;
        }
    }

    if (temp_sensor.enabled) {
        adc_select_input(4);
        uint16_t raw_temp = adc_read();
        float voltage = raw_temp * 3.3f / MULTI_ADC_MAX_VALUE;
        temp_sensor.temperature_c = 27.0f - (voltage - 0.706f) / 0.001721f;
    }
}

void MultiChannelADCInput::reinit() {
    setup();
}

uint16_t MultiChannelADCInput::readADCChannel(uint8_t adc_input) {
    adc_select_input(adc_input);
    return adc_read();
}

uint16_t MultiChannelADCInput::readADCOversampled(uint8_t adc_input, uint8_t samples) {
    if (samples <= 1) {
        return readADCChannel(adc_input);
    }

    uint32_t sum = 0;
    adc_select_input(adc_input);
    for (uint8_t s = 0; s < samples; s++) {
        sum += adc_read();
    }
    return static_cast<uint16_t>(sum / samples);
}

float MultiChannelADCInput::normalizeValue(uint16_t raw, uint16_t center,
                                            uint16_t cal_min, uint16_t cal_max) {
    if (cal_max <= cal_min) return 0.5f;

    if (raw >= center) {
        if (cal_max <= center) return 0.5f;
        return 0.5f + 0.5f * static_cast<float>(raw - center) / static_cast<float>(cal_max - center);
    } else {
        if (center <= cal_min) return 0.5f;
        return 0.5f - 0.5f * static_cast<float>(center - raw) / static_cast<float>(center - cal_min);
    }
}

float MultiChannelADCInput::applyEMA(float current, float previous, float factor) {
    return (factor * current) + ((1.0f - factor) * previous);
}

float MultiChannelADCInput::applyDeadzone(float value, float inner, float outer) {
    float centered = value - 0.5f;
    float magnitude = std::fabs(centered);

    if (magnitude < inner * 0.5f) {
        return 0.5f;
    }

    float half_outer = outer * 0.5f;
    float half_inner = inner * 0.5f;
    if (half_outer <= half_inner) return value;

    float scaled = (magnitude - half_inner) / (half_outer - half_inner);
    scaled = std::clamp(scaled, 0.0f, 0.5f);

    return 0.5f + (centered > 0 ? scaled : -scaled);
}

uint16_t MultiChannelADCInput::getRawValue(uint8_t channel) {
    if (channel >= MULTI_ADC_MAX_CHANNELS || !channels[channel].enabled)
        return 0;
    return channels[channel].raw_value;
}

float MultiChannelADCInput::getNormalizedValue(uint8_t channel) {
    if (channel >= MULTI_ADC_MAX_CHANNELS || !channels[channel].enabled)
        return 0.5f;
    return channels[channel].normalized_value;
}

float MultiChannelADCInput::getTemperature() {
    return temp_sensor.temperature_c;
}

uint8_t MultiChannelADCInput::getActiveChannelCount() {
    return active_channels;
}
