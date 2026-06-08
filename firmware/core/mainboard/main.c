#include "mainboard_hal.h"

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
#include "mainboard_display_module.h"
#endif

#include "machine_config.h"
#include "usb.h"
#include "usb_cdc.h"

config_interface_t machine_config = {
    .version = 1 << 16 // Version 1.0

    // end default machine configuration
};

void usb_cdc_handshake_cb(bool dtr, bool rts)
{
  diag_printf("dtr: %d, rts: %d\r\n", dtr, rts);
}

void usb_mounted_cb()
{
  diag_print("USB mounted\r\n");
}

void usb_unmounted_cb()
{
  diag_print("USB unmounted\r\n");
}

void usb_suspended_cb()
{
  diag_print("USB suspended\r\n");
}

void usb_cdc_rx_cb()
{
  while (usb_cdc_available())
  {
    // Echo data
    char c = usb_cdc_read_char();
    usb_cdc_write_char(c);
  }
}

int main(void)
{
  // Keep the shared mainboard entry point hardware-neutral. Board-specific
  // startup, pin setup, clocks, and peripheral quirks live behind this HAL.
  mainboard_initialize_hardware();

  // Initialise USB device state
  usb_device_init();

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
  // A mainboard-attached display module shares this firmware image and the
  // mainboard clock tree. The module may initialize only panel-local hardware
  // such as display pins, encoder inputs, chip-select lines, sounders, or LEDs.
  mainboard_display_module_initialize_hardware();
#endif

  while (1)
  {
    mainboard_run_background_tasks();
    usb_systick_hal();

#ifdef MAINBOARD_HAS_DISPLAY_MODULE
    mainboard_display_module_run_background_tasks();
    mainboard_display_module_run_feedback_tasks();
#endif
  }
}
