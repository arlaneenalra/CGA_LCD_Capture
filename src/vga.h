#pragma once

#define LDO_GPIO 23

void ldo_pwm_mode();

#define VGA_BASE_PIN 16
#define VGA_HSYNC 0 
#define VGA_VSYNC (VGA_HSYNC + 1)
#define VGA_LO_GREEN (VGA_HSYNC + 2)
#define VGA_HI_GREEN (VGA_HSYNC + 3)
#define VGA_BLUE (VGA_HSYNC + 4)
#define VGA_RED (VGA_HSYNC + 5)

#define VGA_HSYNC_ACTIVE 640

#define VGA_VSYNC_ACTIVE 400
#define VGA_VSYNC_BACKPORCH 20 

#define VGA_FRAME_LINES (VGA_VSYNC_ACTIVE + VGA_VSYNC_BACKPORCH + 1) 

// 4 bits per pixelw
#define VGA_PIXELS (640 * 400 * 4)

#define VGA_FRAME_BUF_SIZE (VGA_PIXELS / 32 )

// number of bytes in a line
#define VGA_DMA_TRANSFERS 80

#define VGA_RGB_ACTIVE 80 

//#define VGA_LINE_ADDR(vga) (uint32_t *)(&((vga)->scr) + (((vga)->line >> 1) * VGA_DMA_TRANSFERS))
#define VGA_LINE_ADDR(vga, offset) (&((vga).scr[((vga).line - offset) * VGA_DMA_TRANSFERS]))

typedef uint32_t vga_frame_buf_t[VGA_FRAME_BUF_SIZE];

typedef struct vga_type {
  vga_frame_buf_t scr;
  uint32_t line;
  uint32_t *line_ptr;
  
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
