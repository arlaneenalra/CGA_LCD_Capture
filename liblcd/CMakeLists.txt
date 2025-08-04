add_library(lcd_capture
  lcd.c
)

target_include_directories(lcd_capture PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)


pico_generate_pio_header(lcd_capture  ${CMAKE_CURRENT_LIST_DIR}/lcd.pio)

target_link_libraries(lcd_capture
  pico_stdlib

  hardware_pio
  hardware_dma
  hardware_irq
)

