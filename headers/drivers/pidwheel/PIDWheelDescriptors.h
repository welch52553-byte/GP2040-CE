#ifndef _PIDWHEEL_DESCRIPTORS_H_
#define _PIDWHEEL_DESCRIPTORS_H_

#include "tusb.h"
#include <cstdint>

#define PIDWHEEL_VENDOR_ID   0x045E
#define PIDWHEEL_PRODUCT_ID  0x001A

#define PIDWHEEL_JOYSTICK_MID  0x7FFF
#define PIDWHEEL_ENDPOINT_SIZE 64

// Input Report ID 1: Wheel axes + buttons
#define PIDWHEEL_REPORT_ID_INPUT 1
// Output Report ID 2: PID Set Effect
#define PIDWHEEL_REPORT_ID_PID_SET_EFFECT 2
// Output Report ID 3: PID Set Constant Force
#define PIDWHEEL_REPORT_ID_PID_SET_CONSTANT 3
// Output Report ID 4: PID Effect Operation (Start/Stop)
#define PIDWHEEL_REPORT_ID_PID_EFFECT_OP 4
// Output Report ID 5: PID Block Free
#define PIDWHEEL_REPORT_ID_PID_BLOCK_FREE 5
// Feature Report ID 6: PID Pool Report
#define PIDWHEEL_REPORT_ID_PID_POOL 6
// Feature Report ID 7: Create New Effect
#define PIDWHEEL_REPORT_ID_PID_CREATE_EFFECT 7

#define PID_MAX_EFFECTS 8

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint16_t steering;
    uint8_t throttle;
    uint8_t brake;
    uint32_t buttons;
    uint8_t hat;
} PIDWheelInputReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t effect_block_index;
    uint8_t effect_type;
    uint16_t duration;
    uint16_t trigger_repeat_interval;
    uint8_t gain;
} PIDWheelSetEffectReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t effect_block_index;
    int16_t magnitude;
} PIDWheelSetConstantForceReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t effect_block_index;
    uint8_t operation; // 1=Start, 2=StartSolo, 3=Stop
    uint8_t loop_count;
} PIDWheelEffectOperationReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t effect_block_index;
} PIDWheelBlockFreeReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t ram_pool_size;
    uint8_t simultaneous_effects_max;
    uint8_t device_managed_pool;
} PIDWheelPoolReport;

typedef struct __attribute__((packed, aligned(1)))
{
    uint8_t report_id;
    uint8_t effect_type;
    uint8_t effect_block_index;
} PIDWheelCreateEffectReport;

// Effect types
enum PIDEffectType : uint8_t {
    PID_EFFECT_CONSTANT_FORCE = 1,
    PID_EFFECT_SPRING = 2,
    PID_EFFECT_DAMPER = 3,
    PID_EFFECT_FRICTION = 4,
    PID_EFFECT_SINE = 5,
};

// Effect operation
enum PIDEffectOp : uint8_t {
    PID_OP_START = 1,
    PID_OP_START_SOLO = 2,
    PID_OP_STOP = 3,
};

// Effect state
typedef struct {
    bool active;
    uint8_t type;
    uint16_t duration;
    uint8_t gain;
    int16_t magnitude;
    uint32_t start_time;
} PIDEffect;

