add_library(vga_pwm
  pwm.c
  pio.c
  modes.c
  vga.c
  dma.c
  irq.c
)

target_include_directories(vga_pwm PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)


pico_generate_pio_header(vga_pwm  ${CMAKE_CURRENT_LIST_DIR}/rgb.pio)

target_link_libraries(vga_pwm
  pico_stdlib

  hardware_pio
  hardware_pwm
  hardware_dma
  hardware_irq
)

