/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef PICO_MULTI_ADC_BOARD_CONFIG_H_
#define PICO_MULTI_ADC_BOARD_CONFIG_H_

#include "enums.pb.h"
#include "class/hid/hid.h"

#define BOARD_CONFIG_LABEL "PicoMultiADC"

// Main pin mapping Configuration
//                                                  // GP2040 | Xinput | Switch  | PS3/4/5  | Dinput | Arcade |
#define GPIO_PIN_02 GpioAction::BUTTON_PRESS_UP     // UP     | UP     | UP      | UP       | UP     | UP     |
#define GPIO_PIN_03 GpioAction::BUTTON_PRESS_DOWN   // DOWN   | DOWN   | DOWN    | DOWN     | DOWN   | DOWN   |
#define GPIO_PIN_04 GpioAction::BUTTON_PRESS_RIGHT  // RIGHT  | RIGHT  | RIGHT   | RIGHT    | RIGHT  | RIGHT  |
#define GPIO_PIN_05 GpioAction::BUTTON_PRESS_LEFT   // LEFT   | LEFT   | LEFT    | LEFT     | LEFT   | LEFT   |
#define GPIO_PIN_06 GpioAction::BUTTON_PRESS_B1     // B1     | A      | B       | Cross    | 2      | K1     |
#define GPIO_PIN_07 GpioAction::BUTTON_PRESS_B2     // B2     | B      | A       | Circle   | 3      | K2     |
#define GPIO_PIN_08 GpioAction::BUTTON_PRESS_R2     // R2     | RT     | ZR      | R2       | 8      | K3     |
#define GPIO_PIN_09 GpioAction::BUTTON_PRESS_L2     // L2     | LT     | ZL      | L2       | 7      | K4     |
#define GPIO_PIN_10 GpioAction::BUTTON_PRESS_B3     // B3     | X      | Y       | Square   | 1      | P1     |
#define GPIO_PIN_11 GpioAction::BUTTON_PRESS_B4     // B4     | Y      | X       | Triangle | 4      | P2     |
#define GPIO_PIN_12 GpioAction::BUTTON_PRESS_R1     // R1     | RB     | R       | R1       | 6      | P3     |
#define GPIO_PIN_13 GpioAction::BUTTON_PRESS_L1     // L1     | LB     | L       | L1       | 5      | P4     |
#define GPIO_PIN_16 GpioAction::BUTTON_PRESS_S1     // S1     | Back   | Minus   | Select   | 9      | Coin   |
#define GPIO_PIN_17 GpioAction::BUTTON_PRESS_S2     // S2     | Start  | Plus    | Start    | 10     | Start  |
#define GPIO_PIN_18 GpioAction::BUTTON_PRESS_L3     // L3     | LS     | LS      | L3       | 11     | LS     |
#define GPIO_PIN_19 GpioAction::BUTTON_PRESS_R3     // R3     | RS     | RS      | R3       | 12     | RS     |
#define GPIO_PIN_20 GpioAction::BUTTON_PRESS_A1     // A1     | Guide  | Home    | PS       | 13     | ~      |
#define GPIO_PIN_21 GpioAction::BUTTON_PRESS_A2     // A2     | ~      | Capture | ~        | 14     | ~      |

// ADC pins (GPIO 26-29) assigned to multi-channel ADC addon
#define GPIO_PIN_26 GpioAction::ASSIGNED_TO_ADDON   // ADC0 - Left Stick X
#define GPIO_PIN_27 GpioAction::ASSIGNED_TO_ADDON   // ADC1 - Left Stick Y
#define GPIO_PIN_28 GpioAction::ASSIGNED_TO_ADDON   // ADC2 - Right Stick X
#define GPIO_PIN_29 GpioAction::ASSIGNED_TO_ADDON   // ADC3 - Right Stick Y

// I2C Display pins
#define GPIO_PIN_00 GpioAction::ASSIGNED_TO_ADDON
#define GPIO_PIN_01 GpioAction::ASSIGNED_TO_ADDON

// Turbo
#define GPIO_PIN_14 GpioAction::BUTTON_PRESS_TURBO
#define GPIO_PIN_15 GpioAction::ASSIGNED_TO_ADDON

