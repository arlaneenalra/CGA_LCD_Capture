#include "vga.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

vga_mode_t vga_mode_list[] = {

  // 640x480@60Hz http://www.tinyvga.com/vga-timing/640x480@60Hz
  // vertical refresh (h sync) - 31.46875 kHz
  // screen refresh (v sync) - 60 Hz

  [MODE_640X480_60] = {
    .pixel_clk = 25175000,
   
    .h = {
      .front_porch = 16,
      .pulse = 96,
      .back_porch = 48,
      .visible = 640,

      .negative = true
    },

    .v = {
      .front_porch = 10,
      .pulse = 2,
      .back_porch = 33,
      .visible = 480,

      .negative = true
    }
  },

  // 640x400@70Hz http://www.tinyvga.com/vga-timing/640x400@70Hz 
  // vertical refresh (h sync) - 31.46875 kHz
  // screen refresh (v sync) - 60 Hz
  [MODE_640X400_70] = {
    .pixel_clk = 25175000,
   
    .h = {
      .front_porch = 16,
      .pulse = 96,
      .back_porch = 48,
      .visible = 640,

      .negative = true
    },

    .v = {
      .front_porch = 12,
      .pulse = 2,
      .back_porch = 35,
      .visible = 400,
 
      .negative = false 
    }
  },

  // 1280x800@60Hz http://www.tinyvga.com/vga-timing/1280x800@60Hz 
  // vertical refresh (h sync) - 49.678571428571 kHz
  // screen refresh (v sync) - 60 Hz
  [MODE_1280X800_60] = {
    .pixel_clk = 83460000,
   
    .h = {
      .front_porch = 64,
      .pulse = 136,
      .back_porch = 200,
      .visible = 1280,

      .negative = true
    },

    .v = {
      .front_porch = 1,
      .pulse = 3,
      .back_porch = 24,
      .visible = 800,
 
      .negative = false 
    }
  }

};

void vga_pwm_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    bool enable) {

  vga->hsync_slice = vga_hsync_pwm(
      &(vga_mode->h), hsync_pin, vga_mode->pixel_clk);
  vga->vsync_slice = vga_vsync_pwm(&(vga_mode->v), vsync_pin);

  //pwm_set_enabled(vga->hsync_slice, true);
  if (enable) {
    pwm_set_mask_enabled(
        vga_get_enable_mask(vga));
  }
}

uint16_t static inline vga_sync_total(vga_sync_t *sync) {
  return (sync->front_porch + sync->pulse + sync->back_porch + sync->visible);
}

uint32_t static inline vga_get_enable_mask(vga_pwm_t *vga) {
  return (1 << vga->hsync_slice) | (1 << vga->vsync_slice);
}

uint8_t static inline vga_hsync_pwm(
    vga_sync_t *sync, uint8_t hsync_pin, uint32_t pixel_clk) {

  // Calculate the divider
  uint32_t sys_clk = clock_get_hz(clk_sys);

  // Calculate the 4bit part of the 8.4 divider
  uint32_t vga_div8 = sys_clk / pixel_clk;
  uint32_t vga_mod = sys_clk % pixel_clk;
  uint32_t vga_frac4 = ((vga_mod * 16) + (pixel_clk >> 1)) / pixel_clk;

  
  gpio_set_function(hsync_pin, GPIO_FUNC_PWM);
  gpio_set_function(hsync_pin + 1, GPIO_FUNC_PWM);

  uint8_t slice = pwm_gpio_to_slice_num(hsync_pin);

  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv_int_frac4(&cfg, vga_div8, vga_frac4);

  // Channel B is always positive
  pwm_config_set_output_polarity(&cfg, sync->negative, false);

  // Set Wrap Counter
  pwm_config_set_wrap(&cfg, vga_sync_total(sync));

  pwm_init(slice, &cfg, false);

  // Set Pulse width in pixels
  pwm_set_both_levels(slice, sync->pulse, sync->pulse);

  return slice;
}

uint8_t static inline vga_vsync_pwm(vga_sync_t *sync, uint8_t vsync_pin) {
  gpio_set_function(vsync_pin, GPIO_FUNC_PWM);
  gpio_set_function(vsync_pin + 1, GPIO_FUNC_PWM);

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

