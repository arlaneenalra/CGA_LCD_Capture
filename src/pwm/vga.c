#include <stdio.h>

#include "vga_internal.h"

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

vga_pwm_t vga;

extern vga_pio_line_burst_t vga_line_burst;

void vga_init(
    vga_mode_t *vga_mode,
    volatile uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin) {

  vga.mode = vga_mode;
  vga.frame_buf = frame_buf;
  
  vga_pwm_init(hsync_pin, vsync_pin);
  vga_pio_init(vga_mode, vsync_pin, rgb_pin);

  vga_dma_init();
}

void vga_enable() {
  vga_pwm_enable(&vga);
  vga_pio_enable(&(vga.pio));

  // Start it all off
  vga_dma_enable();
}

void vga_dump_status() {
  static uint32_t time = 0;
  uint32_t pc = pio_sm_get_pc(vga.pio.pio, vga.pio.sm) - vga.pio.offset;
  printf(
      "PIO: fifo: %d pc: %d sm: %d DMA: "
      "pixel: %d pixel_tc: %d pixel_raddr: %d buffer_addr: %d - %d "
      "burst: %d burst_tc: %d burst_raddr: %d "
      "ctrl: %"PRIu32" %"PRIu32", %"PRIu32" "
      "ctrl_reset: %"PRIu32" %"PRIu32", %"PRIu32"\n",
      pio_sm_get_tx_fifo_level(vga.pio.pio, vga.pio.sm),
      pc, vga.pio.sm,
      dma_channel_is_busy(vga.pixel_dma),
      dma_hw->ch[vga.pixel_dma].transfer_count,
      dma_hw->ch[vga.pixel_dma].read_addr,

      vga.frame_buf, ((void *)vga.frame_buf) + sizeof(vga_frame_buf_t),
      dma_channel_is_busy(vga.burst_dma),
      dma_hw->ch[vga.burst_dma].transfer_count,
      dma_hw->ch[vga.burst_dma].read_addr,

      dma_channel_is_busy(vga.ctrl_dma),
      dma_hw->ch[vga.ctrl_dma].transfer_count,
      dma_hw->ch[vga.ctrl_dma].read_addr,

      dma_channel_is_busy(vga.ctrl_reset_dma),
      dma_hw->ch[vga.ctrl_reset_dma].transfer_count,
      dma_hw->ch[vga.ctrl_reset_dma].read_addr);
}
