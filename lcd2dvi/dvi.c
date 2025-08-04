/**
 * Heavily based on the moon example from:
 * https://github.com/Wren6991/PicoDVI/tree/7af20b2742c3dd0d7e7d3224078085ddea04a85f/software/apps/moon
 *
 *
 **/

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"


#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"
#include "tmds_encode_1bpp.pio.h"
#include "tmds_encode.h"

#include "lcd2dvi.h"

struct dvi_inst dvi0;

volatile dvi_frame_buf_t frame_buf;

void local_dvi_init() {
	vreg_set_voltage(VREG_VSEL);
	sleep_ms(10);
	set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

	dvi0.timing = &DVI_TIMING;
	dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

	// Set up extra SM, and DMA channels, to offload TMDS encode if necessary
	// (note there are two build targets for this app, called `moon` and
	// `moon_pio_encode`, so a simple `make all` will get you both binaries)
#ifdef USE_PIO_TMDS_ENCODE
	PIO encode_pio = dvi0.ser_cfg.pio;
	uint encode_sm = pio_claim_unused_sm(encode_pio, true);
	tmds_encode_1bpp_init(encode_pio, encode_sm);

	uint dma_chan_put = dma_claim_unused_channel(true);
	dma_channel_config c = dma_channel_get_default_config(dma_chan_put);
	channel_config_set_dreq(&c, pio_get_dreq(encode_pio, encode_sm, true));
	dma_channel_configure(dma_chan_put, &c,
		&encode_pio->txf[encode_sm],
		NULL,
		FRAME_WIDTH / 32,
		false
	);

	uint dma_chan_get = dma_claim_unused_channel(true);
	c = dma_channel_get_default_config(dma_chan_get);
	channel_config_set_dreq(&c, pio_get_dreq(encode_pio, encode_sm, false));
	channel_config_set_write_increment(&c, true);
	channel_config_set_read_increment(&c, false);
	dma_channel_configure(dma_chan_get, &c,
		NULL,
		&encode_pio->rxf[encode_sm],
		FRAME_WIDTH / DVI_SYMBOLS_PER_WORD,
		false
	);
#endif

}

void local_dvi_core() {
	dvi_register_irqs_this_core(&dvi0, DVI_IRQ);
	dvi_start(&dvi0);

	while (true) {
		for (uint y = 0; y < FRAME_HEIGHT; ++y) {
			const uint32_t *colourbuf = &((const uint32_t*)frame_buf)[y * FRAME_WIDTH / 32];
			uint32_t *tmdsbuf;
			queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
#ifndef USE_PIO_TMDS_ENCODE
			tmds_encode_1bpp(colourbuf, tmdsbuf, FRAME_WIDTH);
#else
			dma_channel_set_read_addr(dma_chan_put, colourbuf, true);
			dma_channel_set_write_addr(dma_chan_get, tmdsbuf, true);
			dma_channel_wait_for_finish_blocking(dma_chan_get);
#endif
			queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
		}
	}
}
