/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef PICO_MULTI_ADC_BOARD_CONFIG_H_
#define PICO_MULTI_ADC_BOARD_CONFIG_H_

#include "enums.pb.h"
#include "class/hid/hid.h"

#define BOARD_CONFIG_LABEL "PicoMultiADC"

// ============================================================
// GPIO Pin Mapping
// ============================================================

// --- Mechanical buttons ---
#define GPIO_PIN_00 GpioAction::BUTTON_PRESS_S1     // S1 | Back   | Select
#define GPIO_PIN_01 GpioAction::BUTTON_PRESS_S2     // S2 | Start  | Start (hold on boot = WebConfig)
#define GPIO_PIN_02 GpioAction::BUTTON_PRESS_B3     // B3 | X      | Square
#define GPIO_PIN_03 GpioAction::BUTTON_PRESS_B4     // B4 | Y      | Triangle
#define GPIO_PIN_04 GpioAction::BUTTON_PRESS_B2     // B2 | B      | Circle

// --- ADC pins (GPIO 26-29) assigned to MultiChannelADC addon ---
#define GPIO_PIN_26 GpioAction::ASSIGNED_TO_ADDON   // ADC0 - Steer Left
#define GPIO_PIN_27 GpioAction::ASSIGNED_TO_ADDON   // ADC1 - Steer Right
#define GPIO_PIN_28 GpioAction::ASSIGNED_TO_ADDON   // ADC2 - Throttle
#define GPIO_PIN_29 GpioAction::ASSIGNED_TO_ADDON   // ADC3 - Brake

// --- Rumble motor pins ---
#define GPIO_PIN_06 GpioAction::ASSIGNED_TO_ADDON   // Left motor PWM
#define GPIO_PIN_07 GpioAction::ASSIGNED_TO_ADDON   // Right motor PWM
#define GPIO_PIN_08 GpioAction::ASSIGNED_TO_ADDON   // Motor sleep (optional)

// Keyboard Mapping Configuration
#define KEY_DPAD_UP     HID_KEY_ARROW_UP
#define KEY_DPAD_DOWN   HID_KEY_ARROW_DOWN
#define KEY_DPAD_RIGHT  HID_KEY_ARROW_RIGHT
#define KEY_DPAD_LEFT   HID_KEY_ARROW_LEFT
#define KEY_BUTTON_B1   HID_KEY_SHIFT_LEFT
#define KEY_BUTTON_B2   HID_KEY_Z
#define KEY_BUTTON_R2   HID_KEY_X
#define KEY_BUTTON_L2   HID_KEY_V
#define KEY_BUTTON_B3   HID_KEY_CONTROL_LEFT
#define KEY_BUTTON_B4   HID_KEY_ALT_LEFT
#define KEY_BUTTON_R1   HID_KEY_SPACE
#define KEY_BUTTON_L1   HID_KEY_C
#define KEY_BUTTON_S1   HID_KEY_5
#define KEY_BUTTON_S2   HID_KEY_1
#define KEY_BUTTON_L3   HID_KEY_EQUAL
#define KEY_BUTTON_R3   HID_KEY_MINUS
#define KEY_BUTTON_A1   HID_KEY_9
#define KEY_BUTTON_A2   HID_KEY_F2
#define KEY_BUTTON_FN   -1

// ============================================================
// MultiChannelADC Addon - Sim Racing Hall-Effect Keys
// ============================================================
#define MULTI_CHANNEL_ADC_ENABLED 1

// ADC Pin Assignments
#define MULTI_ADC_STEER_LEFT_PIN   26    // GPIO26 / ADC0
#define MULTI_ADC_STEER_RIGHT_PIN  27    // GPIO27 / ADC1
#define MULTI_ADC_THROTTLE_PIN     28    // GPIO28 / ADC2
#define MULTI_ADC_BRAKE_PIN        29    // GPIO29 / ADC3

// Hall sensor calibration values (12-bit ADC: 0-4095)
// REST  = ADC reading when key is NOT pressed (idle position)
// ACTIVE = ADC reading when key is FULLY pressed
// Adjust these values based on your actual hall sensor measurements!
// Tip: use a serial monitor to read raw ADC values at rest and pressed states
#define MULTI_ADC_STEER_LEFT_REST    512
#define MULTI_ADC_STEER_LEFT_ACTIVE  3584

#define MULTI_ADC_STEER_RIGHT_REST   512
#define MULTI_ADC_STEER_RIGHT_ACTIVE 3584

#define MULTI_ADC_THROTTLE_REST      512
#define MULTI_ADC_THROTTLE_ACTIVE    3584

#define MULTI_ADC_BRAKE_REST         512
#define MULTI_ADC_BRAKE_ACTIVE       3584

// Auto-calibrate REST values on startup (reads ADC when keys are released)
#define MULTI_ADC_AUTO_CALIBRATE_REST 1

// Signal processing
#define MULTI_ADC_DEADZONE           3     // 3% deadzone to filter noise
#define MULTI_ADC_SMOOTHING_FACTOR   100   // EMA factor: 100/1000 = 0.1
#define MULTI_ADC_OVERSAMPLING       4     // 4x oversampling

// ============================================================
// DRV8833 Rumble - Dual Motor Vibration (Xbox-style)
// ============================================================
#define DRV8833_RUMBLE_ENABLED          1
#define DRV8833_RUMBLE_LEFT_MOTOR_PIN   6     // GPIO6 - Left motor (low freq, heavy)
#define DRV8833_RUMBLE_RIGHT_MOTOR_PIN  7     // GPIO7 - Right motor (high freq, light)
#define DRV8833_RUMBLE_MOTOR_SLEEP_PIN  8     // GPIO8 - Motor driver sleep pin
#define DRV8833_RUMBLE_PWM_FREQUENCY    10000 // 10 kHz PWM
#define DRV8833_RUMBLE_DUTY_MIN         0.0f
#define DRV8833_RUMBLE_DUTY_MAX         100.0f

// ============================================================
// UART Debug Output (connect USB-TTL adapter to read)
// ============================================================
#define MULTI_ADC_DEBUG_ENABLED 1
#define MULTI_ADC_DEBUG_UART_TX_PIN 20  // GPIO20 = UART1 TX

// ============================================================
// Display (disabled - GPIO 0/1 used for buttons)
// ============================================================
#define HAS_I2C_DISPLAY 0
#define I2C0_ENABLED 0

#endif
