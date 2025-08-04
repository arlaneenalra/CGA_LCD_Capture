
add_executable(lcd2dvi
  main.c
  dvi.c
)

target_include_directories(lcd2dvi PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${PICO_DVI_PATH}/include
)

pico_enable_stdio_usb(lcd2dvi 1)
pico_enable_stdio_uart(lcd2dvi 0)

pico_add_extra_outputs(lcd2dvi)

target_compile_definitions(lcd2dvi PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG}
	DVI_VERTICAL_REPEAT=1
	DVI_N_TMDS_BUFFERS=3
	DVI_MONOCHROME_TMDS
)


target_link_libraries(lcd2dvi
  pico_stdlib
  pico_time
  pico_multicore

  hardware_pio
  hardware_dma

 	pico_stdlib
	libdvi

  lcd_capture
)
