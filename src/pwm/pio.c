#include "vga_internal.h"
#include "rgb.pio.h"

void vga_pio_init(
    rgb_pio_t *pio,
    vga_mode_t *mode,
    uint8_t vsync_base_pin,
    uint8_t rgb_base_pin) {

  bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
      &vga_pwm_pio_program,
      &(pio->pio),
      &(pio->sm),
      &(pio->offset),
      rgb_base_pin,
      4,
      true);

  hard_assert(rc);

  vga_pwm_pio_program_init(
      pio->pio,
      pio->sm,
      pio->offset,
      mode->pixel_clk,
      rgb_base_pin,
      vsync_base_pin);

}

void vga_pio_enable(rgb_pio_t *pio) {
  pio_sm_set_enabled(pio->pio, pio->sm, true);
}

