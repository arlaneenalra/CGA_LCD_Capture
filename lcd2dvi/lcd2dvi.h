#pragma once

#include <inttypes.h>

#include "hardware/irq.h"

// Borrowed form 
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

#define DVI_LINE_SIZE (FRAME_WIDTH / 32)

#define DVI_IRQ DMA_IRQ_1
#define LCD_IRQ DMA_IRQ_0

// We need 6 consecutive pins
// This is for the feather board
#define LCD_BASE_PIN 0
#define LCD_OE_PIN 7

typedef uint32_t dvi_frame_buf_t[(FRAME_WIDTH / 32) * FRAME_HEIGHT];

extern volatile dvi_frame_buf_t frame_buf;

void local_dvi_init();
void local_dvi_core();

