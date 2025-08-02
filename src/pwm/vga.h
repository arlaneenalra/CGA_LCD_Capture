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

typedef volatile uint32_t vga_frame_buf_t[VGA_FRAME_BUFF_SIZE(VGA_WIDTH, VGA_HEIGHT)];

void vga_init(
    volatile uint32_t *frame_buf,
    uint8_t hsync_pin,
    uint8_t vsync_pin,
    uint8_t rgb_pin);

void vga_enable();

