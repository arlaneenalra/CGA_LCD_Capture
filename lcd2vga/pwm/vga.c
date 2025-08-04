#include <stdio.h>

#include "vga_internal.h"

vga_pwm_t vga;

extern vga_pio_line_burst_t vga_line_burst;


void vga_init(
    volatile uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin) {

  // 640x480@60Hz http://www.tinyvga.com/vga-timing/640x480@60Hz
  // vertical refresh (h sync) - 31.46875 kHz
  // screen refresh (v sync) - 60 Hz
  // Pixel clock 25.175Mhz
  vga.mode.pixel_clk = 25175000;

  vga.mode.h.front_porch = 16;
  vga.mode.h.pulse = 96;
  vga.mode.h.back_porch = 48;
  vga.mode.h.visible = 640;
  vga.mode.h.negative = true;

  vga.mode.v.front_porch = 10;
  vga.mode.v.pulse = 2;
  vga.mode.v.back_porch = 33;
  vga.mode.v.visible = 480;
  vga.mode.v.negative = true;

  vga.frame_buf = frame_buf;
  
  vga_pwm_init(hsync_pin, vsync_pin);
  vga_pio_init(vsync_pin, rgb_pin);

  vga_dma_init();
}

void vga_enable() {
  vga_pwm_enable(&vga);
  vga_pio_enable(&(vga.pio));

  // Start it all off
  vga_dma_enable();
}

