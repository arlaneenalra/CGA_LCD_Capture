#pragma once

#include "hardware/pio.h"
#include "hardware/dma.h"

#define SCR_WIDTH 640
#define SCR_HEIGHT 200

// Must be integer divisible by 8 and 32
#define SCR_PIXELS (SCR_WIDTH * SCR_HEIGHT)

// These displays are 1bpp right now, so the raw data coming in
// is 8 pixels into a single byte. 
#define SCR_FRAME_SIZE (SCR_PIXELS / 32)
#define SCR_LINE_SIZE (SCR_WIDTH / 32)

// We're going to work with DMA transfers of 32bits.
#define SCR_DMA_TRANSFERS (SCR_PIXELS / 32)

// Setup IO pins
#define D0 0
#define D1 (D0 + 1)
#define D2 (D0 + 2)
#define D3 (D0 + 3)
#define FRAME_START (D0 + 4)
#define LATCH_LINE D0 + 5)
#define DOT_CLOCK (D0 + 6)

#define IN_FRAME_PINS (DOT_CLOCK - D0)

typedef struct pio_alloc_type {
  PIO pio;
  uint sm;
  uint offset;
} pio_alloc_t;

typedef struct dma_alloc_type {
  int channel;
  dma_channel_config config;
} dma_alloc_t;


// define our baseline frame buffer.
typedef uint32_t scr_frame_buf_t[SCR_FRAME_SIZE];

typedef struct scr_type {
  scr_frame_buf_t pixels[2];
  uint8_t frame;

  pio_alloc_t pio;

  dma_alloc_t dma;

} scr_t;


void in_frame_pio_init(pio_alloc_t *pio_alloc, uint base_pin);
void in_frame_dma_init(scr_t *scr);
