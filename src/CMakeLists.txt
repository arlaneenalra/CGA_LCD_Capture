add_executable(sniffer
  main.c
)

pico_enable_stdio_usb(sniffer 1)

pico_add_extra_outputs(sniffer)

target_link_libraries(sniffer pico_stdlib)
