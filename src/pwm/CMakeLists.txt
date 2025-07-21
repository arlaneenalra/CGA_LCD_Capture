add_library(vga_pwm
  vga_pwm.c
)

target_include_directories(vga_pwm PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(vga_pwm
  pico_stdlib
  pico_time

  hardware_pwm
)
