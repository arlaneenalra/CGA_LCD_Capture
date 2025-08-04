#include "lcd_internal.h"

scr_t scr;

void in_frame_init(frame_handler_t handler, uint base_pin, irq_num_t dma_irq, uint oe_pin) {
  memset((void *)scr.pixels, 0x00, sizeof(scr.pixels));
  scr.handler = handler;
  scr.oe_pin = oe_pin;

  gpio_init(oe_pin);
  gpio_put(oe_pin, true);
  gpio_set_dir(oe_pin, GPIO_OUT);

  in_frame_pio_init(&scr.pio, base_pin);
  in_frame_dma_init(dma_irq);
}

void in_frame_pio_init(pio_alloc_t *pio_alloc,  uint base_pin) {

  bool rc = pio_claim_free_sm_and_add_program(
    &in_frame_program,
    &(pio_alloc->pio),
    &(pio_alloc->sm),
    &(pio_alloc->offset));

  hard_assert(rc);


 for (uint i = base_pin; i < IN_FRAME_PINS; i++) {
    pio_gpio_init(pio_alloc->pio, i+base_pin);

    gpio_disable_pulls(i + base_pin);
  }

  pio_sm_set_consecutive_pindirs(
      pio_alloc->pio,
      pio_alloc->sm,
      base_pin, IN_FRAME_PINS, false);


  pio_sm_config c = in_frame_program_get_default_config(pio_alloc->offset);

  sm_config_set_clkdiv(&c, 1.0f);

  sm_config_set_in_pins(&c, base_pin);
  /*sm_config_set_in_pin_base(&c, base_pin);
  sm_config_set_in_pin_count(&c, IN_FRAME_PINS);a*/

  pio_sm_init(pio_alloc->pio, pio_alloc->sm, pio_alloc->offset, &c);
 }

void in_frame_dma_init(irq_num_t dma_irq) {
  dma_alloc_t *dma = &(scr.dma);
  pio_alloc_t *pio = &(scr.pio);

  dma->dma_irq = dma_irq;

  queue_init(&scr.frame_queue, sizeof(queue_frame_t), FRAME_COUNT);

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
  dma_irqn_set_channel_enabled(dma_irq - DMA_IRQ_0, dma->channel, true);
  irq_set_exclusive_handler(dma_irq, frame_capture_irq);
  irq_set_enabled(dma_irq, true);
}

void frame_capture_irq() {
  static queue_frame_t irq_frame = {0, 0};

  // cleanly (re)start the PIO
  pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, false);

  pio_sm_restart(scr.pio.pio, scr.pio.sm);
  pio_sm_exec(scr.pio.pio, scr.pio.sm, pio_encode_jmp(scr.pio.offset));

  irq_frame.frame = scr.frame;

  // Clear the IRQ
  dma_irqn_acknowledge_channel(scr.dma.dma_irq - DMA_IRQ_0, scr.dma.channel);

  // Attempt to push a new frame, or drop a frame
  // and update the dropped frame counter
  if (queue_try_add(&scr.frame_queue, &irq_frame)) {
    // Update to the next frame.
    scr.frame = (scr.frame + 1) % FRAME_BUFFER_LENGTH;

    irq_frame.dropped = 0;
  } else {
    irq_frame.dropped++;
  }

  // Start the next frame capture.
  pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, true);

  // start the DMA transfer;
  dma_channel_set_write_addr(
      scr.dma.channel,
      &(scr.pixels[scr.frame]),
      true);
}

void frame_capture() {
  static absolute_time_t last_start;
  queue_frame_t frame;
  uint8_t count = 0;

  gpio_put(scr.oe_pin, false);

  while(true) {
    queue_remove_blocking(&scr.frame_queue, &frame);

    // Output a frame to USB 
    scr.handler(scr.pixels[frame.frame]);
  }
}

