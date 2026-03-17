#include "addons/multi_channel_adc.h"
#include "storagemanager.h"
#include "drivermanager.h"
#include "helper.h"
#include "config.pb.h"

#include "hardware/adc.h"

#include <algorithm>
#include <cmath>

bool MultiChannelADCInput::available() {
    return Storage::getInstance().getAddonOptions().multiChannelADCOptions.enabled;
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
    const MultiChannelADCOptions& options = Storage::getInstance().getAddonOptions().multiChannelADCOptions;

    adc_init();

    smoothing_factor = options.smoothingFactor / 1000.0f;
    deadzone = options.deadzone / 100.0f;
    oversampling = options.oversampling;

    initChannel(steerLeft,  options.steerLeftPin,
                options.steerLeftRest,  options.steerLeftActive);
    initChannel(steerRight, options.steerRightPin,
                options.steerRightRest, options.steerRightActive);
    initChannel(throttle,   options.throttlePin,
                options.throttleRest,   options.throttleActive);
    initChannel(brake,      options.brakePin,
                options.brakeRest,      options.brakeActive);

    if (options.autoCalibrate) {
        if (steerLeft.enabled)
            steerLeft.rest_value = readADCOversampled(steerLeft.adc_input, 16);
        if (steerRight.enabled)
            steerRight.rest_value = readADCOversampled(steerRight.adc_input, 16);
        if (throttle.enabled)
            throttle.rest_value = readADCOversampled(throttle.adc_input, 16);
        if (brake.enabled)
            brake.rest_value = readADCOversampled(brake.adc_input, 16);
    }
}

static void processChannel(hall_channel_t &ch, MultiChannelADCInput *self,
                            uint8_t oversampling, float smoothing_factor, float deadzone) {
    if (!ch.enabled) return;

    ch.raw_value = self->readADCOversampled(ch.adc_input, oversampling);
    ch.activation = self->computeActivation(ch);
    ch.activation = self->applyEMA(ch.activation, ch.ema_value, smoothing_factor);
    ch.ema_value = ch.activation;
    ch.activation = self->applyDeadzone(ch.activation, deadzone);
}

void MultiChannelADCInput::process() {
    Gamepad *gamepad = Storage::getInstance().GetGamepad();

    if (throttle.enabled || brake.enabled) {
        gamepad->hasAnalogTriggers = true;
    }

    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    if (DriverManager::getInstance().getDriver() != nullptr) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
    }

    processChannel(steerLeft, this, oversampling, smoothing_factor, deadzone);
    processChannel(steerRight, this, oversampling, smoothing_factor, deadzone);
    processChannel(throttle, this, oversampling, smoothing_factor, deadzone);
    processChannel(brake, this, oversampling, smoothing_factor, deadzone);

    // Steering: combine left + right into Left Stick X axis
    float steerLeftAct  = steerLeft.enabled  ? steerLeft.activation  : 0.0f;
    float steerRightAct = steerRight.enabled ? steerRight.activation : 0.0f;

    float netSteering = steerRightAct - steerLeftAct;
    netSteering = std::clamp(netSteering, -1.0f, 1.0f);

    int32_t steerOutput = static_cast<int32_t>(joystickMid) +
                          static_cast<int32_t>(netSteering * joystickMid);
    gamepad->state.lx = static_cast<uint16_t>(
        std::clamp(steerOutput, (int32_t)0, (int32_t)0xFFFF));

    // Throttle → Right Trigger (RT, 0-255)
    float throttleAct = throttle.enabled ? throttle.activation : 0.0f;
    gamepad->state.rt = static_cast<uint8_t>(
        std::clamp(throttleAct * 255.0f, 0.0f, 255.0f));

    // Brake → Left Trigger (LT, 0-255)
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
