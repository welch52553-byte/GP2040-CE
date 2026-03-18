#include "drivers/pidwheel/PIDWheelDriver.h"
#include "drivers/pidwheel/PIDWheelDescriptors.h"
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

void PIDWheelDriver::initialize() {
    inputReport = {};
    inputReport.report_id = PIDWHEEL_REPORT_ID_INPUT;
    inputReport.steering = PIDWHEEL_JOYSTICK_MID;
    inputReport.throttle = 0;
    inputReport.brake = 0;
    inputReport.buttons = 0;
    inputReport.hat = 0x0F;

    memset(last_report, 0, sizeof(last_report));
    memset(effects, 0, sizeof(effects));
    next_free_effect = 1;

    class_driver = {
    #if CFG_TUSB_DEBUG >= 2
        .name = "PIDWHEEL",
    #endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = hid_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL
    };
}

bool PIDWheelDriver::process(Gamepad * gamepad) {
    inputReport.steering = gamepad->state.lx;

    inputReport.throttle = gamepad->state.rt;
    inputReport.brake = gamepad->state.lt;

    // D-Pad → Hat
    switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD) {
        case GAMEPAD_MASK_UP:                        inputReport.hat = 0; break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:   inputReport.hat = 1; break;
        case GAMEPAD_MASK_RIGHT:                     inputReport.hat = 2; break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT: inputReport.hat = 3; break;
        case GAMEPAD_MASK_DOWN:                      inputReport.hat = 4; break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:  inputReport.hat = 5; break;
        case GAMEPAD_MASK_LEFT:                      inputReport.hat = 6; break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:    inputReport.hat = 7; break;
        default:                                     inputReport.hat = 0x0F; break;
    }

    inputReport.buttons = 0
        | (gamepad->pressedB1() ? (1U << 0) : 0)
        | (gamepad->pressedB2() ? (1U << 1) : 0)
        | (gamepad->pressedB3() ? (1U << 2) : 0)
        | (gamepad->pressedB4() ? (1U << 3) : 0)
        | (gamepad->pressedL1() ? (1U << 4) : 0)
        | (gamepad->pressedR1() ? (1U << 5) : 0)
        | (gamepad->pressedL2() ? (1U << 6) : 0)
        | (gamepad->pressedR2() ? (1U << 7) : 0)
        | (gamepad->pressedS1() ? (1U << 8) : 0)
        | (gamepad->pressedS2() ? (1U << 9) : 0)
        | (gamepad->pressedL3() ? (1U << 10) : 0)
        | (gamepad->pressedR3() ? (1U << 11) : 0)
        | (gamepad->pressedA1() ? (1U << 12) : 0)
        | (gamepad->pressedA2() ? (1U << 13) : 0)
    ;

    // Calculate PID force and apply to haptics
    int16_t force = calculateForce();
    Gamepad * procGamepad = Storage::getInstance().GetProcessedGamepad();
    if (force != 0) {
        uint16_t absForce = static_cast<uint16_t>(std::min(std::abs((int)force), 255));
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

    if (tud_suspended())
        tud_remote_wakeup();

    void * report = &inputReport;
    uint16_t report_size = sizeof(inputReport);
    if (memcmp(last_report, report, report_size) != 0) {
        if (tud_hid_ready() && tud_hid_report(PIDWHEEL_REPORT_ID_INPUT, ((uint8_t*)report) + 1, report_size - 1)) {
            memcpy(last_report, report, report_size);
            return true;
        }
    }
    return false;
}

uint16_t PIDWheelDriver::get_report(uint8_t report_id, hid_report_type_t report_type,
                                     uint8_t *buffer, uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == PIDWHEEL_REPORT_ID_PID_POOL) {
            PIDWheelPoolReport pool = {};
            pool.report_id = PIDWHEEL_REPORT_ID_PID_POOL;
            pool.ram_pool_size = PID_MAX_EFFECTS;
            pool.simultaneous_effects_max = PID_MAX_EFFECTS;
            pool.device_managed_pool = 1;
            uint16_t len = std::min((uint16_t)sizeof(pool), reqlen);
            memcpy(buffer, &pool, len);
            return len;
        }
        if (report_id == PIDWHEEL_REPORT_ID_PID_CREATE_EFFECT) {
            uint8_t idx = allocateEffect();
            PIDWheelCreateEffectReport resp = {};
            resp.report_id = PIDWHEEL_REPORT_ID_PID_CREATE_EFFECT;
            resp.effect_type = 0;
            resp.effect_block_index = idx;
            uint16_t len = std::min((uint16_t)sizeof(resp), reqlen);
            memcpy(buffer, &resp, len);
            return len;
        }
    }
    if (report_type == HID_REPORT_TYPE_INPUT) {
        uint16_t len = std::min((uint16_t)sizeof(inputReport), reqlen);
        memcpy(buffer, &inputReport, len);
        return len;
    }
    return 0;
}

