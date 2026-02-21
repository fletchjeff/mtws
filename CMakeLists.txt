cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(computercard C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

macro(add_prex_target _name _dir _src)
  add_executable(${_name})
  if(TARGET ${_name})
    target_compile_options(${_name} PRIVATE -Wdouble-promotion -Wfloat-conversion -Wall -Wextra)
    target_link_options(${_name} PRIVATE -Wl,--print-memory-usage)

    # Give oscillator more time to start - some boards won't run if this isn't included
    target_compile_definitions(${_name} PRIVATE PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

    target_include_directories(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${_dir})
    target_link_libraries(${_name} pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi)
    pico_add_extra_outputs(${_name})
    target_sources(${_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/${_dir}/${_src})
    pico_enable_stdio_usb(${_name} 0)
  endif()
endmacro()

add_prex_target(additive prex/additive main.cpp)
add_prex_target(centroid prex/centroid main.cpp)
add_prex_target(wavetable prex/wavetable main.cpp)
add_prex_target(mvp prex/mvp main.cpp)
