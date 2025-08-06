#pragma once

#include <inttypes.h>

#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/util/queue.h"

#define SCR_WIDTH 640
#define SCR_HEIGHT 200

// Must be integer divisible by 8 and 32
#define SCR_PIXELS (SCR_WIDTH * SCR_HEIGHT)

// These displays are 1bpp right now, so the raw data coming in
// is 8 pixels into a single byte. 
#define SCR_FRAME_SIZE (SCR_PIXELS / 32)
#define SCR_LINE_SIZE (SCR_WIDTH / 32)
#define SCR_LINE_BYTE_SIZE (SCR_WIDTH / 8)

// We're going to work with DMA transfers of 32bits.
#define SCR_DMA_TRANSFERS (SCR_PIXELS / 32)

#define FRAME_COUNT 1 
#define FRAME_BUFFER_LENGTH (FRAME_COUNT + 1)

// Setup IO pins
#define D0 0
#define D1 (D0 + 1)
#define D2 (D0 + 2)
#define D3 (D0 + 3)
#define FRAME_START (D0 + 4)
#define DOT_CLOCK (D0 + 5)

#define OE_PIN  7

#define IN_FRAME_PINS (DOT_CLOCK - D0 + 1)

typedef struct pio_alloc_type {
  PIO pio;
  uint sm;
  uint offset;
} pio_alloc_t;

typedef struct dma_alloc_type {
  int channel;
  dma_channel_config config;
  irq_num_t dma_irq;
} dma_alloc_t;

typedef struct queue_frame_type {
  uint8_t frame;
  uint16_t dropped;

} queue_frame_t;

// define our baseline frame buffer.
typedef uint32_t scr_frame_buf_t[SCR_FRAME_SIZE];
typedef void (*frame_handler_t)(scr_frame_buf_t pixels);

typedef struct scr_type {
  scr_frame_buf_t pixels[FRAME_BUFFER_LENGTH];
  uint8_t frame;

  pio_alloc_t pio;
  dma_alloc_t dma;

  uint oe_pin;

  queue_t frame_queue;

  frame_handler_t handler;

} scr_t;

void in_frame_init(frame_handler_t handler, uint base_pin, irq_num_t dma_irq, uint oe_pin);
void in_frame_pio_init(pio_alloc_t *pio_alloc, uint base_pin);
void in_frame_dma_init(irq_num_t dma_irq);

void frame_capture_irq();
void frame_capture();

char *get_6bits(uint8_t in[], uint32_t length, uint32_t offset6bit);

extern scr_t scr;