// HID Report Descriptor for Wheel with PID
static const uint8_t pidwheel_report_descriptor[] = {
    // ========================
    // Input Report: Wheel axes + buttons
    // ========================
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,        // USAGE (Joystick) — recognized as wheel by sim software
    0xA1, 0x01,        // COLLECTION (Application)

    // Report ID 1: Input
    0x85, PIDWHEEL_REPORT_ID_INPUT,

    // Steering axis (16-bit, 0-65535)
    0x05, 0x02,        //   USAGE_PAGE (Simulation Controls)
    0x09, 0xC8,        //   USAGE (Steering)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // LOGICAL_MAXIMUM (65535)
    0x75, 0x10,        //   REPORT_SIZE (16)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)

    // Throttle (8-bit, 0-255)
    0x09, 0xC4,        //   USAGE (Accelerator)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,        //   REPORT_SIZE (8)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)

    // Brake (8-bit, 0-255)
    0x09, 0xC5,        //   USAGE (Brake)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,        //   REPORT_SIZE (8)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)

    // 32 Buttons
    0x05, 0x09,        //   USAGE_PAGE (Button)
    0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
    0x29, 0x20,        //   USAGE_MAXIMUM (Button 32)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //   REPORT_SIZE (1)
    0x95, 0x20,        //   REPORT_COUNT (32)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)

    // Hat Switch (4-bit)
    0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
    0x09, 0x39,        //   USAGE (Hat switch)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x25, 0x07,        //   LOGICAL_MAXIMUM (7)
    0x75, 0x04,        //   REPORT_SIZE (4)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x81, 0x42,        //   INPUT (Data,Var,Abs,Null)
    // Padding 4 bits
    0x75, 0x04,        //   REPORT_SIZE (4)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)

    // ========================
    // PID Force Feedback
    // ========================
    0x05, 0x0F,        //   USAGE_PAGE (Physical Interface Device)
    0x09, 0x21,        //   USAGE (Set Effect Report)
    0xA1, 0x02,        //   COLLECTION (Logical)

    // Report ID 2: Set Effect
    0x85, PIDWHEEL_REPORT_ID_PID_SET_EFFECT,

    0x09, 0x22,        //     USAGE (Effect Block Index)
    0x15, 0x01,        //     LOGICAL_MINIMUM (1)
    0x25, PID_MAX_EFFECTS, // LOGICAL_MAXIMUM
    0x75, 0x08,        //     REPORT_SIZE (8)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0x09, 0x25,        //     USAGE (Effect Type)
    0xA1, 0x02,        //     COLLECTION (Logical)
    0x09, 0x26,        //       USAGE (ET Constant Force)
    0x09, 0x27,        //       USAGE (ET Ramp)
    0x09, 0x30,        //       USAGE (ET Square)
    0x09, 0x31,        //       USAGE (ET Sine)
    0x09, 0x32,        //       USAGE (ET Triangle)
    0x09, 0x33,        //       USAGE (ET Sawtooth Up)
    0x09, 0x34,        //       USAGE (ET Sawtooth Down)
    0x09, 0x40,        //       USAGE (ET Spring)
    0x09, 0x41,        //       USAGE (ET Damper)
    0x09, 0x42,        //       USAGE (ET Inertia)
    0x09, 0x43,        //       USAGE (ET Friction)
    0x15, 0x01,        //       LOGICAL_MINIMUM (1)
    0x25, 0x0B,        //       LOGICAL_MAXIMUM (11)
    0x75, 0x08,        //       REPORT_SIZE (8)
    0x95, 0x01,        //       REPORT_COUNT (1)
    0x91, 0x00,        //       OUTPUT (Data,Ary,Abs)
    0xC0,              //     END_COLLECTION

    0x09, 0x50,        //     USAGE (Duration)
    0x15, 0x00,        //     LOGICAL_MINIMUM (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // LOGICAL_MAXIMUM (65535)
    0x75, 0x10,        //     REPORT_SIZE (16)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0x09, 0x54,        //     USAGE (Trigger Repeat Interval)
    0x15, 0x00,        //     LOGICAL_MINIMUM (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // LOGICAL_MAXIMUM (65535)
    0x75, 0x10,        //     REPORT_SIZE (16)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0x09, 0x52,        //     USAGE (Gain)
    0x15, 0x00,        //     LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,  //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,        //     REPORT_SIZE (8)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0xC0,              //   END_COLLECTION (Set Effect)

    // Report ID 3: Set Constant Force
    0x09, 0x73,        //   USAGE (Set Constant Force Report)
    0xA1, 0x02,        //   COLLECTION (Logical)
    0x85, PIDWHEEL_REPORT_ID_PID_SET_CONSTANT,

    0x09, 0x22,        //     USAGE (Effect Block Index)
    0x15, 0x01,        //     LOGICAL_MINIMUM (1)
    0x25, PID_MAX_EFFECTS,
    0x75, 0x08,        //     REPORT_SIZE (8)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0x09, 0x70,        //     USAGE (Magnitude)
    0x16, 0x01, 0x80,  //     LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F,  //     LOGICAL_MAXIMUM (32767)
    0x75, 0x10,        //     REPORT_SIZE (16)
    0x95, 0x01,        //     REPORT_COUNT (1)
    0x91, 0x02,        //     OUTPUT (Data,Var,Abs)

    0xC0,              //   END_COLLECTION (Constant Force)

    // Report ID 4: Effect Operation
    0x09, 0x77,        //   USAGE (Effect Operation Report)
    0xA1, 0x02,        //   COLLECTION (Logical)
    0x85, PIDWHEEL_REPORT_ID_PID_EFFECT_OP,

    0x09, 0x22,        //     USAGE (Effect Block Index)
    0x15, 0x01,
    0x25, PID_MAX_EFFECTS,
    0x75, 0x08,
    0x95, 0x01,
    0x91, 0x02,

    0x09, 0x78,        //     USAGE (Operation)
    0xA1, 0x02,        //     COLLECTION (Logical)
    0x09, 0x79,        //       USAGE (Op Effect Start)
    0x09, 0x7A,        //       USAGE (Op Effect Start Solo)
    0x09, 0x7B,        //       USAGE (Op Effect Stop)
    0x15, 0x01,
    0x25, 0x03,
    0x75, 0x08,
    0x95, 0x01,
    0x91, 0x00,        //       OUTPUT (Data,Ary,Abs)
    0xC0,              //     END_COLLECTION

    0x09, 0x7C,        //     USAGE (Loop Count)
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, 0x01,
    0x91, 0x02,

    0xC0,              //   END_COLLECTION (Effect Operation)

    // Report ID 5: PID Block Free
    0x09, 0x90,        //   USAGE (PID Block Free Report)
    0xA1, 0x02,
    0x85, PIDWHEEL_REPORT_ID_PID_BLOCK_FREE,

    0x09, 0x22,        //     USAGE (Effect Block Index)
    0x15, 0x01,
    0x25, PID_MAX_EFFECTS,
    0x75, 0x08,
    0x95, 0x01,
    0x91, 0x02,

    0xC0,              //   END_COLLECTION (Block Free)

    // Report ID 6: PID Pool Report (Feature)
    0x09, 0x7F,        //   USAGE (PID Pool Report)
    0xA1, 0x02,
    0x85, PIDWHEEL_REPORT_ID_PID_POOL,

    0x09, 0x80,        //     USAGE (RAM Pool Size)
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, 0x01,
    0xB1, 0x02,        //     FEATURE (Data,Var,Abs)

    0x09, 0x83,        //     USAGE (Simultaneous Effects Max)
    0x15, 0x00,
    0x25, PID_MAX_EFFECTS,
    0x75, 0x08,
    0x95, 0x01,
    0xB1, 0x02,

    0x09, 0x84,        //     USAGE (Device Managed Pool)
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x08,
    0x95, 0x01,
    0xB1, 0x02,

    0xC0,              //   END_COLLECTION (Pool Report)

    // Report ID 7: Create New Effect (Feature)
    0x09, 0xAB,        //   USAGE (Create New Effect Report)
    0xA1, 0x02,
    0x85, PIDWHEEL_REPORT_ID_PID_CREATE_EFFECT,

    0x09, 0x25,        //     USAGE (Effect Type)
    0x15, 0x01,
    0x25, 0x0B,
    0x75, 0x08,
    0x95, 0x01,
    0xB1, 0x02,        //     FEATURE (Data,Var,Abs)

    0x09, 0x22,        //     USAGE (Effect Block Index)
    0x15, 0x01,
    0x25, PID_MAX_EFFECTS,
    0x75, 0x08,
    0x95, 0x01,
    0xB1, 0x02,

    0xC0,              //   END_COLLECTION (Create Effect)

    0xC0               // END_COLLECTION (Application)
};

