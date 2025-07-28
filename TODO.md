* The frequency based sync calcs create a jittery mess. It's better to just put a fixed divider in play and assume 250/10.
* Need to actually start drawing the full frame. That means adding an IRQ to the end of the PIO to reset the DMA read on new frame.
* Lots of code clean up.
