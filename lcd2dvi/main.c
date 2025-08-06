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

#define SCR_ADDR(x, y) ((x) + (y) * SCR_LINE_SIZE)

#define DVI_OFFSET (DVI_LINE_SIZE * 40)

#define DVI_ADDR(x, y) ((x) + (y) * DVI_LINE_SIZE + DVI_OFFSET)

void dump_frame(scr_frame_buf_t pixels) {
  uint32_t source = 0;

  // line doubled version
  for (int x = 0; x < SCR_LINE_SIZE; x++) {
    for (int y = 0; y < SCR_HEIGHT; y++) {
      source = pixels[SCR_ADDR(x, y)];

      frame_buf[DVI_ADDR(x, (y * 2) + 1)] = frame_buf[DVI_ADDR(x, y * 2)] =
        decode_pixels(source);
    }
  }
}

int main() {
  local_dvi_init(); 

  // zero out our frame buffer
  memset((void *)frame_buf, 0x00, sizeof(frame_buf));

  in_frame_init(&dump_frame, LCD_BASE_PIN, LCD_IRQ, LCD_OE_PIN);

  multicore_launch_core1(local_dvi_core);

  // Start capturing frames
  frame_capture_irq();
  frame_capture();
}
