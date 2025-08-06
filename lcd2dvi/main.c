#include <stdio.h>
#include <string.h>


#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bit_ops.h"

#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"

#include "lcd.h"

#include "lcd2dvi.h"

void dump_frame(scr_frame_buf_t pixels);

// Decodes a block of 8 pixesls from the source
#define decode_pixels(source) __rev(source)

void dump_frame(scr_frame_buf_t pixels) {
  static uint32_t cycle = 0;
  uint32_t offset = DVI_LINE_SIZE * 40;
  uint32_t index = DVI_LINE_SIZE * 40;
  uint32_t source = 0;

  // line doubled version
  for (int x = 0; x < 20; x++) {
    for (int y = 0; y < 200; y++) {
      source = pixels[x + (y*20)];
      frame_buf[x + (y * 20)] = decode_pixels(source);
      
      //index = offset + x + (y * 2 * DVI_LINE_SIZE);
      //frame_buf[index + DVI_LINE_SIZE] = frame_buf[index] = decode_pixels(source);
    }
  }
}

int main() {
  local_dvi_init(); 

  // zero out our frame buffer
  //memset((void *)buffer, 0x00, sizeof(buffer));
  memset((void *)frame_buf, 0x0F, sizeof(frame_buf));

  //stdio_init_all();
  
  in_frame_init(&dump_frame, LCD_BASE_PIN, LCD_IRQ, LCD_OE_PIN);

  multicore_launch_core1(local_dvi_core);

  // Start capturing frames
  frame_capture_irq();
  frame_capture();
}
