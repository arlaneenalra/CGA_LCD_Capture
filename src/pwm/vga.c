#include "vga_internal.h"

#include "hardware/dma.h"
#include "hardware/irq.h"

vga_pwm_t vga;

void vga_init(
    vga_mode_t *vga_mode,
    uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin) {

  vga.mode = vga_mode;
  vga.frame_buf = frame_buf;
  
  vga_pwm_init(hsync_pin, vsync_pin);
  vga_pio_init(&(vga.pio), vga_mode, vsync_pin, rgb_pin);

  vga_dma_init();
}

void vga_enable() {
  vga_pwm_enable(&vga);
  vga_pio_enable(&(vga.pio));

  // Start everything off!
  //vga_dma_irq();

  vga_dma_enable();
}


