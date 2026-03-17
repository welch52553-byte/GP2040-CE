#include "addons/multi_channel_adc.h"
#include "storagemanager.h"
#include "drivermanager.h"
#include "helper.h"

#include "hardware/adc.h"

#include <algorithm>
#include <cmath>

bool MultiChannelADCInput::available() {
    return MULTI_CHANNEL_ADC_ENABLED;
}

static void initChannel(hall_channel_t &ch, Pin_t pin, uint16_t rest, uint16_t active) {
    ch.gpio_pin = pin;
    ch.rest_value = rest;
    ch.active_value = active;
    ch.enabled = false;
    ch.raw_value = 0;
    ch.activation = 0.0f;
    ch.ema_value = 0.0f;

    if (isValidPin(pin) && pin >= MULTI_ADC_PIN_OFFSET &&
        pin <= (MULTI_ADC_PIN_OFFSET + 3)) {
        ch.adc_input = pin - MULTI_ADC_PIN_OFFSET;
        adc_gpio_init(pin);
        ch.enabled = true;
    }
}

void MultiChannelADCInput::setup() {
    adc_init();

    smoothing_factor = MULTI_ADC_SMOOTHING_FACTOR / 1000.0f;
    deadzone = MULTI_ADC_DEADZONE / 100.0f;
    oversampling = MULTI_ADC_OVERSAMPLING;

    initChannel(steerLeft,  MULTI_ADC_STEER_LEFT_PIN,
                MULTI_ADC_STEER_LEFT_REST,  MULTI_ADC_STEER_LEFT_ACTIVE);
    initChannel(steerRight, MULTI_ADC_STEER_RIGHT_PIN,
                MULTI_ADC_STEER_RIGHT_REST, MULTI_ADC_STEER_RIGHT_ACTIVE);
    initChannel(throttle,   MULTI_ADC_THROTTLE_PIN,
                MULTI_ADC_THROTTLE_REST,    MULTI_ADC_THROTTLE_ACTIVE);
    initChannel(brake,      MULTI_ADC_BRAKE_PIN,
                MULTI_ADC_BRAKE_REST,       MULTI_ADC_BRAKE_ACTIVE);

    if (MULTI_ADC_AUTO_CALIBRATE_REST) {
        if (steerLeft.enabled) {
            steerLeft.rest_value = readADCOversampled(steerLeft.adc_input, 16);
        }
        if (steerRight.enabled) {
            steerRight.rest_value = readADCOversampled(steerRight.adc_input, 16);
        }
        if (throttle.enabled) {
            throttle.rest_value = readADCOversampled(throttle.adc_input, 16);
        }
        if (brake.enabled) {
            brake.rest_value = readADCOversampled(brake.adc_input, 16);
        }
    }
}

void MultiChannelADCInput::process() {
    Gamepad *gamepad = Storage::getInstance().GetGamepad();

    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    if (DriverManager::getInstance().getDriver() != nullptr) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
    }

    // --- Read and process each channel ---

    if (steerLeft.enabled) {
        steerLeft.raw_value = readADCOversampled(steerLeft.adc_input, oversampling);
        steerLeft.activation = computeActivation(steerLeft);
        steerLeft.activation = applyEMA(steerLeft.activation, steerLeft.ema_value, smoothing_factor);
        steerLeft.ema_value = steerLeft.activation;
        steerLeft.activation = applyDeadzone(steerLeft.activation, deadzone);
    }

    if (steerRight.enabled) {
        steerRight.raw_value = readADCOversampled(steerRight.adc_input, oversampling);
        steerRight.activation = computeActivation(steerRight);
        steerRight.activation = applyEMA(steerRight.activation, steerRight.ema_value, smoothing_factor);
        steerRight.ema_value = steerRight.activation;
        steerRight.activation = applyDeadzone(steerRight.activation, deadzone);
    }

    if (throttle.enabled) {
        throttle.raw_value = readADCOversampled(throttle.adc_input, oversampling);
        throttle.activation = computeActivation(throttle);
        throttle.activation = applyEMA(throttle.activation, throttle.ema_value, smoothing_factor);
        throttle.ema_value = throttle.activation;
        throttle.activation = applyDeadzone(throttle.activation, deadzone);
    }

    if (brake.enabled) {
        brake.raw_value = readADCOversampled(brake.adc_input, oversampling);
        brake.activation = computeActivation(brake);
        brake.activation = applyEMA(brake.activation, brake.ema_value, smoothing_factor);
        brake.ema_value = brake.activation;
        brake.activation = applyDeadzone(brake.activation, deadzone);
    }

    // --- Steering: combine left + right into Left Stick X axis ---
    // Left key pressed  → X axis moves toward 0x0000 (left)
    // Right key pressed → X axis moves toward 0xFFFF (right)
    // Both at rest      → X axis = center (0x7FFF)
    float steerLeftAct  = steerLeft.enabled  ? steerLeft.activation  : 0.0f;
    float steerRightAct = steerRight.enabled ? steerRight.activation : 0.0f;

    // net steering: -1.0 (full left) to +1.0 (full right)
    float netSteering = steerRightAct - steerLeftAct;
    netSteering = std::clamp(netSteering, -1.0f, 1.0f);

    // map to joystick range: center + net * half_range
    int32_t steerOutput = static_cast<int32_t>(joystickMid) +
                          static_cast<int32_t>(netSteering * joystickMid);
    gamepad->state.lx = static_cast<uint16_t>(
        std::clamp(steerOutput, (int32_t)0, (int32_t)0xFFFF));

    // --- Throttle → Right Trigger (RT, 0-255) ---
    float throttleAct = throttle.enabled ? throttle.activation : 0.0f;
    gamepad->state.rt = static_cast<uint8_t>(
        std::clamp(throttleAct * 255.0f, 0.0f, 255.0f));

    // --- Brake → Left Trigger (LT, 0-255) ---
    float brakeAct = brake.enabled ? brake.activation : 0.0f;
    gamepad->state.lt = static_cast<uint8_t>(
        std::clamp(brakeAct * 255.0f, 0.0f, 255.0f));
}

void MultiChannelADCInput::reinit() {
    setup();
}

uint16_t MultiChannelADCInput::readADCOversampled(uint8_t adc_input, uint8_t samples) {
    adc_select_input(adc_input);
    if (samples <= 1) {
        return adc_read();
    }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += adc_read();
    }
    return static_cast<uint16_t>(sum / samples);
}

float MultiChannelADCInput::computeActivation(hall_channel_t &ch) {
    int32_t range = static_cast<int32_t>(ch.active_value) - static_cast<int32_t>(ch.rest_value);
    if (range == 0) return 0.0f;

    float activation = static_cast<float>(
        static_cast<int32_t>(ch.raw_value) - static_cast<int32_t>(ch.rest_value)
    ) / static_cast<float>(range);

    return std::clamp(activation, 0.0f, 1.0f);
}

float MultiChannelADCInput::applyEMA(float current, float previous, float factor) {
    return (factor * current) + ((1.0f - factor) * previous);
}

float MultiChannelADCInput::applyDeadzone(float value, float dz) {
    if (value < dz) return 0.0f;
    if (dz >= 1.0f) return value;
    return (value - dz) / (1.0f - dz);
}
