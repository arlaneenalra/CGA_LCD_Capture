# CGA Class LCD Capture Device

A interface board that converts some esoteric laptop LCD video signals to VGA or DVI.

## Status

2025-08-05 - Initial working firmware and prototype hardware!
* Capture works with 640x480x1bpp effective output in VGA and DVI
* Uses line doubling scaling with a 40 pixel top and bottom offset.
* Works on the Pico and Pico 2 in both VGA and DVI mode depending on build and hardware.

## The Problem
If you've ever worked on vintage XT/AT class laptops, in particular the Tandy 1100FD and 1110HD, then you're quite aware that there are any number of machines (or have been) available with broken LCD panels.
Unfortunately, these LCD panels are almost impossible to come by the in the modern age.
This particular machine uses a CGA class (i.e. 640x200 pixel) FSTN display that operates based on a set of shift registers and latches rather than the more traditional video signals.
On top of that, it clocks in 4 pixels in parallel every "dot" clock rather than a single pixel as one might expect.
This makes it somewhat difficult to work with.

Thankfully, the signals are actually pretty easy to understand with an Oscilloscope and the [service manual](https://archive.org/details/tandy-1100-fd-service-manual/Tandy1100FD-Service-Manual-Addendum/) is available on Archive.org.
[Huge thanks to EEVblog!](https://www.eevblog.com/2024/11/13/scanning-vintage-computer-service-manuals-tandy-1100fd-laptop-computer/)

In the service manual:
* Page 4-54 (about 76) describes the pin out and rough theory of operation.
* Page 4-61 (about 83) has a further breakdown of the operation.
* Page A-2 through A-6 (about 136 through 139) has a breakdown of the operation including timing diagrams.

Those timing diagrams were vital to getting this working as that's how I figured out that that 4 data lines coming out of the main board are 4 pixels in parallel and not 1 pixel with 16 possible values.

## Current Hardware

![Prototype Board](images/prototype-dvi.jpg?raw=true "Prototype Board")

The LCD capture code needs an 74LVC245.
Mainly, this is a level shifter that has 5v tolerant inputs and can operate on 3.3V from the Pico board.

The capture code use 6 consecutive inputs fed through the level shifter.

By default, this starts with GPIO 0:
* GPIO 0 - Pixel 1
* GPIO 1 - Pixel 2
* GPIO 2 - Pixel 3
* GPIO 3 - Pixel 4
* GPIO 4 - Frame Start Signal
* GPIO 5 - "Dot" Clock

We don't need the line clock for this configuration because we're always clocking a full frame.

GPIO 7 is currently used to pull the output enable of the 74LVC245 low.
This is likely unnecessary, but has not been pulled from the firmware just yet. 

### DVI version

The default build uses a [DVI Sock](https://github.com/Wren6991/Pico-DVI-Sock) on a Raspberry Pi Pico or Pico 2 board.
I used the one from [Ada Fruit](https://www.adafruit.com/product/5957) on a stock board.

There's a plan to use get an Ada Fruit DVI Feather board working, but I was having issues with the PIO code.
Currently it requires 6 consecutive GPIO pins.

### VGA version

The code for the VGA version works, but is a bit hackish and I strongly recommend using the DVI based version instead if possible.
I'm leaving it here for posterity more than anything else right now. It uses 2 PWM slices and a pretty large PIO state machine.

By default the pinout is:

* GPIO 16 H Sync out
* GPIO 17 tied to GPIO 15
* GPIO 14 V Sync out
* GPIO 18 - Green 0
* GPIO 19 - Green 1
* GPIO 20 - Blue 0
* GPIO 21 - Red 0

GPIO 18 through 21 are wired according to https://vanhunteradams.com/Pico/VGA/VGA.html with an extra 470 ohm resistor on pin 18 with the resistors on 18 and 19 tied together.
I'll work up schematics on this at some point down the road.

## To Build the Firmware
You will need to have the pre-requisites for the pico-sdk already installed.
On mac you will likely need [osx-cross](https://github.com/osx-cross/homebrew-arm) or similar for an arm tool chain.
I have not used the VS Code extension at all.

* Clone the repo: `git clone --recursive https://github.com/arlaneenalra/CGA_LCD_Capture.git`
* Run `setup.sh` - this will build all versions of the code in the build sub directory.
* Pick the build you want and the appropriate uf2 file from lcd2vga or lcd2dvi in that build directory.

## References:
* https://vanhunteradams.com/Pico/VGA/VGA.html
* https://github.com/vha3/Hunter-Adams-RP2040-Demos/blob/master/VGA_Graphics/VGA_Graphics_Primitives/vga16_graphics.h
* https://github.com/Wren6991/PicoDVI
* https://github.com/Wren6991/Pico-DVI-Sock


