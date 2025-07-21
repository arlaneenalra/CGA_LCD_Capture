#pragma once

#include <inttypes.h>
#include <stdbool.h>

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
 * Here we have:
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
#define VGA_HSYNC 16
#define VGA_HSYNC_OUT (VGA_HSYNC + 1)

#define VGA_VSYNC 14
#define VGA_VSYNC_CLK (VGA_HSYNC_OUT)

#define VGA_RGB_BASE_PIN 18
#define VGA_LO_GREEN (VGA_RGB_BASE_PIN + 0)
#define VGA_HI_GREEN (VGA_RGB_BASE_PIN + 1)
#define VGA_BLUE (VGA_RGB_BASE_PIN + 2)
#define VGA_RED (VGA_RGB_BASE_PIN + 3)

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

typedef struct vga_pwm_type {
  uint8_t hsync_slice;
  uint8_t vsync_slice;

} vga_pwm_t;


void vga_pwm_init(
    vga_pwm_t *vga,
    vga_mode_t *vga_mode,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    bool enable);

uint8_t static inline vga_hsync_pwm(
    vga_sync_t *sync, uint8_t hsync_pin, uint32_t pixel_clk);
uint8_t static inline vga_vsync_pwm(vga_sync_t *sync, uint8_t vsync_pin);
 
uint16_t static inline vga_sync_total(vga_sync_t *sync);
uint32_t static inline vga_get_enable_mask(vga_pwm_t *vga);

extern vga_mode_t vga_mode_list[];

enum vga_mode {
   MODE_640X480_60, 
   MODE_640X400_70, // WARNING: A lot of monitors think this is 720x400@70Hz
   MODE_1280X800_60,   
};

