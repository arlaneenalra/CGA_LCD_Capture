#include <stdio.h>
#include <string.h>

#include <inttypes.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"

#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"

#include "lcd.h"
#include "lcd.pio.h"

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

scr_t scr;
vga_frame_buf_t buffer;

queue_t frame_queue;

void dump_frame(scr_frame_buf_t pixels) {
/*  uint32_t vga_index = 0;
  uint32_t source = 0;
  uint32_t dest = 0;

  for (int i = 0; i < SCR_DMA_TRANSFERS; i++) {
    source = pixels[i];
    dest = 0;
    for (int j=0; j < 32; j++) {
      dest = dest >> 4;
      if (source & 1 > 0) {
        dest = dest + 0xF0000000; 
      }
      source = source >> 1;

      vga.scr[vga_index] = dest;

      if (j % 8 == 3) {
        vga_index++;
        dest = 0;
      }
    }
  }*/
}

void in_frame_pio_init(pio_alloc_t *pio_alloc,  uint base_pin) {
  bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
    &in_frame_program,
    &(pio_alloc->pio),
    &(pio_alloc->sm),
    &(pio_alloc->offset),
    base_pin,
    IN_FRAME_PINS,
    true);

  hard_assert(rc);

  for (uint i = 0; i < DOT_CLOCK; i++) {
    pio_gpio_init(pio_alloc->pio, i + base_pin);
/*    gpio_set_function(
        i + base_pin,
        GPIO_FUNC_NULL
    );*/
//    gpio_disable_pulls(i + base_pin);
//    gpio_set_input_enabled(i + base_pin, true);
    //gpio_pull_up(i + base_pin);
    //gpio_set_input_hysteresis_enabled(i + base_pin, false);
  }



  pio_set_gpio_base(pio_alloc->pio, base_pin);

  pio_sm_config c = in_frame_program_get_default_config(pio_alloc->offset);

  // TODO : I may need to move this after pio_sm_init ...
  pio_sm_set_consecutive_pindirs(
    pio_alloc->pio,
    pio_alloc->sm,
    base_pin,
    IN_FRAME_PINS,
    false);

  sm_config_set_clkdiv(&c, 1.0f);
  sm_config_set_in_pins(&c, base_pin);

  pio_sm_init(pio_alloc->pio, pio_alloc->sm, pio_alloc->offset, &c); 
}

void in_frame_dma_init(scr_t *scr) {
  dma_alloc_t *dma = &(scr->dma);
  pio_alloc_t *pio = &(scr->pio);

  // Do basic setup.
  dma->channel = dma_claim_unused_channel(true);
  dma->config = dma_channel_get_default_config(dma->channel);
  
  channel_config_set_transfer_data_size(&(dma->config), DMA_SIZE_32);
  channel_config_set_read_increment(&(dma->config), false);
  channel_config_set_write_increment(&(dma->config), true);
  channel_config_set_dreq(
      &(dma->config),
      pio_get_dreq(pio->pio, pio->sm, false)
  );
 
  dma_channel_configure(
      dma->channel,
      &(dma->config),
      NULL,
      &(pio->pio->rxf[pio->sm]),
      SCR_DMA_TRANSFERS,
      false);
  
  // Setup IRQ
  dma_channel_set_irq0_enabled(dma->channel, true);
  irq_set_exclusive_handler(DMA_IRQ_0, frame_capture_irq);
  irq_set_enabled(DMA_IRQ_0, true);
}

void frame_capture_irq() {
  static queue_frame_t irq_frame = {0, 0, 0, 0};

  // cleanly (re)start the PIO
  pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, false);

  irq_frame.end = get_absolute_time();

  pio_sm_restart(scr.pio.pio, scr.pio.sm);
  pio_sm_exec(scr.pio.pio, scr.pio.sm, pio_encode_jmp(scr.pio.offset));

  irq_frame.frame = scr.frame;

  // Clear the IRQ
  dma_channel_acknowledge_irq0(scr.dma.channel);

  // Attempt to push a new frame, or drop a frame
  // and update the dropped frame counter
  if (queue_try_add(&frame_queue, &irq_frame)) {
    // Update to the next frame.
    scr.frame = (scr.frame + 1) % FRAME_BUFFER_LENGTH;

    irq_frame.dropped = 0;
  } else {
    irq_frame.dropped++;
  }

  irq_frame.start = get_absolute_time();
 
  // Start the next frame capture.
  pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, true);

  // start the DMA transfer;
  dma_channel_set_write_addr(
      scr.dma.channel,
      &(scr.pixels[scr.frame]),
      true);
}

static inline void frame_capture() {
  static absolute_time_t last_start;
  absolute_time_t start, end;
  queue_frame_t frame;
  uint8_t count = 0;

  while(true) {
    queue_remove_blocking(&frame_queue, &frame);
    start = get_absolute_time();

    // Output a frame to USB 
    dump_frame(scr.pixels[frame.frame]);
    end = get_absolute_time();

/*    printf(
      "Frame: %d Dropped: %d Capture Time: %" PRIu64 "us Frame Time: %" PRIu64 "us Display Time %" PRIu64"us\n" ,
      frame.frame,
      frame.dropped,
      absolute_time_diff_us(frame.start, frame.end),
      absolute_time_diff_us(last_start, frame.start),
      absolute_time_diff_us(start, end)
    );*/

    last_start = frame.start;
    }
}

// Write data to the the high speed serial port.
// Warning! Not thread safe!
void __not_in_flash_func(frame_write)(const char buf[], uint32_t count) {
  // Dump the write if we're not ready.
  if (!tud_ready()) {
    return;
  }
 
  // Loop until we've written everything
  // This is roughly a copy of what stdio_udb does.
  for (uint32_t i = 0; i < count;) {
    uint32_t remaining = count - i;
    uint32_t available = tud_cdc_n_write_available(1); 
    
    // Clamp the number of bytes to write to what we can
    // write.
    if (remaining > available) {
      remaining = available;
    }

    // If we have data to write, write it!
    if (remaining) {
      uint32_t written = tud_cdc_n_write(1, buf + i, remaining);
      i += written;
    }

    tud_task();
    tud_cdc_write_flush();

    if (!tud_ready()) {
      break;
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
      &(vga_mode_list[MODE_640X480_60]),
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

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  // zero out our frame buffer
  memset((void *)buffer, 0x00, sizeof(buffer));
  memset((void *)scr.pixels, 0x00, sizeof(scr.pixels));

for(int i=0; i < 80 ; i ++) {
  buffer[i] = 0xF1F2F4F8;
  buffer[i+80] = 0x1F2F4F8F;
  buffer[i+160] = 0xF1F2F4F8;
  buffer[i+240] = 0x1F2F4F8F;

}

  // tusb setup
//  tud_init(BOARD_TUD_RHPORT);

  stdio_init_all();

//  in_frame_pio_init(&scr.pio, D0);
//  in_frame_dma_init(&scr);

  queue_init(&frame_queue, sizeof(queue_frame_t), FRAME_COUNT);

//  multicore_launch_core1(frame_capture);
  multicore_launch_core1(vga_core);
//    vga_core();

  // Start capturing frames
//  frame_capture_irq();
  
//  frame_capture();
  while(true) {

    vga_dump_status();

    tight_loop_contents();
  }
}
