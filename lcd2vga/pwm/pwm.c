#include "vga_internal.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

static inline uint16_t vga_sync_total(vga_sync_t *sync);
static inline uint32_t vga_get_enable_mask();

static inline void vga_pwm_pin_setup(uint8_t pin);

void vga_pwm_init(
    uint8_t hsync_pin,
    uint8_t vsync_pin) {

  // Setup the sync signals 
  vga.hsync_slice = vga_hsync_pwm(
      &(vga.mode.h), hsync_pin, vga.mode.pixel_clk);
  vga.vsync_slice = vga_vsync_pwm(&(vga.mode.v), vsync_pin);
}


void vga_pwm_enable() {
    pwm_set_mask_enabled(
        vga_get_enable_mask());
}

uint16_t static inline vga_sync_total(vga_sync_t *sync) {
  return (sync->front_porch + sync->pulse + sync->back_porch + sync->visible);
}

uint32_t static inline vga_get_enable_mask() {
  return (1 << vga.hsync_slice) | (1 << vga.vsync_slice);
}

uint8_t vga_hsync_pwm(
    vga_sync_t *sync, uint8_t hsync_pin, uint32_t pixel_clk) {

  vga_pwm_pin_setup(hsync_pin);
  vga_pwm_pin_setup(hsync_pin + 1);


  uint8_t slice = pwm_gpio_to_slice_num(hsync_pin);

  pwm_config cfg = pwm_get_default_config();

  pwm_config_set_clkdiv_int_frac4(&cfg, 10, 0);

  // Channel B is always positive
  pwm_config_set_output_polarity(&cfg, sync->negative, true);

  // Set Wrap Counter
  pwm_config_set_wrap(&cfg, vga_sync_total(sync));

  pwm_init(slice, &cfg, false);

  // Set Pulse width in pixels
  pwm_set_both_levels(slice, sync->pulse, sync->pulse);

  return slice;
}

uint8_t vga_vsync_pwm(vga_sync_t *sync, uint8_t vsync_pin) {
  vga_pwm_pin_setup(vsync_pin);
  vga_pwm_pin_setup(vsync_pin + 1);


  uint8_t slice = pwm_gpio_to_slice_num(vsync_pin);

  pwm_config cfg = pwm_get_default_config();

  // Set the clock to trigger on the falling edge of B
  pwm_config_set_clkdiv_mode (&cfg, PWM_DIV_B_FALLING);
 
  pwm_config_set_output_polarity(&cfg, sync->negative, false);

  // Set Wrap Counter
  pwm_config_set_wrap(&cfg, vga_sync_total(sync));

  pwm_init(slice, &cfg, false);
 
  // Set Pulse width in pixels
  pwm_set_both_levels(slice, sync->pulse, sync->pulse);

  return slice;
}

void vga_pwm_pin_setup(uint8_t pin) {
  gpio_set_function(pin, GPIO_FUNC_PWM);
  gpio_set_slew_rate(pin, GPIO_SLEW_RATE_FAST);
  gpio_disable_pulls(pin);
}
