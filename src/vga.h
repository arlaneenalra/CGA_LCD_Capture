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

typedef struct vga_type {

  pio_alloc_t hsync;
  pio_alloc_t vsync;

} vga_t;

void vga_pio_init(vga_t *vga, uint base_pin);

#include "hsync.pio.h"
#include "vsync.pio.h"
