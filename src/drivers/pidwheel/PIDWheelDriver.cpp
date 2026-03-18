#include "drivers/pidwheel/PIDWheelDriver.h"
#include "drivers/pidwheel/PIDWheelDescriptors.h"
#include "drivers/shared/driverhelper.h"
#include "storagemanager.h"
#include "drivermanager.h"

#include "tusb.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "device/usbd_pvt.h"

#include <cstring>
#include <algorithm>
#include <cmath>
#include "pico/time.h"

static bool pidwheel_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
    return hidd_control_xfer_cb(rhport, stage, request);
}

void PIDWheelDriver::initialize() {
    memset(&inputReport, 0, sizeof(inputReport));
    inputReport.report_id = HID_ID_INPUT;

    stateReport.report_id = HID_ID_STATE;
    stateReport.status = HID_ACTUATOR_POWER | HID_ENABLE_ACTUATORS;

    memset(last_report, 0, sizeof(last_report));
    memset(effects, 0, sizeof(effects));
    actuatorsEnabled = true;
    devicePaused = false;
    deviceGain = 255;
    lastPosition = 0;

    class_driver = {
    #if CFG_TUSB_DEBUG >= 2
        .name = "PIDWHEEL",
    #endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = pidwheel_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL
    };
}

bool PIDWheelDriver::process(Gamepad * gamepad) {
    // Map gamepad state to wheel axes (signed 16-bit, -32767..32767)
    inputReport.X = static_cast<int16_t>(gamepad->state.lx) + INT16_MIN;  // Steering
    inputReport.Y = static_cast<int16_t>(gamepad->state.rt * 128) - 16384; // Throttle
    inputReport.Z = static_cast<int16_t>(gamepad->state.lt * 128) - 16384; // Brake

    // Buttons
    uint32_t btns = 0
        | (gamepad->pressedB1() ? (1U << 0) : 0)
        | (gamepad->pressedB2() ? (1U << 1) : 0)
        | (gamepad->pressedB3() ? (1U << 2) : 0)
        | (gamepad->pressedB4() ? (1U << 3) : 0)
        | (gamepad->pressedL1() ? (1U << 4) : 0)
        | (gamepad->pressedR1() ? (1U << 5) : 0)
        | (gamepad->pressedS1() ? (1U << 6) : 0)
        | (gamepad->pressedS2() ? (1U << 7) : 0)
        | (gamepad->pressedL3() ? (1U << 8) : 0)
        | (gamepad->pressedR3() ? (1U << 9) : 0)
        | (gamepad->pressedA1() ? (1U << 10) : 0)
        | (gamepad->pressedA2() ? (1U << 11) : 0)
    ;
    inputReport.buttons = btns;

    // Calculate PID force and apply to haptics
    int16_t force = calculateForce();
    Gamepad * procGamepad = Storage::getInstance().GetProcessedGamepad();

    if (actuatorsEnabled && !devicePaused && force != 0) {
        uint16_t absForce = static_cast<uint16_t>(std::min(std::abs((int)force) >> 7, 255));
        if (force > 0) {
            procGamepad->auxState.haptics.leftActuator.enabled = true;
            procGamepad->auxState.haptics.leftActuator.active = true;
            procGamepad->auxState.haptics.leftActuator.intensity = absForce;
            procGamepad->auxState.haptics.rightActuator.active = false;
            procGamepad->auxState.haptics.rightActuator.intensity = 0;
        } else {
            procGamepad->auxState.haptics.rightActuator.enabled = true;
            procGamepad->auxState.haptics.rightActuator.active = true;
            procGamepad->auxState.haptics.rightActuator.intensity = absForce;
            procGamepad->auxState.haptics.leftActuator.active = false;
            procGamepad->auxState.haptics.leftActuator.intensity = 0;
        }
    } else {
        procGamepad->auxState.haptics.leftActuator.active = false;
        procGamepad->auxState.haptics.leftActuator.intensity = 0;
        procGamepad->auxState.haptics.rightActuator.active = false;
        procGamepad->auxState.haptics.rightActuator.intensity = 0;
    }

    // Update state report
    bool anyPlaying = false;
    for (int i = 0; i < PID_MAX_EFFECTS; i++) {
        if (effects[i].state & 0x01) { anyPlaying = true; break; }
    }
    stateReport.status = HID_ACTUATOR_POWER
        | (actuatorsEnabled ? HID_ENABLE_ACTUATORS : 0)
        | (devicePaused ? HID_EFFECT_PAUSE : 0)
        | (anyPlaying ? HID_EFFECT_PLAYING : 0);

    if (tud_suspended())
        tud_remote_wakeup();

    void * report = &inputReport;
    uint16_t report_size = sizeof(inputReport);
    if (memcmp(last_report, report, report_size) != 0) {
        if (tud_hid_ready() && tud_hid_report(HID_ID_INPUT, ((uint8_t*)report) + 1, report_size - 1)) {
            memcpy(last_report, report, report_size);
            return true;
        }
    }
    return false;
}

