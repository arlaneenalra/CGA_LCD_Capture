#include "vga_internal.h"

#include "hardware/dma.h"
#include "hardware/irq.h"

#define V_FRAME_BLK 0
#define V_LINE_BLK 2 

vga_pwm_t *vga_pwm;

void vga_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin) {

  vga->mode = vga_mode;
  vga->frame_buf = frame_buf;
  vga_pwm = vga;

  vga_pwm_init(vga, vga_mode, hsync_pin, vsync_pin);
  vga_pio_init(&(vga->pio), vga_mode, vsync_pin, rgb_pin);

  vga_dma_init(vga);
}

void vga_enable(vga_pwm_t *vga) {
  vga_pwm_enable(vga);

  // Start everything off!
  vga_dma_irq();

  vga_pio_enable(&(vga->pio));
}


void vga_dma_init(vga_pwm_t *vga) {
  vga_mode_t *mode = vga->mode;

  vga->line = 0;

  vga->pixel_dma = dma_claim_unused_channel(true);
  int pixel_dma = vga->pixel_dma;

  // Setup the configuration for the pixel channel 
  dma_channel_config pixel_cfg = dma_channel_get_default_config(pixel_dma);

  channel_config_set_transfer_data_size(&pixel_cfg, DMA_SIZE_32);
  channel_config_set_read_increment(&pixel_cfg, true);
  channel_config_set_write_increment(&pixel_cfg, false);
  channel_config_set_dreq(
      &pixel_cfg,
      PIO_DREQ_NUM(vga->pio.pio, vga->pio.sm, true));
  //channel_config_set_irq_quiet(&pixel_cfg, false); 

  dma_channel_configure(
      pixel_dma,
      &pixel_cfg,
      &(vga->pio.pio->txf[vga->pio.sm]),
      vga->frame_buf,
      VGA_FRAME_LINE_SIZE(mode->h.visible),
      false);

  // Enable dma
  dma_channel_set_irq1_enabled(pixel_dma, true);
  irq_set_exclusive_handler(DMA_IRQ_1, vga_dma_irq);
  irq_set_enabled(DMA_IRQ_1, true);
}

void __not_in_flash_func(vga_dma_irq)() {
  static bool fired = false;
  vga_mode_t *mode = vga_pwm->mode;
 
  if (fired) {
    gpio_put(PICO_DEFAULT_LED_PIN, true);
  }

  fired = true;
 
  dma_channel_acknowledge_irq1(vga_pwm->pixel_dma);

  if (vga_pwm->line <= 0) {
    vga_pwm->line = mode->v.visible;  

    pio_sm_put(vga_pwm->pio.pio, vga_pwm->pio.sm, mode->v.back_porch);
    pio_sm_put(vga_pwm->pio.pio, vga_pwm->pio.sm, mode->v.visible);

  
    // Reset the pixel dma 
    dma_channel_set_read_addr(
      vga_pwm->pixel_dma,
      vga_pwm->frame_buf,
      false);
  }

  pio_sm_put(vga_pwm->pio.pio, vga_pwm->pio.sm, mode->h.back_porch);
  pio_sm_put(vga_pwm->pio.pio, vga_pwm->pio.sm, mode->h.visible);

  dma_channel_start(vga_pwm->pixel_dma);
}

