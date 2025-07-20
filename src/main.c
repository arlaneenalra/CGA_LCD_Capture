//#define PICO_STDIO_USB_TASK_INTERVAL_US 100

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

#include "vga.h"


scr_t scr;
vga_t vga;

uint32_t zero_line[VGA_RGB_ACTIVE];

queue_t frame_queue;

void dump_frame(scr_frame_buf_t pixels) {
  uint32_t vga_index = 0;
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
  }
}

void vga_pio_init(vga_t *vga, uint base_pin) {
  pio_alloc_t *hsync = &(vga->hsync);
  pio_alloc_t *vsync = &(vga->vsync);
  pio_alloc_t *rgb = &(vga->rgb);

  bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
    &hsync_program,
    &(hsync->pio),
    &(hsync->sm),
    &(hsync->offset),
    base_pin + VGA_HSYNC,
    1,
    true);

  hard_assert(rc);

  rc = pio_claim_free_sm_and_add_program_for_gpio_range(
    &vsync_program,
    &(vsync->pio),
    &(vsync->sm),
    &(vsync->offset),
    base_pin + VGA_VSYNC,
    1,
    true);

  hard_assert(rc);

  rc = pio_claim_free_sm_and_add_program_for_gpio_range(
    &rgb_program,
    &(rgb->pio),
    &(rgb->sm),
    &(rgb->offset),
    base_pin + VGA_LO_GREEN,
    4,
    true);

  hard_assert(rc);

  hsync_program_init(hsync->pio, hsync->sm, hsync->offset, (base_pin + VGA_HSYNC));
  vsync_program_init(vsync->pio, vsync->sm, vsync->offset, (base_pin + VGA_VSYNC));
  rgb_program_init(rgb->pio, rgb->sm, rgb->offset, (base_pin + VGA_LO_GREEN));

  pio_sm_put_blocking(hsync->pio, hsync->sm, VGA_HSYNC_ACTIVE + VGA_HSYNC_FRONTPORCH);
  pio_sm_put_blocking(vsync->pio, vsync->sm, VGA_VSYNC_ACTIVE);

//  pio_sm_put_blocking(rgb->pio, rgb->sm, 41);
  pio_sm_put_blocking(rgb->pio, rgb->sm, VGA_RGB_ACTIVE * 2);

  pio_sm_set_enabled(hsync->pio, hsync->sm, true);
  pio_sm_set_enabled(vsync->pio, vsync->sm, true);
  pio_sm_set_enabled(rgb->pio, rgb->sm, true);
}

void vga_dma_init(vga_t *vga) {
  dma_alloc_t *dma = &(vga->dma);
  pio_alloc_t *rgb = &(vga->rgb);

  vga->line = 0;
  //vga->line_ptr = VGA_LINE_ADDR(*vga, 0); 
  vga->line_ptr = zero_line; 


  // Do basic setup.
  dma->channel = dma_claim_unused_channel(true);
  dma->config = dma_channel_get_default_config(dma->channel);
  
  channel_config_set_transfer_data_size(&(dma->config), DMA_SIZE_32);
  channel_config_set_read_increment(&(dma->config), true);
  channel_config_set_write_increment(&(dma->config), false);
  channel_config_set_dreq(
      &(dma->config),
      PIO_DREQ_NUM(rgb->pio, rgb->sm, true)
  );

  dma_channel_configure(
      dma->channel,
      &(dma->config),
      &(rgb->pio->txf[rgb->sm]),
      vga->line_ptr,
      VGA_DMA_TRANSFERS,
      false);

  dma_channel_set_irq1_enabled(dma->channel, true);
  irq_set_exclusive_handler(DMA_IRQ_1, vga_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);
}

void vga_dma_irq() {
  // Acknowledge the IRQ
  dma_channel_acknowledge_irq1(vga.dma.channel);

  vga.line = (vga.line + 1);
  if (vga.line == VGA_FRAME_LINES) {

    pio_sm_set_enabled(vga.rgb.pio, vga.rgb.sm, false);

    vga.line = 0;
    vga.line_ptr = zero_line;

    // Reset the RGB PIO
    pio_sm_restart(vga.rgb.pio, vga.rgb.sm);
    pio_sm_clear_fifos(vga.rgb.pio, vga.rgb.sm);
    pio_sm_restart(vga.rgb.pio, vga.rgb.sm);
    pio_sm_exec(vga.rgb.pio, vga.rgb.sm, pio_encode_jmp(vga.rgb.offset));

    pio_sm_put_blocking(vga.rgb.pio, vga.rgb.sm, VGA_RGB_ACTIVE * 2);
    pio_sm_set_enabled(vga.rgb.pio, vga.rgb.sm, true);
  }

  dma_channel_set_read_addr(
      vga.dma.channel,
      vga.line_ptr,
      true);

/*  vga.line = (vga.line + 1);
  if (vga.line % VGA_FRAME_LINES == 0) {
    vga.line = 0;

  }*/

  if (vga.line <= VGA_VSYNC_BACKPORCH || vga.line > VGA_VIDEO_LINES) {
    vga.line_ptr = zero_line;
  } else {
    vga.line_ptr = VGA_LINE_ADDR(vga, VGA_VSYNC_BACKPORCH);
  }    
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
      PIO_DREQ_NUM(pio->pio, pio->sm, false)
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

// Ignores any data sent to the device for now.
void cdc_task() {
  static char buf[64];
  if (tud_cdc_n_available(1)) {
    tud_cdc_n_read(1, buf, sizeof(buf));
  }
  if (tud_cdc_n_available(0)) {
    tud_cdc_n_read(0, buf, sizeof(buf));
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
  vga_pio_init(&vga, VGA_BASE_PIN);
  vga_dma_init(&vga);
  vga_dma_irq();

  while(true) {
    tight_loop_contents();
  }
}

int main() {
  ldo_pwm_mode();

  set_sys_clock_khz(250000, true);

  for (int i = 0; i < VGA_RGB_ACTIVE; i++) {
    zero_line[i] = 0x0;
  }

  for(int i=0; i<32000; i++) {
    vga.scr[i] = 0xF0F0F0F0;
//    vga.scr[i] = 0xFFFF0000;
  }

for(int i=0; i < 80; i ++) {
  vga.scr[i] = 0xFFFFFFFF;
  vga.scr[i + (399 * 80)] = 0xFFFFFFFF;
}

  // tusb setup
//  tud_init(BOARD_TUD_RHPORT);

  stdio_init_all();

  in_frame_pio_init(&scr.pio, D0);
  in_frame_dma_init(&scr);

  queue_init(&frame_queue, sizeof(queue_frame_t), FRAME_COUNT);

//  multicore_launch_core1(frame_capture);
  multicore_launch_core1(vga_core);

  // Start capturing frames
  frame_capture_irq();
 
  
//  frame_capture();
  while(true) {
    //tud_task();
    //cdc_task();
    tight_loop_contents();
  }
}
