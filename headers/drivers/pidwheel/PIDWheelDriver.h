#ifndef _PIDWHEEL_DRIVER_H_
#define _PIDWHEEL_DRIVER_H_

#include "gpdriver.h"
#include "drivers/pidwheel/PIDWheelDescriptors.h"

class PIDWheelDriver : public GPDriver {
public:
    virtual void initialize();
    virtual void initializeAux() {}
    virtual bool process(Gamepad * gamepad);
    virtual void processAux() {}
    virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen);
    virtual void set_report(uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize);
    virtual bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                        tusb_control_request_t const *request);
    virtual const uint16_t * get_descriptor_string_cb(uint8_t index, uint16_t langid);
    virtual const uint8_t * get_descriptor_device_cb();
    virtual const uint8_t * get_hid_descriptor_report_cb(uint8_t itf);
    virtual const uint8_t * get_descriptor_configuration_cb(uint8_t index);
    virtual const uint8_t * get_descriptor_device_qualifier_cb();
    virtual uint16_t GetJoystickMidValue();
    virtual USBListener * get_usb_auth_listener() { return nullptr; }

private:
    void processSetEffect(const uint8_t *buffer, uint16_t bufsize);
    void processSetConstantForce(const uint8_t *buffer, uint16_t bufsize);
    void processEffectOperation(const uint8_t *buffer, uint16_t bufsize);
    void processBlockFree(const uint8_t *buffer, uint16_t bufsize);
    int16_t calculateForce();
    uint8_t allocateEffect();

    PIDWheelInputReport inputReport;
    uint8_t last_report[PIDWHEEL_ENDPOINT_SIZE];
    PIDEffect effects[PID_MAX_EFFECTS];
    uint8_t next_free_effect;
};

#endif
