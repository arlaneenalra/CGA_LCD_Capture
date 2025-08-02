
add_subdirectory(pwm)
add_subdirectory(lcd)

add_executable(lcd_reader
  main.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(lcd_reader PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/libb64)

pico_enable_stdio_usb(lcd_reader 1)
pico_enable_stdio_uart(lcd_reader 0)

pico_add_extra_outputs(lcd_reader)

target_link_libraries(lcd_reader
  pico_stdlib
  pico_time
  pico_multicore

  hardware_pio
  hardware_dma
  hardware_irq
  
  hardware_pwm

  vga_pwm
  lcd_capture
)
