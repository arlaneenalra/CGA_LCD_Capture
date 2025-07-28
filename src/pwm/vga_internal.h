#pragma once

#include "vga.h"
#include "hardware/pio.h"

typedef struct rgb_pio_type {
  PIO pio;
  uint sm;
  uint offset;

} rgb_pio_t;


typedef struct vga_pwm_type {
  uint8_t hsync_slice;
  uint8_t vsync_slice;

  vga_mode_t *mode;

  rgb_pio_t pio;

  int burst_dma;
  int pixel_dma;
  int reset_dma;
  int ctrl_dma;
  int ctrl_reset_dma;

  uint32_t line;

  volatile uint32_t *frame_buf;

} vga_pwm_t;

/**
 * Defines the values that VGA pio needs to track in the order that
 * pio code expects them. This block is sent to the pio on every
 * line even with certain values ignored to simplify the irq complexity
 * and timing.
 *
 * This must be aligned because it is used as a ring buffer by dma.
 */
#define VGA_LINE_BURST_ALIGNMENT (1 << 4)

typedef struct __attribute__ ((packed, aligned(VGA_LINE_BURST_ALIGNMENT))) vga_pio_line_burst_type {
  uint32_t v_back_porch;
  uint32_t v_visible;
  uint32_t h_back_porch;
  uint32_t h_visible;
} vga_pio_line_burst_t;

#define VGA_LINE_BURST_SIZE 4 

/**
 * Shared internal functions and globals.
 *
 *
 */
extern vga_pwm_t vga;

void vga_dma_init();
void vga_dma_reset();
void vga_dma_enable();

uint8_t vga_hsync_pwm(
    vga_sync_t *sync, uint8_t hsync_pin, uint32_t pixel_clk);
uint8_t vga_vsync_pwm(vga_sync_t *sync, uint8_t vsync_pin);


void vga_pwm_init(
    uint8_t hsync_pin,
    uint8_t vsync_pin);
void vga_pwm_enable();

void vga_pio_init(
    vga_mode_t *mode,
    uint8_t vsync_base_pin,
    uint8_t rgb_base_pin);
void vga_pio_enable(rgb_pio_t *pio);


void vga_irq_init();
void vga_irq_enable();
void vga_irq_handler();

