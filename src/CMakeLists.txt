add_executable(lcd_reader
  main.c
  usb_descriptors.c
  libb64/cencode.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(lcd_reader PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/libb64)

pico_generate_pio_header(lcd_reader ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

pico_generate_pio_header(lcd_reader  ${CMAKE_CURRENT_LIST_DIR}/vga/hsync.pio)
pico_generate_pio_header(lcd_reader  ${CMAKE_CURRENT_LIST_DIR}/vga/vsync.pio)

pico_enable_stdio_usb(lcd_reader 1)

pico_add_extra_outputs(lcd_reader)

target_link_libraries(lcd_reader
  pico_stdlib
  pico_time
  pico_multicore
  hardware_pio
  hardware_dma
  hardware_irq
  tinyusb_device
  tinyusb_board
)