uint16_t PIDWheelDriver::get_report(uint8_t report_id, hid_report_type_t report_type,
                                     uint8_t *buffer, uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_INPUT) {
        if (report_id == HID_ID_STATE) {
            uint16_t len = std::min((uint16_t)sizeof(stateReport.status), reqlen);
            memcpy(buffer, &stateReport.status, len);
            return len;
        }
    }
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == HID_ID_POOLREP) {
            PIDWheelPoolReport pool = {};
            pool.ramPoolSize = PID_MAX_EFFECTS;
            pool.maxSimultaneousEffects = PID_MAX_EFFECTS;
            pool.memoryManagement = 0x03; // DeviceManaged + SharedParameterBlocks
            uint16_t len = std::min((uint16_t)sizeof(pool), reqlen);
            memcpy(buffer, &pool, len);
            return len;
        }
        if (report_id == HID_ID_BLKLDREP) {
            PIDWheelBlockLoad bl = {};
            bl.effectBlockIndex = allocateEffect();
            bl.loadStatus = (bl.effectBlockIndex > 0) ? 1 : 2; // 1=Success, 2=Full
            bl.ramPoolAvailable = PID_MAX_EFFECTS;
            uint16_t len = std::min((uint16_t)sizeof(bl), reqlen);
            memcpy(buffer, &bl, len);
            return len;
        }
        if (report_id == HID_ID_NEWEFREP) {
            PIDWheelCreateEffect ce = {};
            ce.effectType = 0;
            ce.byteCount = 0;
            uint16_t len = std::min((uint16_t)sizeof(ce), reqlen);
            memcpy(buffer, &ce, len);
            return len;
        }
    }
    return 0;
}

void PIDWheelDriver::set_report(uint8_t report_id, hid_report_type_t report_type,
                                 uint8_t const *buffer, uint16_t bufsize) {
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        switch (report_id) {
            case HID_ID_EFFREP:    handleSetEffect(buffer, bufsize); break;
            case HID_ID_ENVREP:    handleSetEnvelope(buffer, bufsize); break;
            case HID_ID_CONDREP:   handleSetCondition(buffer, bufsize); break;
            case HID_ID_PRIDREP:   handleSetPeriodic(buffer, bufsize); break;
            case HID_ID_CONSTREP:  handleSetConstantForce(buffer, bufsize); break;
            case HID_ID_RAMPREP:   handleSetRamp(buffer, bufsize); break;
            case HID_ID_EFOPREP:   handleEffectOperation(buffer, bufsize); break;
            case HID_ID_BLKFRREP:  handleBlockFree(buffer, bufsize); break;
            case HID_ID_CTRLREP:   handleDeviceControl(buffer, bufsize); break;
            case HID_ID_GAINREP:   handleDeviceGain(buffer, bufsize); break;
        }
    }
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == HID_ID_NEWEFREP && bufsize >= 1) {
            uint8_t effectType = buffer[0];
            uint8_t idx = allocateEffect();
            if (idx > 0 && idx <= PID_MAX_EFFECTS) {
                memset(&effects[idx - 1], 0, sizeof(PIDEffect));
                effects[idx - 1].type = effectType;
                effects[idx - 1].gain = 255;
                effects[idx - 1].duration = FFB_EFFECT_DURATION_INFINITE;
            }
        }
    }
}

