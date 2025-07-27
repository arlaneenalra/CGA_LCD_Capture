#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "hardware/pio.h"

/* We use PWM for H and V sync, with the V sync clocked off
 * the H sync.
 *
 * To make work, we're going to have to type the B channel
 * of the H sync slice to the B channel of the V sync slice
 * and configure the V sync to clock of it's B channel.
 *
 * Note: Both pins must be on the same PWM slice. For V Sync
 *       the output pin must be Channel A.
 *      
 * For example we have:
 *
 * GPIO 16 -> H Sync out (Channel A)
 * GPIO 17 -> H Sync out (Channel B)
 * GPIO 14 -> V Sync out (Channel A)
 * GPIO 15 -< V Sync clk in (Channel B - Tied to GPIO 17)
 *
 * In theory, we could do this without the extra hardware by 
 * further dividing the pixel clock by total number of clocks in
 * a horizontal line. 
 */

#define VGA_WIDTH 640 
#define VGA_HEIGHT 480

// Determines the size of a frame buffer in 32 bit integers 
#define VGA_FRAME_BUFF_SIZE(width, height) ((width * height * 4) / 32)
#define VGA_FRAME_LINE_SIZE(width) ((width * 4) / 32)

typedef uint32_t vga_frame_buf_t[VGA_FRAME_BUFF_SIZE(VGA_WIDTH, VGA_HEIGHT)];

typedef struct vga_sync_type {
  uint16_t front_porch;
  uint16_t pulse;
  uint16_t back_porch;
  uint16_t visible;
  
  bool negative;

} vga_sync_t;

typedef struct vga_mode_type {
  uint32_t pixel_clk;

  // based of pixel clock
  vga_sync_t h;

  // based of lines
  vga_sync_t v;

} vga_mode_t;

typedef struct rgb_pio_type {
  PIO pio;
  uint sm;
  uint offset;

} rgb_pio_t;

typedef struct vga_pwm_type {
  uint8_t hsync_slice;
  uint8_t vsync_slice;

  rgb_pio_t pio;

} vga_pwm_t;

void vga_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin);

void vga_pwm_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint8_t hsync_pin,
    uint8_t vsync_pin);

void vga_pio_init(
    rgb_pio_t *pio, uint8_t vsync_base_pin, uint8_t rgb_base_pin);

void vga_dma_init(rgb_pio_t *rgb_pio, vga_mode_t *mode, uint32_t *frame_buf);
void vga_dma_irq();

uint8_t vga_hsync_pwm(
    vga_sync_t *sync, uint8_t hsync_pin, uint32_t pixel_clk);
uint8_t vga_vsync_pwm(vga_sync_t *sync, uint8_t vsync_pin);

void vga_pwm_enable(vga_pwm_t *vga);
void vga_pio_enable(rgb_pio_t *pio);

void vga_enable(vga_pwm_t *vga);

extern vga_mode_t vga_mode_list[];

enum vga_mode {
   MODE_640X480_60, 
   MODE_640X400_70, // WARNING: A lot of monitors think this is 720x400@70Hz
   MODE_1280X800_60 // This mode is totally unrealistic  
};


