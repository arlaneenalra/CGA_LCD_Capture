#include "vga.h"

#include "hardware/dma.h"

typedef struct vga_dma_ctrl_type {
  volatile void *read_addr;
  volatile void *write_addr;
  uint32_t count;
  dma_channel_config ctrl;

} vga_dma_ctrl_t;

typedef struct vga_frame_type {
  uint32_t back_porch;
  uint32_t visible; 
} vga_frame_t;

vga_frame_t v_frame;
vga_frame_t h_line;
vga_dma_ctrl_t ctrl_blks[3];

#define V_FRAME_BLK 0
#define V_LINE_BLK 2 

uint8_t ctrl_dma;
uint8_t data_dma;
uint8_t pixel_dma;

uint32_t line = 0;

static inline void vga_dma_chain(
    uint8_t channel,
    volatile void *write_ptr,
    bool write_inc,
    volatile void *read_ptr, 
    bool read_inc,
    uint transfer_count,    
    uint8_t chain);

void vga_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin) {

  vga_pwm_init(vga, vga_mode, hsync_pin, vsync_pin);
  vga_pio_init(&(vga->pio), vsync_pin, rgb_pin);

  vga_dma_init(&(vga->pio), vga_mode, frame_buf);

}

void vga_enable(vga_pwm_t *vga) {
  vga_pio_enable(&(vga->pio));
  vga_pwm_enable(vga);

  // Start everything off!
  vga_dma_irq();
}


void vga_dma_init(rgb_pio_t *rgb_pio, vga_mode_t *mode, uint32_t *frame_buf) {

  // We're going to need this several times
  volatile void *pio_tx_buf = &(rgb_pio->pio->txf[rgb_pio->sm]);

  // Setup counters for the vertical back porch and number
  // of visible lines.
  v_frame.back_porch = mode->v.back_porch;
  v_frame.visible = mode->v.visible;

  h_line.back_porch = mode->h.back_porch;
  h_line.visible = mode->h.visible;

  ctrl_dma = dma_claim_unused_channel(true);
  data_dma = dma_claim_unused_channel(true);
  pixel_dma = dma_claim_unused_channel(true);

  // Setup the configuration for the pixel channel 
  dma_channel_config pixel_cfg = dma_channel_get_default_config(data_dma);

  channel_config_set_transfer_data_size(&pixel_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&pixel_cfg, true);
  channel_config_set_write_increment(&pixel_cfg, false);
  channel_config_set_dreq(
      &pixel_cfg,
      PIO_DREQ_NUM(rgb_pio->pio, rgb_pio->sm, true)); 
  channel_config_set_irq_quiet(&pixel_cfg, false); 

  dma_channel_configure(
      pixel_dma,
      &pixel_cfg,
      pio_tx_buf,
      frame_buf,
      VGA_FRAME_LINE_SIZE(h_line.visible),
      false);

  // Setup the configuration for the data channel 
  dma_channel_config data_cfg = dma_channel_get_default_config(data_dma);

  channel_config_set_transfer_data_size(&data_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&data_cfg, true);
  channel_config_set_write_increment(&data_cfg, false);
  /*channel_config_set_dreq(
      &data_cfg,
      PIO_DREQ_NUM(rgb_pio->pio, rgb_pio->sm, true)); */
  channel_config_set_irq_quiet(&data_cfg, true); 
  channel_config_set_chain_to(&data_cfg, ctrl_dma);

  // Reset the pixel data dma 
  ctrl_blks[0].read_addr = frame_buf;
  ctrl_blks[0].write_addr = &(dma_hw->ch[pixel_dma].read_addr);
  ctrl_blks[0].count = 1;
  ctrl_blks[0].ctrl.ctrl = data_cfg.ctrl;

  // Write the vertical back porch and visible 
  ctrl_blks[1].read_addr = &(v_frame);
  ctrl_blks[1].write_addr = pio_tx_buf;
  ctrl_blks[1].count = 2;
  ctrl_blks[1].ctrl.ctrl = data_cfg.ctrl;

  // Write the line back porch and visible
  ctrl_blks[2].read_addr = &(h_line);
  ctrl_blks[2].write_addr = pio_tx_buf;
  ctrl_blks[2].count = 2;
  ctrl_blks[2].ctrl.ctrl = data_cfg.ctrl;
  
  channel_config_set_chain_to(&(ctrl_blks[2].ctrl), pixel_dma);

  // Enable IRQs on this one
  channel_config_set_irq_quiet(&(ctrl_blks[2].ctrl), false);

  dma_channel_config ctrl_cfg = dma_channel_get_default_config(ctrl_dma);

  channel_config_set_transfer_data_size(&ctrl_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&ctrl_cfg, true);
  channel_config_set_write_increment(&ctrl_cfg, true);
  channel_config_set_ring(&ctrl_cfg, true, 4);
  channel_config_set_irq_quiet(&ctrl_cfg, true);
  channel_config_set_chain_to(&ctrl_cfg, data_dma);

  dma_channel_configure(
      ctrl_dma,
      &ctrl_cfg,
      &(dma_hw->ch[data_dma].read_addr),
      &(ctrl_blks[0]),
      4,
      false);

  
  // Enable dma
  dma_set_irq1_channel_mask_enabled(pixel_dma, true);
  irq_set_exclusive_handler(DMA_IRQ_1, vga_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);

}

void __not_in_flash_func(vga_dma_irq)() {

  // Either trigger a new frame or a new line.
  dma_channel_set_read_addr(ctrl_dma, (line > 0) ? &(ctrl_blks[V_FRAME_BLK]) : &(ctrl_blks[V_LINE_BLK]), true);

  line--;
  if (line <= 0) {
    line = v_frame.visible;  
  }
}

static inline void vga_dma_chain(
    uint8_t channel,
    volatile void *write_ptr,
    bool write_inc,
    volatile void *read_ptr, 
    bool read_inc,
    uint transfer_count,    
    uint8_t chain) {

  dma_channel_config cfg = dma_channel_get_default_config(channel);

  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&cfg, read_inc);
  channel_config_set_write_increment(&cfg, write_inc);
  channel_config_set_chain_to(&cfg, chain);

  dma_channel_configure(
      channel, &cfg, write_ptr, read_ptr, transfer_count, false);
}