void PIDWheelDriver::handleSetEffect(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetEffect)) return;
    auto *d = (const PIDWheelSetEffect*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    PIDEffect &e = effects[idx - 1];
    e.type = d->effectType;
    e.duration = d->duration;
    e.gain = d->gain;
    e.startDelay = d->startDelay;
}

void PIDWheelDriver::handleSetEnvelope(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetEnvelope)) return;
    auto *d = (const PIDWheelSetEnvelope*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    PIDEffect &e = effects[idx - 1];
    e.attackLevel = d->attackLevel;
    e.fadeLevel = d->fadeLevel;
    e.attackTime = d->attackTime;
    e.fadeTime = d->fadeTime;
    e.useEnvelope = true;
}

void PIDWheelDriver::handleSetCondition(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetCondition)) return;
    auto *d = (const PIDWheelSetCondition*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    PIDEffect &e = effects[idx - 1];
    e.condition.cpOffset = d->cpOffset;
    e.condition.positiveCoefficient = d->positiveCoefficient;
    e.condition.negativeCoefficient = d->negativeCoefficient;
    e.condition.positiveSaturation = d->positiveSaturation;
    e.condition.negativeSaturation = d->negativeSaturation;
    e.condition.deadBand = d->deadBand;
}

void PIDWheelDriver::handleSetPeriodic(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetPeriodic)) return;
    auto *d = (const PIDWheelSetPeriodic*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    PIDEffect &e = effects[idx - 1];
    e.magnitude = d->magnitude;
    e.offset = d->offset;
    e.phase = d->phase;
    e.period = d->period;
}

void PIDWheelDriver::handleSetConstantForce(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetConstantForce)) return;
    auto *d = (const PIDWheelSetConstantForce*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    effects[idx - 1].magnitude = d->magnitude;
}

void PIDWheelDriver::handleSetRamp(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelSetRamp)) return;
    auto *d = (const PIDWheelSetRamp*)data;
    uint8_t idx = d->effectBlockIndex;
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;
    effects[idx - 1].startLevel = d->startLevel;
    effects[idx - 1].endLevel = d->endLevel;
}

void PIDWheelDriver::handleEffectOperation(const uint8_t *data, uint16_t len) {
    if (len < sizeof(PIDWheelEffectOperation)) return;
    auto *d = (const PIDWheelEffectOperation*)data;
    uint8_t idx = d->effectBlockIndex;
    uint8_t op = d->operation;

    if (op == 2) { // Start Solo — stop all others
        for (int i = 0; i < PID_MAX_EFFECTS; i++)
            effects[i].state = 0;
    }

    if (idx >= 1 && idx <= PID_MAX_EFFECTS) {
        if (op == 1 || op == 2) { // Start / Start Solo
            effects[idx - 1].state = 1;
            effects[idx - 1].startTime = time_us_32() / 1000;
        } else if (op == 3) { // Stop
            effects[idx - 1].state = 0;
        }
    }
}

void PIDWheelDriver::handleBlockFree(const uint8_t *data, uint16_t len) {
    if (len < 1) return;
    uint8_t idx = data[0];
    if (idx >= 1 && idx <= PID_MAX_EFFECTS) {
        memset(&effects[idx - 1], 0, sizeof(PIDEffect));
    }
}

void PIDWheelDriver::handleDeviceControl(const uint8_t *data, uint16_t len) {
    if (len < 1) return;
    uint8_t ctrl = data[0];
    if (ctrl & 0x01) actuatorsEnabled = true;   // Enable Actuators
    if (ctrl & 0x02) actuatorsEnabled = false;  // Disable Actuators
    if (ctrl & 0x04) {                           // Stop All Effects
        for (int i = 0; i < PID_MAX_EFFECTS; i++)
            effects[i].state = 0;
    }
    if (ctrl & 0x08) {                           // Device Reset
        memset(effects, 0, sizeof(effects));
        actuatorsEnabled = true;
        devicePaused = false;
        deviceGain = 255;
    }
    if (ctrl & 0x10) devicePaused = true;        // Pause
    if (ctrl & 0x20) devicePaused = false;       // Continue
}

