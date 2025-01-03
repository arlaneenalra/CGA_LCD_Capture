#include <stdio.h>
#include <string.h>

#include <inttypes.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "hardware/clocks.h"

#include "lcd.pio.h"
#include "lcd.h"

scr_t scr;

void dump_frame(scr_frame_buf_t pixels) {
  uint32_t x, y, j, idx;
  uint32_t cell;
  uint32_t mask;
  char line[641];

  memset(line, 0, sizeof(line));

  for (y = 0; y < 200; y++) {
    for (x = 0; x < 20; x++) {
      cell = pixels[x + (y * SCR_LINE_SIZE)];

      // Draw pixels
      mask = 0x80000000;
      for (j = 0; j < 32; j++) {
        idx = (x * 32) + j;
        if (cell & mask) {
          line[idx] = '*';
        } else {
          line[idx] = ' ';
        }
        mask = mask >> 1; 
      }
    }
    puts(line);
  }
  putchar('\n');
}

void in_frame_pio_init(pio_alloc_t *pio_alloc,  uint base_pin) {

  for (uint i = 0; i < DOT_CLOCK; i++) {
    gpio_init(i + base_pin);
    //gpio_set_input_hysteresis_enabled(i + base_pin, false);
  }

  bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(
    &in_frame_program,
    &(pio_alloc->pio),
    &(pio_alloc->sm),
    &(pio_alloc->offset),
    base_pin,
    IN_FRAME_PINS,
    true);

  hard_assert(rc);

  pio_sm_set_consecutive_pindirs(
    pio_alloc->pio,
    pio_alloc->sm,
    base_pin,
    IN_FRAME_PINS,
    false);
  pio_sm_config c = in_frame_program_get_default_config(pio_alloc->offset);

  sm_config_set_clkdiv(&c, 1.0f);

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

}

void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
#ifdef CLOCKS_FC0_SRC_VALUE_CLK_RTC
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);
#endif

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
#ifdef CLOCKS_FC0_SRC_VALUE_CLK_RTC
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);
#endif

    // Can't measure clk_ref / xosc as it is the ref
}

int main() {
  absolute_time_t start, end;

//  set_sys_clock_khz(250000, true);

  stdio_init_all();

  in_frame_pio_init(&scr.pio, D0);
  in_frame_dma_init(&scr);


  printf("START:\n");

  while(true) {

    printf("FRAME:\n");
    start = get_absolute_time();

    // cleanly (re)start the PIO
    pio_sm_restart(scr.pio.pio, scr.pio.sm);
    pio_sm_exec(scr.pio.pio, scr.pio.sm, pio_encode_jmp(scr.pio.offset));

    pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, true);

    // start the DMA transfer;
    dma_channel_set_read_addr(
        scr.dma.channel,
        &(scr.pio.pio->rxf[scr.pio.sm]),
        false);
    dma_channel_set_write_addr(
        scr.dma.channel,
        &(scr.pixels[scr.frame]),
        true);


    // wait for dma transfer to finish
    dma_channel_wait_for_finish_blocking(scr.dma.channel);
    pio_sm_set_enabled(scr.pio.pio, scr.pio.sm, false);

    scr.frame = 1 - scr.frame;

    end = get_absolute_time();

    dump_frame(scr.pixels[1 - scr.frame]);

    printf("Frame: %d Time: %" PRIu64 "us\n", scr.frame, absolute_time_diff_us(start, end));
  }
}
