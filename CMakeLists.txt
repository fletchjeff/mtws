cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(computercard C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

macro(add_knots_target _name _dir _src)
  add_executable(${_name})
  if(TARGET ${_name})
    target_compile_options(${_name} PRIVATE -Wdouble-promotion -Wfloat-conversion -Wall -Wextra)
    target_link_options(${_name} PRIVATE -Wl,--print-memory-usage)

    # Give oscillator more time to start - some boards won't run if this isn't included
    target_compile_definitions(${_name} PRIVATE PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

    target_include_directories(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${_dir})
    target_link_libraries(${_name} pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi hardware_interp)
    if(${ARGC} GREATER 3)
      set_target_properties(${_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ARGV3})
    endif()
    pico_add_extra_outputs(${_name})
    target_sources(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${_dir}/${_src})
    pico_enable_stdio_usb(${_name} 0)
  endif()
endmacro()

add_knots_target(sawsome knots/solo_engines/sawsome main.cpp solo/sawsome)
target_sources(sawsome PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/sawsome/sawsome_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(sawsome pico_multicore)

add_knots_target(bender knots/solo_engines/bender main.cpp solo/bender)
target_sources(bender PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/bender/bender_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(bender pico_multicore)

add_knots_target(floatable knots/solo_engines/floatable main.cpp solo/floatable)
target_sources(floatable PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/floatable/floatable_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(floatable pico_multicore)

add_knots_target(cumulus knots/solo_engines/cumulus main.cpp solo/cumulus)
target_sources(cumulus PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/cumulus/cumulus_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(cumulus pico_multicore)

add_knots_target(losenge knots/solo_engines/losenge main.cpp solo/losenge)
target_sources(losenge PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/losenge/losenge_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(losenge pico_multicore)

add_knots_target(din_sum knots/solo_engines/din_sum main.cpp solo/din_sum)
target_sources(din_sum PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/din_sum/din_sum_solo_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/solo_engines/solo_common/solo_control_router.cpp
)
target_link_libraries(din_sum pico_multicore)

add_knots_target(knots knots/src main.cpp)
target_sources(knots PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/core/control_router.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/core/midi_worker.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/bender_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/cumulus_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/din_sum_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/engine_registry.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/floatable_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/losenge_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/engines/sawsome_engine.cpp
  ${CMAKE_CURRENT_LIST_DIR}/knots/src/usb/usb_descriptors.c
)
target_include_directories(knots PUBLIC ${CMAKE_CURRENT_LIST_DIR}/knots/src/usb)
target_link_libraries(knots pico_multicore tinyusb_device tinyusb_board hardware_flash hardware_sync)

# USB serial input monitor tool for inspecting raw Audio/CV/Pulse input values.
add_executable(input_monitor)
if(TARGET input_monitor)
  target_compile_options(input_monitor PRIVATE -Wdouble-promotion -Wfloat-conversion -Wall -Wextra)
  target_link_options(input_monitor PRIVATE -Wl,--print-memory-usage)

  # Give oscillator more time to start - some boards won't run if this isn't included.
  target_compile_definitions(input_monitor PRIVATE PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

  target_include_directories(input_monitor PUBLIC ${CMAKE_CURRENT_LIST_DIR})
  target_link_libraries(
    input_monitor
    pico_unique_id
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_i2c
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_interp
  )
  set_target_properties(input_monitor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tools/input_monitor)
  target_sources(input_monitor PUBLIC ${CMAKE_CURRENT_LIST_DIR}/tools/input_monitor/src/main.cpp)
  pico_enable_stdio_usb(input_monitor 1)
  pico_enable_stdio_uart(input_monitor 0)
  pico_add_extra_outputs(input_monitor)
endif()
