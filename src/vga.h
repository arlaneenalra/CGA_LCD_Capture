#pragma once

#define VGA_BASE_PIN 16
#define VGA_HSYNC 0 
#define VGA_VSYNC (VGA_HSYNC + 1)
#define VGA_LO_GREEN (VGA_HSYNC + 2)
#define VGA_HI_GREEN (VGA_HSYNC + 3)
#define VGA_BLUE (VGA_HSYNC + 4)
#define VGA_RED (VGA_HSYNC + 5)

#define VGA_HSYNC_ACTIVE 655
#define VGA_VSYNC_ACTIVE 399
#define VGA_RGB_ACTIVE 640

#define VGA_PIXELS (640 * 200)
// 4 bits per pixel
#define VGA_FRAME_BUF_SIZE (VGA_PIXELS / 4 )
#define VGA_DMA_TRANSFERS VGA_FRAME_BUF_SIZE 

typedef uint32_t vga_frame_buf_t[VGA_FRAME_BUF_SIZE];

typedef struct vga_type {
  vga_frame_buf_t scr;
  
  pio_alloc_t hsync;
  pio_alloc_t vsync;
  pio_alloc_t rgb;

  dma_alloc_t dma;
} vga_t;

void vga_pio_init(vga_t *vga, uint base_pin);
void vga_dma_init(vga_t *vga);
void vga_dma_irq();

#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"