// Keyboard Mapping Configuration
//                                            // GP2040 | Xinput | Switch  | PS3/4/5  | Dinput | Arcade |
#define KEY_DPAD_UP     HID_KEY_ARROW_UP      // UP     | UP     | UP      | UP       | UP     | UP     |
#define KEY_DPAD_DOWN   HID_KEY_ARROW_DOWN    // DOWN   | DOWN   | DOWN    | DOWN     | DOWN   | DOWN   |
#define KEY_DPAD_RIGHT  HID_KEY_ARROW_RIGHT   // RIGHT  | RIGHT  | RIGHT   | RIGHT    | RIGHT  | RIGHT  |
#define KEY_DPAD_LEFT   HID_KEY_ARROW_LEFT    // LEFT   | LEFT   | LEFT    | LEFT     | LEFT   | LEFT   |
#define KEY_BUTTON_B1   HID_KEY_SHIFT_LEFT    // B1     | A      | B       | Cross    | 2      | K1     |
#define KEY_BUTTON_B2   HID_KEY_Z             // B2     | B      | A       | Circle   | 3      | K2     |
#define KEY_BUTTON_R2   HID_KEY_X             // R2     | RT     | ZR      | R2       | 8      | K3     |
#define KEY_BUTTON_L2   HID_KEY_V             // L2     | LT     | ZL      | L2       | 7      | K4     |
#define KEY_BUTTON_B3   HID_KEY_CONTROL_LEFT  // B3     | X      | Y       | Square   | 1      | P1     |
#define KEY_BUTTON_B4   HID_KEY_ALT_LEFT      // B4     | Y      | X       | Triangle | 4      | P2     |
#define KEY_BUTTON_R1   HID_KEY_SPACE         // R1     | RB     | R       | R1       | 6      | P3     |
#define KEY_BUTTON_L1   HID_KEY_C             // L1     | LB     | L       | L1       | 5      | P4     |
#define KEY_BUTTON_S1   HID_KEY_5             // S1     | Back   | Minus   | Select   | 9      | Coin   |
#define KEY_BUTTON_S2   HID_KEY_1             // S2     | Start  | Plus    | Start    | 10     | Start  |
#define KEY_BUTTON_L3   HID_KEY_EQUAL         // L3     | LS     | LS      | L3       | 11     | LS     |
#define KEY_BUTTON_R3   HID_KEY_MINUS         // R3     | RS     | RS      | R3       | 12     | RS     |
#define KEY_BUTTON_A1   HID_KEY_9             // A1     | Guide  | Home    | PS       | 13     | ~      |
#define KEY_BUTTON_A2   HID_KEY_F2            // A2     | ~      | Capture | ~        | 14     | ~      |
#define KEY_BUTTON_FN   -1                    // Hotkey Function                                        |

// ==========================================
// Multi-Channel ADC Configuration
// ==========================================
// Enable the multi-channel ADC addon
#define MULTI_CHANNEL_ADC_ENABLED 1

// ADC Channel Pin Assignments (GPIO 26-29 = ADC0-ADC3)
#define MULTI_ADC_CH0_PIN 26        // ADC0 -> GPIO 26
#define MULTI_ADC_CH1_PIN 27        // ADC1 -> GPIO 27
#define MULTI_ADC_CH2_PIN 28        // ADC2 -> GPIO 28
#define MULTI_ADC_CH3_PIN 29        // ADC3 -> GPIO 29

// Channel-to-Function Mapping
#define MULTI_ADC_CH0_MAPPING ADCChannelMapping::LEFT_STICK_X
#define MULTI_ADC_CH1_MAPPING ADCChannelMapping::LEFT_STICK_Y
#define MULTI_ADC_CH2_MAPPING ADCChannelMapping::RIGHT_STICK_X
#define MULTI_ADC_CH3_MAPPING ADCChannelMapping::RIGHT_STICK_Y

// Temperature sensor on ADC4
#define MULTI_ADC_TEMP_ENABLED 1

// Signal Processing
#define MULTI_ADC_SMOOTHING_ENABLED 1
#define MULTI_ADC_SMOOTHING_FACTOR 50      // EMA factor: 50/1000 = 0.05
#define MULTI_ADC_DEADZONE_INNER 5         // 5% inner deadzone
#define MULTI_ADC_DEADZONE_OUTER 95        // 95% outer deadzone
#define MULTI_ADC_AUTO_CALIBRATE 1         // Auto-calibrate on startup
#define MULTI_ADC_OVERSAMPLING 4           // 4x oversampling for noise reduction
#define MULTI_ADC_SAMPLE_RATE_US 100       // 100us minimum between samples

// ==========================================
// Turbo Configuration
// ==========================================
#define TURBO_ENABLED 1
#define TURBO_LED_PIN 15

// ==========================================
// LED Configuration
// ==========================================
#define LED_BRIGHTNESS_MAXIMUM 100
#define LED_BRIGHTNESS_STEPS 5
#define LED_FORMAT LED_FORMAT_GRB
#define LEDS_PER_PIXEL 1

// ==========================================
// I2C Display Configuration
// ==========================================
#define HAS_I2C_DISPLAY 1
#define I2C0_ENABLED 1
#define I2C0_PIN_SDA 0
#define I2C0_PIN_SCL 1
#define BUTTON_LAYOUT BUTTON_LAYOUT_STICKLESS
#define BUTTON_LAYOUT_RIGHT BUTTON_LAYOUT_STICKLESSB

#endif
