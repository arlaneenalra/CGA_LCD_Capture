add_executable(lcd_reader
  main.c
)

pico_generate_pio_header(lcd_reader ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)
pico_enable_stdio_usb(lcd_reader 1)

pico_add_extra_outputs(lcd_reader)

target_link_libraries(lcd_reader
  pico_stdlib
  pico_time
  hardware_pio
  hardware_dma
  hardware_irq
)
