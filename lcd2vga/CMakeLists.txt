
add_subdirectory(pwm)
#add_subdirectory(lcd)

add_executable(lcd2vga
  main.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(lcd2vga PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_enable_stdio_usb(lcd2vga 1)
pico_enable_stdio_uart(lcd2vga 0)

pico_add_extra_outputs(lcd2vga)

target_link_libraries(lcd2vga
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