// Device Descriptor
static const uint8_t pidwheel_device_descriptor[] = {
    18,                     // bLength
    1,                      // bDescriptorType (Device)
    0x00, 0x02,             // bcdUSB 2.00
    0x00,                   // bDeviceClass (Use class from Interface)
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    64,                     // bMaxPacketSize0
    PIDWHEEL_VENDOR_ID & 0xFF, (PIDWHEEL_VENDOR_ID >> 8) & 0xFF,
    PIDWHEEL_PRODUCT_ID & 0xFF, (PIDWHEEL_PRODUCT_ID >> 8) & 0xFF,
    0x00, 0x01,             // bcdDevice 1.00
    0x01,                   // iManufacturer
    0x02,                   // iProduct
    0x00,                   // iSerialNumber
    0x01,                   // bNumConfigurations
};

// Configuration Descriptor
#define PIDWHEEL_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static const uint8_t pidwheel_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, PIDWHEEL_CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),
    TUD_HID_INOUT_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE,
                             sizeof(pidwheel_report_descriptor),
                             0x81, 0x02, PIDWHEEL_ENDPOINT_SIZE, 1),
};

static const uint8_t pidwheel_device_qualifier[] = {
    10,                     // bLength
    6,                      // bDescriptorType (Device Qualifier)
    0x00, 0x02,             // bcdUSB 2.00
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    64,                     // bMaxPacketSize0
    0x01,                   // bNumConfigurations
    0x00,                   // Reserved
};

// String descriptors
static const char * pidwheel_string_manufacturer = "GP2040-CE";
static const char * pidwheel_string_product = "GP2040-CE Racing Wheel";

#endif