void PIDWheelDriver::set_report(uint8_t report_id, hid_report_type_t report_type,
                                 uint8_t const *buffer, uint16_t bufsize) {
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        switch (report_id) {
            case PIDWHEEL_REPORT_ID_PID_SET_EFFECT:
                processSetEffect(buffer, bufsize);
                break;
            case PIDWHEEL_REPORT_ID_PID_SET_CONSTANT:
                processSetConstantForce(buffer, bufsize);
                break;
            case PIDWHEEL_REPORT_ID_PID_EFFECT_OP:
                processEffectOperation(buffer, bufsize);
                break;
            case PIDWHEEL_REPORT_ID_PID_BLOCK_FREE:
                processBlockFree(buffer, bufsize);
                break;
        }
    }
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == PIDWHEEL_REPORT_ID_PID_CREATE_EFFECT && bufsize >= 2) {
            uint8_t effect_type = buffer[0];
            uint8_t idx = allocateEffect();
            if (idx > 0 && idx <= PID_MAX_EFFECTS) {
                effects[idx - 1].type = effect_type;
                effects[idx - 1].active = false;
                effects[idx - 1].gain = 255;
                effects[idx - 1].magnitude = 0;
                effects[idx - 1].duration = 0;
            }
        }
    }
}

void PIDWheelDriver::processSetEffect(const uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < sizeof(PIDWheelSetEffectReport) - 1) return;

    uint8_t idx = buffer[0];
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;

    PIDEffect &effect = effects[idx - 1];
    effect.type = buffer[1];
    effect.duration = buffer[2] | (buffer[3] << 8);
    if (bufsize > 5) {
        effect.gain = buffer[6];
    }
}

void PIDWheelDriver::processSetConstantForce(const uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < sizeof(PIDWheelSetConstantForceReport) - 1) return;

    uint8_t idx = buffer[0];
    if (idx < 1 || idx > PID_MAX_EFFECTS) return;

    effects[idx - 1].magnitude = (int16_t)(buffer[1] | (buffer[2] << 8));
}

void PIDWheelDriver::processEffectOperation(const uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < sizeof(PIDWheelEffectOperationReport) - 1) return;

    uint8_t idx = buffer[0];
    uint8_t op = buffer[1];

    if (op == PID_OP_START_SOLO) {
        for (int i = 0; i < PID_MAX_EFFECTS; i++) {
            effects[i].active = false;
        }
    }

    if (idx >= 1 && idx <= PID_MAX_EFFECTS) {
        if (op == PID_OP_START || op == PID_OP_START_SOLO) {
            effects[idx - 1].active = true;
            effects[idx - 1].start_time = time_us_32();
        } else if (op == PID_OP_STOP) {
            effects[idx - 1].active = false;
        }
    }
}

void PIDWheelDriver::processBlockFree(const uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < 1) return;
    uint8_t idx = buffer[0];
    if (idx >= 1 && idx <= PID_MAX_EFFECTS) {
        memset(&effects[idx - 1], 0, sizeof(PIDEffect));
    }
}

int16_t PIDWheelDriver::calculateForce() {
    int32_t totalForce = 0;

    for (int i = 0; i < PID_MAX_EFFECTS; i++) {
        if (!effects[i].active) continue;

        if (effects[i].duration > 0) {
            uint32_t elapsed = (time_us_32() - effects[i].start_time) / 1000;
            if (elapsed > effects[i].duration) {
                effects[i].active = false;
                continue;
            }
        }

        int32_t force = 0;
        uint8_t gain = effects[i].gain;
        if (gain == 0) gain = 255;

        switch (effects[i].type) {
            case PID_EFFECT_CONSTANT_FORCE:
                force = effects[i].magnitude;
                break;
            case PID_EFFECT_SPRING: {
                int32_t steeringCenter = static_cast<int32_t>(inputReport.steering) - 0x7FFF;
                force = -(steeringCenter / 4);
                break;
            }
            case PID_EFFECT_DAMPER: {
                static int16_t lastSteering = 0;
                int16_t currentSteering = static_cast<int16_t>(inputReport.steering >> 1);
                int16_t velocity = currentSteering - lastSteering;
                lastSteering = currentSteering;
                force = -velocity * 8;
                break;
            }
            case PID_EFFECT_FRICTION:
                force = 0;
                break;
            default:
                break;
        }

        force = (force * gain) / 255;
        totalForce += force;
    }

    return static_cast<int16_t>(std::clamp(totalForce, (int32_t)-32767, (int32_t)32767));
}

uint8_t PIDWheelDriver::allocateEffect() {
    for (uint8_t i = 0; i < PID_MAX_EFFECTS; i++) {
        if (!effects[i].active && effects[i].type == 0) {
            return i + 1;
        }
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
            for (uint8_t i = 0; i < chr_count; i++)
                desc_str[1 + i] = str[i];
            break;
        }
        case 2: {
            const char *str = pidwheel_string_product;
            chr_count = strlen(str);
            for (uint8_t i = 0; i < chr_count; i++)
                desc_str[1 + i] = str[i];
            break;
        }
        default:
            return nullptr;
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc_str;
}

const uint8_t * PIDWheelDriver::get_descriptor_device_cb() {
    return pidwheel_device_descriptor;
}

const uint8_t * PIDWheelDriver::get_hid_descriptor_report_cb(uint8_t itf) {
    return pidwheel_report_descriptor;
}

const uint8_t * PIDWheelDriver::get_descriptor_configuration_cb(uint8_t index) {
    return pidwheel_configuration_descriptor;
}

const uint8_t * PIDWheelDriver::get_descriptor_device_qualifier_cb() {
    return pidwheel_device_qualifier;
}

uint16_t PIDWheelDriver::GetJoystickMidValue() {
    return PIDWHEEL_JOYSTICK_MID;
}
