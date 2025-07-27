#include "vga_internal.h"

vga_mode_t vga_mode_list[] = {

  // 640x480@60Hz http://www.tinyvga.com/vga-timing/640x480@60Hz
  // vertical refresh (h sync) - 31.46875 kHz
  // screen refresh (v sync) - 60 Hz

  [MODE_640X480_60] = {
    .pixel_clk = 25175000,
   
    .h = {
      .front_porch = 16,
      .pulse = 96,
      .back_porch = 48,
      .visible = 640,

      .negative = true
    },

    .v = {
      .front_porch = 10,
      .pulse = 2,
      .back_porch = 33,
      .visible = 480,

      .negative = true
    }
  },

  // 640x400@70Hz http://www.tinyvga.com/vga-timing/640x400@70Hz 
  // vertical refresh (h sync) - 31.46875 kHz
  // screen refresh (v sync) - 60 Hz
  [MODE_640X400_70] = {
    .pixel_clk = 25175000,
   
    .h = {
      .front_porch = 16,
      .pulse = 96,
      .back_porch = 48,
      .visible = 640,

      .negative = true
    },

    .v = {
      .front_porch = 12,
      .pulse = 2,
      .back_porch = 35,
      .visible = 400,
 
      .negative = false 
    }
  },

  // 1280x800@60Hz http://www.tinyvga.com/vga-timing/1280x800@60Hz 
  // vertical refresh (h sync) - 49.678571428571 kHz
  // screen refresh (v sync) - 60 Hz
  [MODE_1280X800_60] = {
    .pixel_clk = 83460000,
   
    .h = {
      .front_porch = 64,
      .pulse = 136,
      .back_porch = 200,
      .visible = 1280,

      .negative = true
    },

    .v = {
      .front_porch = 1,
      .pulse = 3,
      .back_porch = 24,
      .visible = 800,
 
      .negative = false 
    }
  }

};


