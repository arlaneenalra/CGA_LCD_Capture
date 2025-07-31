#include "vga_internal.h"

#include "hardware/dma.h"
#include "hardware/irq.h"

/**
 * Control blocks designed around Alias 1
 */
typedef struct vga_ctrl_blk_type {
  uint32_t ctrl;
  void volatile *read_addr;
} vga_ctrl_blk_t;

void vga_dma_init() {
  volatile static vga_pio_line_burst_t vga_line_burst;
  volatile static void *vga_burst_addr = &vga_line_burst;

  static vga_ctrl_blk_t ctrl_blks[VGA_HEIGHT];
  volatile static void *ctrl_blks_addr = &ctrl_blks[0];


  uint32_t line_size = VGA_FRAME_LINE_SIZE(vga.mode->h.visible);

  // Setup the line burst that we need to send every line 
  vga_line_burst.v_back_porch = vga.mode->v.back_porch;
  vga_line_burst.v_visible = vga.mode->v.visible - 1;
  vga_line_burst.h_back_porch = vga.mode->h.back_porch;
  vga_line_burst.h_visible = vga.mode->h.visible - 1;

  // Setup Control blocks
  // Allocate dma channels upfront since we need to know both channels
  // ahead of time.
  vga.burst_dma = dma_claim_unused_channel(true);
  vga.pixel_dma = dma_claim_unused_channel(true);
  vga.reset_dma = dma_claim_unused_channel(true);
  vga.ctrl_dma = dma_claim_unused_channel(true);
  vga.ctrl_reset_dma = dma_claim_unused_channel(true);

  dma_channel_config ctrl_reset_cfg = dma_channel_get_default_config(
      vga.ctrl_reset_dma);

  channel_config_set_transfer_data_size(&ctrl_reset_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&ctrl_reset_cfg, false);
  channel_config_set_write_increment(&ctrl_reset_cfg, false);
  channel_config_set_high_priority(&ctrl_reset_cfg, true);
  channel_config_set_irq_quiet(&ctrl_reset_cfg, true);  

  dma_channel_configure(
      vga.ctrl_reset_dma,
      &ctrl_reset_cfg,
      &(dma_hw->ch[vga.ctrl_dma].al3_read_addr_trig),
      &ctrl_blks_addr,
      1,
      false); 

  // Setup the configuration for the pixel channel 
  dma_channel_config pixel_cfg = dma_channel_get_default_config(vga.pixel_dma);

  channel_config_set_transfer_data_size(&pixel_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&pixel_cfg, true);
  channel_config_set_write_increment(&pixel_cfg, false);
  channel_config_set_dreq(
      &pixel_cfg,
      pio_get_dreq(vga.pio.pio, vga.pio.sm, true));
  channel_config_set_irq_quiet(&pixel_cfg, true); 
  channel_config_set_high_priority(&pixel_cfg, true);
  channel_config_set_chain_to(&pixel_cfg, vga.ctrl_dma);

  // First, build the control blocks for the frame
  for (int i = 0; i < VGA_HEIGHT;  i++) {
    ctrl_blks[i].read_addr = &(vga.frame_buf[i * line_size]); 
    ctrl_blks[i].ctrl = pixel_cfg.ctrl;
  }

  // The last line should reset the control block channel
  channel_config_set_chain_to(
      &(pixel_cfg),
      vga.ctrl_reset_dma);
  
  ctrl_blks[VGA_HEIGHT - 1].ctrl = pixel_cfg.ctrl;

  dma_channel_config ctrl_cfg = dma_channel_get_default_config(vga.ctrl_dma);

  channel_config_set_transfer_data_size(&ctrl_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&ctrl_cfg, true);
  channel_config_set_write_increment(&ctrl_cfg, true);
  channel_config_set_ring(&ctrl_cfg, true, 3);
  channel_config_set_high_priority(&ctrl_cfg, true);
  channel_config_set_irq_quiet(&ctrl_cfg, true);  
  channel_config_set_chain_to(&ctrl_cfg, vga.reset_dma);

  dma_channel_configure(
      vga.ctrl_dma,
      &ctrl_cfg,
      &(dma_hw->ch[vga.pixel_dma].al1_ctrl),
      &ctrl_blks[0],
      2,
      false); 

  // Setup the configuration for the reset channel 
  // Used to reset the read address of the first DMA channel for some reason
  // a ring buffer is not working there.
  dma_channel_config reset_cfg = dma_channel_get_default_config(vga.reset_dma);

  channel_config_set_transfer_data_size(&reset_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&reset_cfg, false);
  channel_config_set_write_increment(&reset_cfg, false);
  channel_config_set_high_priority(&reset_cfg, true);
  channel_config_set_irq_quiet(&reset_cfg, true);  

  dma_channel_configure(
      vga.reset_dma,
      &reset_cfg,
      &(dma_hw->ch[vga.burst_dma].al3_read_addr_trig),
      &vga_burst_addr,
      1,
      false); 

  // Setup the configuration for the line burst channel
  dma_channel_config burst_cfg = dma_channel_get_default_config(vga.burst_dma);

  channel_config_set_transfer_data_size(&burst_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&burst_cfg, true);
  channel_config_set_write_increment(&burst_cfg, false);
  channel_config_set_dreq(
      &burst_cfg,
      pio_get_dreq(vga.pio.pio, vga.pio.sm, true));
  channel_config_set_irq_quiet(&burst_cfg, true);  
  channel_config_set_high_priority(&burst_cfg, true);
  channel_config_set_chain_to(&burst_cfg, vga.pixel_dma);

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
      0,
      line_size,
      false);

}

void vga_dma_enable() {
  dma_channel_start(vga.ctrl_reset_dma);
}