void PIDWheelDriver::handleDeviceGain(const uint8_t *data, uint16_t len) {
    if (len < 1) return;
    deviceGain = data[0];
}

int32_t PIDWheelDriver::calcConditionForce(PIDEffect &effect, int32_t position) {
    PIDEffectCondition &c = effect.condition;
    int32_t pos = position - c.cpOffset;
    int32_t force = 0;

    if (pos > c.deadBand) {
        force = (int32_t)(pos - c.deadBand) * c.positiveCoefficient / 32767;
        if (c.positiveSaturation > 0 && force > c.positiveSaturation)
            force = c.positiveSaturation;
    } else if (pos < -(int32_t)c.deadBand) {
        force = (int32_t)(pos + c.deadBand) * c.negativeCoefficient / 32767;
        if (c.negativeSaturation > 0 && force < -(int32_t)c.negativeSaturation)
            force = -(int32_t)c.negativeSaturation;
    }
    return force;
}

int16_t PIDWheelDriver::calculateForce() {
    if (!actuatorsEnabled || devicePaused) return 0;

    int32_t totalForce = 0;
    uint32_t now_ms = time_us_32() / 1000;
    int32_t currentPosition = inputReport.X;

    for (int i = 0; i < PID_MAX_EFFECTS; i++) {
        if (!(effects[i].state & 0x01)) continue;
        if (effects[i].type == FFB_EFFECT_NONE) continue;

        uint32_t elapsed = now_ms - effects[i].startTime;

        if (effects[i].startDelay > 0 && elapsed < effects[i].startDelay)
            continue;

        if (effects[i].duration != FFB_EFFECT_DURATION_INFINITE && effects[i].duration > 0) {
            if (elapsed > effects[i].startDelay + effects[i].duration) {
                effects[i].state = 0;
                continue;
            }
        }

        int32_t force = 0;
        uint8_t gain = effects[i].gain;
        if (gain == 0) gain = 255;

        switch (effects[i].type) {
            case FFB_EFFECT_CONSTANT:
                force = effects[i].magnitude;
                break;

            case FFB_EFFECT_SPRING:
                force = calcConditionForce(effects[i], currentPosition);
                break;

            case FFB_EFFECT_DAMPER: {
                int32_t velocity = currentPosition - lastPosition;
                PIDEffectCondition &c = effects[i].condition;
                force = velocity * c.positiveCoefficient / 32767;
                int32_t sat = c.positiveSaturation > 0 ? c.positiveSaturation : 32767;
                force = std::clamp(force, -sat, sat);
                break;
            }

            case FFB_EFFECT_FRICTION: {
                int32_t velocity = currentPosition - lastPosition;
                if (velocity > 0) force = -effects[i].condition.positiveCoefficient;
                else if (velocity < 0) force = effects[i].condition.negativeCoefficient;
                break;
            }

            case FFB_EFFECT_INERTIA: {
                int32_t velocity = currentPosition - lastPosition;
                int32_t accel = velocity - 0;
                force = -accel * effects[i].condition.positiveCoefficient / 32767;
                break;
            }

            case FFB_EFFECT_SINE:
            case FFB_EFFECT_SQUARE:
            case FFB_EFFECT_TRIANGLE:
            case FFB_EFFECT_SAWTOOTHUP:
            case FFB_EFFECT_SAWTOOTHDOWN: {
                if (effects[i].period == 0) break;
                uint32_t t = (elapsed * 1000) % (effects[i].period * 1000);
                float phase = (float)t / (float)(effects[i].period * 1000) * 2.0f * 3.14159265f;
                phase += (float)effects[i].phase * 3.14159265f / 18000.0f;
                float wave = 0;
                if (effects[i].type == FFB_EFFECT_SINE) wave = sinf(phase);
                else if (effects[i].type == FFB_EFFECT_SQUARE) wave = (sinf(phase) >= 0) ? 1.0f : -1.0f;
                else if (effects[i].type == FFB_EFFECT_TRIANGLE) wave = acosf(cosf(phase)) / 1.5708f - 1.0f;
                else if (effects[i].type == FFB_EFFECT_SAWTOOTHUP) wave = fmodf(phase / 6.2832f, 1.0f) * 2.0f - 1.0f;
                else if (effects[i].type == FFB_EFFECT_SAWTOOTHDOWN) wave = 1.0f - fmodf(phase / 6.2832f, 1.0f) * 2.0f;
                force = (int32_t)(wave * effects[i].magnitude) + effects[i].offset;
                break;
            }

            case FFB_EFFECT_RAMP: {
                if (effects[i].duration == 0 || effects[i].duration == FFB_EFFECT_DURATION_INFINITE) {
                    force = effects[i].endLevel;
                } else {
                    float progress = (float)(elapsed - effects[i].startDelay) / (float)effects[i].duration;
                    progress = std::clamp(progress, 0.0f, 1.0f);
                    force = effects[i].startLevel + (int32_t)((effects[i].endLevel - effects[i].startLevel) * progress);
                }
                break;
            }

            default:
                break;
        }

        // Apply gain
        force = (force * gain) / 255;

        // Apply envelope
        if (effects[i].useEnvelope && effects[i].duration != FFB_EFFECT_DURATION_INFINITE) {
            uint32_t effectElapsed = elapsed - effects[i].startDelay;
            if (effectElapsed < effects[i].attackTime && effects[i].attackTime > 0) {
                float ratio = (float)effectElapsed / (float)effects[i].attackTime;
                int32_t envForce = effects[i].attackLevel + (int32_t)((force - effects[i].attackLevel) * ratio);
                force = envForce;
            } else if (effects[i].duration > effects[i].fadeTime) {
                uint32_t fadeStart = effects[i].duration - effects[i].fadeTime;
                if (effectElapsed > fadeStart && effects[i].fadeTime > 0) {
                    float ratio = (float)(effectElapsed - fadeStart) / (float)effects[i].fadeTime;
                    int32_t envForce = force + (int32_t)((effects[i].fadeLevel - force) * ratio);
                    force = envForce;
                }
            }
        }

        totalForce += force;
    }

    lastPosition = currentPosition;

    // Apply device gain
    totalForce = (totalForce * deviceGain) / 255;

    return static_cast<int16_t>(std::clamp(totalForce, (int32_t)-32767, (int32_t)32767));
}

