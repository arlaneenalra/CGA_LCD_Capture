#include <stdio.h>
#include <string.h>


#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"

#include "lcd.h"

#include "pwm/vga.h"

#define VGA_HSYNC 16
#define VGA_HSYNC_OUT (VGA_HSYNC + 1)

#define VGA_VSYNC 14
#define VGA_VSYNC_CLK (VGA_HSYNC_OUT)

#define VGA_RGB_BASE_PIN 18
#define VGA_LO_GREEN (VGA_RGB_BASE_PIN + 0)
#define VGA_HI_GREEN (VGA_RGB_BASE_PIN + 1)
#define VGA_BLUE (VGA_RGB_BASE_PIN + 2)
#define VGA_RED (VGA_RGB_BASE_PIN + 3)

#define LDO_GPIO 23
void ldo_pwm_mode();
void dump_frame(scr_frame_buf_t pixels);

vga_frame_buf_t buffer;

// Decodes a block of 8 pixesls from the source
inline uint32_t decode_pixels(uint32_t source, uint32_t offset) {
  uint32_t block = (source & (0x000000FF << offset)) >> offset;
  
  return 
    ((block & 0x01) > 0 ? 0x0000000F : 0x0 ) |
    ((block & 0x02) > 0 ? 0x000000F0 : 0x0 ) |
    ((block & 0x04) > 0 ? 0x00000F00 : 0x0 ) |
    ((block & 0x08) > 0 ? 0x0000F000 : 0x0 ) |

    ((block & 0x10) > 0 ? 0x000F0000 : 0x0 ) |
    ((block & 0x20) > 0 ? 0x00F00000 : 0x0 ) |
    ((block & 0x40) > 0 ? 0x0F000000 : 0x0 ) |
    ((block & 0x80) > 0 ? 0xF0000000 : 0x0 );

}

void dump_frame(scr_frame_buf_t pixels) {
  uint32_t offset = 80 * 40;
  uint32_t vga_index = 80 * 40;
  uint32_t source = 0;

  // line doubled version
  for (int x = 0; x < 20; x++) {
    for (int y = 0; y < 200; y++) {
      source = pixels[x + (y*20)];
      vga_index = offset + (x * 4) + (y * 160);
      buffer[vga_index + 80] = buffer[vga_index] = 
        decode_pixels(source, 24);
      buffer[vga_index + 81] = buffer[vga_index + 1] =
        decode_pixels(source, 16);
      buffer[vga_index + 82] = buffer[vga_index + 2] = 
        decode_pixels(source, 8);
      buffer[vga_index + 83] = buffer[vga_index + 3] = 
        decode_pixels(source, 0);
    }
  }
}

void ldo_pwm_mode() {
  gpio_init(LDO_GPIO);
  gpio_set_dir(LDO_GPIO, GPIO_OUT);
  gpio_put(LDO_GPIO, true);
}

static inline void vga_core() {
  vga_init(
      buffer,
      VGA_HSYNC, 
      VGA_VSYNC,
      VGA_RGB_BASE_PIN);


  vga_enable();

  while(true) {
    tight_loop_contents();
  }
}

int main() {
  ldo_pwm_mode();

  set_sys_clock_khz(250000, true);

  // zero out our frame buffer
  memset((void *)buffer, 0x00, sizeof(buffer));

  in_frame_init(&dump_frame, D0, DMA_IRQ_0, OE_PIN);

  multicore_launch_core1(vga_core);

  // Start capturing frames
  frame_capture_irq();
  frame_capture();
}
