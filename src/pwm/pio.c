#include "vga_internal.h"
#include "rgb.pio.h"


void vga_pio_init(
    vga_mode_t *mode,
    uint8_t vsync_base_pin,
    uint8_t rgb_base_pin) {

  bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
      &vga_pwm_pio_program,
      &(vga.pio.pio),
      &(vga.pio.sm),
      &(vga.pio.offset),
      rgb_base_pin,
      4,
      true);

  hard_assert(rc);


  vga_pwm_pio_program_init(
      vga.pio.pio,
      vga.pio.sm,
      vga.pio.offset,
      mode,
      rgb_base_pin,
      vsync_base_pin);

}

void vga_pio_enable(rgb_pio_t *pio) {
  pio_sm_set_enabled(pio->pio, pio->sm, true);
}

