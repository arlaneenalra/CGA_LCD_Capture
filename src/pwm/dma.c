#include "vga_internal.h"

#include "hardware/dma.h"
#include "hardware/irq.h"

volatile vga_pio_line_burst_t vga_line_burst;
volatile void *vga_burst_addr = &vga_line_burst;


dma_channel_config burst_cfg;
dma_channel_config pixel_cfg;
dma_channel_config reset_cfg;

void vga_dma_init() {

  vga.line = 0;

  // Setup the line burst that we need to send every line 
  vga_line_burst.v_back_porch = vga.mode->v.back_porch;
  vga_line_burst.v_visible = vga.mode->v.visible;
  vga_line_burst.h_back_porch = vga.mode->h.back_porch;
  vga_line_burst.h_visible = vga.mode->h.visible - 1;

  // Allocate dma channels upfront since we need to know both channels
  // ahead of time.
  vga.burst_dma = dma_claim_unused_channel(true);
  vga.pixel_dma = dma_claim_unused_channel(true);
  vga.reset_dma = dma_claim_unused_channel(true);

  // Setup the configuration for the line burst channel
  burst_cfg = dma_channel_get_default_config(vga.burst_dma);

  channel_config_set_transfer_data_size(&burst_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&burst_cfg, true);
  channel_config_set_write_increment(&burst_cfg, false);
  // The line burst is a repeated block.
  channel_config_set_ring(&burst_cfg, false, VGA_LINE_BURST_ALIGNMENT );
  channel_config_set_dreq(
      &burst_cfg,
      pio_get_dreq(vga.pio.pio, vga.pio.sm, true));
  channel_config_set_irq_quiet(&burst_cfg, true);  
  channel_config_set_high_priority(&burst_cfg, true);
  channel_config_set_chain_to(&burst_cfg, vga.pixel_dma);

/*  dma_channel_configure(
      vga.burst_dma,
      &burst_cfg,
      &(vga.pio.pio->txf[vga.pio.sm]),
      &vga_line_burst,
      VGA_LINE_BURST_SIZE,
      false);*/


  // Setup the configuration for the pixel channel 
  pixel_cfg = dma_channel_get_default_config(vga.pixel_dma);

  channel_config_set_transfer_data_size(&pixel_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&pixel_cfg, false);
  channel_config_set_write_increment(&pixel_cfg, false);
  channel_config_set_dreq(
      &pixel_cfg,
      pio_get_dreq(vga.pio.pio, vga.pio.sm, true));
  channel_config_set_irq_quiet(&pixel_cfg, true); 
  channel_config_set_high_priority(&pixel_cfg, true);
  channel_config_set_chain_to(&pixel_cfg, vga.reset_dma);

/*  dma_channel_configure(
      vga.pixel_dma,
      &pixel_cfg,
      &(vga.pio.pio->txf[vga.pio.sm]),
      vga.frame_buf,
      VGA_FRAME_LINE_SIZE(vga.mode->h.visible),
      false);*/

  // Setup the configuration for the reset channel 
  // Used to reset the read address of the first DMA channel for some reason
  // a ring buffer is not working there.
  reset_cfg = dma_channel_get_default_config(vga.reset_dma);

  channel_config_set_transfer_data_size(&reset_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&reset_cfg, false);
  channel_config_set_write_increment(&reset_cfg, false);
  channel_config_set_irq_quiet(&reset_cfg, true);  

/*  dma_channel_configure(
      vga.reset_dma,
      &reset_cfg,
      &(dma_hw->ch[vga.burst_dma].al3_read_addr_trig),
      //&(dma_hw->ch[vga.burst_dma].read_addr), // IRQ trigger
      &vga_burst_addr,
      1,
      false); */

  // Enable dma
  /*dma_channel_set_irq1_enabled(vga.pixel_dma, true);
  irq_set_exclusive_handler(DMA_IRQ_1, vga_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);*/
}

void vga_dma_reset() {
  // see: https://forums.raspberrypi.com/viewtopic.php?p=2020906&sid=c2379627c3741480155272ca188f6d65#p2020906
  dma_hw->abort =
    (1 << vga.pixel_dma) |
    (1 << vga.burst_dma) |
    (1 << vga.reset_dma);
  
  while (dma_hw->abort) {
    tight_loop_contents();
  }
  gpio_put(PICO_DEFAULT_LED_PIN, true);
}

void vga_dma_enable() {
  dma_channel_configure(
      vga.burst_dma,
      &burst_cfg,
      &(vga.pio.pio->txf[vga.pio.sm]),
      &vga_line_burst,
      VGA_LINE_BURST_SIZE,
      false);

  dma_channel_configure(
      vga.pixel_dma,
      &pixel_cfg,
      &(vga.pio.pio->txf[vga.pio.sm]),
      vga.frame_buf,
      VGA_FRAME_LINE_SIZE(vga.mode->h.visible),
      false);

  dma_channel_configure(
      vga.reset_dma,
      &reset_cfg,
      &(dma_hw->ch[vga.burst_dma].al3_read_addr_trig),
      &vga_burst_addr,
      1,
      true);

  //dma_channel_start(vga.reset_dma);
}

/*void __not_in_flash_func(vga_dma_irq)() {
  
  if (vga.line <= 0 ) {
    vga.line = vga_line_burst.h_visible;
    dma_channel_set_read_addr(
      vga.pixel_dma,
      vga.frame_buf,
      false);
  }

  wvga.line--;
  dma_channel_acknowledge_irq1(vga.pixel_dma); 

  dma_channel_set_read_addr(
      vga.burst_dma,
      &vga_line_burst,
      true);
}*/

