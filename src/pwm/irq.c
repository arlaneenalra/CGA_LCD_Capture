#include "vga_internal.h"

#include "stdio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"

void vga_irq_init() {
  pio_interrupt_clear(vga.pio.pio, vga.pio.sm);

  int irq_num = pio_get_irq_num(vga.pio.pio, 1);

   // See: https://github.com/raspberrypi/pico-examples/blob/84e8d489ca321a4be90ee49e36dc29e5c645da08/pio/i2c/i2c.pio#L115
  pio_set_irq1_source_enabled(
      vga.pio.pio,
      (enum pio_interrupt_source) ((uint) pis_interrupt0 + vga.pio.sm),
      true);
  pio_interrupt_clear(vga.pio.pio, vga.pio.sm);
  irq_set_exclusive_handler(irq_num, vga_irq_handler);
}

void vga_irq_enable() {
  int irq_num = pio_get_irq_num(vga.pio.pio, 1);

  irq_set_enabled(irq_num, true);
}

//void __not_in_flash_func(vga_irq_handler)() {
void vga_irq_handler() {
  //pio_sm_set_enabled(vga.pio.pio, vga.pio.sm, false);
  vga_dma_reset();

  pio_sm_drain_tx_fifo(vga.pio.pio, vga.pio.sm);

  //pio_interrupt_clear(vga.pio.pio, vga.pio.sm);

  //vga_dma_enable();

  /*vga_dma_reset();

  // clear the interrupt
  //pio_sm_clear_fifos(vga.pio.pio, vga.pio.sm);
  pio_sm_restart(vga.pio.pio, vga.pio.sm);
  pio_sm_exec(vga.pio.pio, vga.pio.sm, pio_encode_jmp(vga.pio.offset));

  vga_dma_enable();

  pio_sm_set_enabled(vga.pio.pio, vga.pio.sm, true);*/
}
