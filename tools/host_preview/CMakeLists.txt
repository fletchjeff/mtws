cmake_minimum_required(VERSION 3.16)

if(APPLE AND NOT DEFINED CMAKE_OSX_SYSROOT)
  execute_process(
    COMMAND xcrun --show-sdk-path
    OUTPUT_VARIABLE MACOS_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(MACOS_SDK_PATH)
    set(CMAKE_OSX_SYSROOT "${MACOS_SDK_PATH}" CACHE PATH "macOS SDK path")
  endif()
endif()

project(mtws_host_preview CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MTWS_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(mtws_host_preview
  src/main.cpp
  ${MTWS_ROOT}/knots/solo_engines/solo_common/solo_control_router.cpp
  ${MTWS_ROOT}/knots/solo_engines/sawsome/sawsome_solo_engine.cpp
  ${MTWS_ROOT}/knots/solo_engines/bender/bender_solo_engine.cpp
  ${MTWS_ROOT}/knots/solo_engines/floatable/floatable_solo_engine.cpp
  ${MTWS_ROOT}/knots/solo_engines/cumulus/cumulus_solo_engine.cpp
  ${MTWS_ROOT}/knots/solo_engines/losenge/losenge_solo_engine.cpp
  ${MTWS_ROOT}/knots/solo_engines/din_sum/din_sum_solo_engine.cpp
)

target_include_directories(mtws_host_preview PRIVATE
  ${MTWS_ROOT}
)

target_compile_options(mtws_host_preview PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

if(APPLE)
  target_compile_options(mtws_host_preview PRIVATE
    -isystem ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
  )

  find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
  target_link_libraries(mtws_host_preview PRIVATE ${AUDIOTOOLBOX_FRAMEWORK})
endif()