uint8_t PIDWheelDriver::allocateEffect() {
    for (uint8_t i = 0; i < PID_MAX_EFFECTS; i++) {
        if (effects[i].type == FFB_EFFECT_NONE && effects[i].state == 0)
            return i + 1;
    }
    return 1;
}

bool PIDWheelDriver::vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                              tusb_control_request_t const *request) {
    return false;
}

const uint16_t * PIDWheelDriver::get_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t desc_str[32 + 1];
    uint8_t chr_count = 0;
    switch (index) {
        case 0:
            desc_str[1] = 0x0409;
            chr_count = 1;
            break;
        case 1: {
            const char *str = pidwheel_string_manufacturer;
            chr_count = strlen(str);
            for (uint8_t i = 0; i < chr_count; i++) desc_str[1 + i] = str[i];
            break;
        }
        case 2: {
            const char *str = pidwheel_string_product;
            chr_count = strlen(str);
            for (uint8_t i = 0; i < chr_count; i++) desc_str[1 + i] = str[i];
            break;
        }
        default: return nullptr;
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc_str;
}

const uint8_t * PIDWheelDriver::get_descriptor_device_cb() { return pidwheel_device_descriptor; }
const uint8_t * PIDWheelDriver::get_hid_descriptor_report_cb(uint8_t itf) { return pidwheel_report_descriptor; }
const uint8_t * PIDWheelDriver::get_descriptor_configuration_cb(uint8_t index) { return pidwheel_configuration_descriptor; }
const uint8_t * PIDWheelDriver::get_descriptor_device_qualifier_cb() { return pidwheel_device_qualifier; }
uint16_t PIDWheelDriver::GetJoystickMidValue() { return PIDWHEEL_JOYSTICK_MID; }
