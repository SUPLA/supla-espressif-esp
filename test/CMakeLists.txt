cmake_minimum_required(VERSION 3.11)

enable_testing()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_BUILD_TYPE Debug)


include_directories(doubles/)
include_directories(../)
include_directories(../supla-common/)
include_directories(../src/include/)
include_directories(../src/nettle/include)
include_directories(../src/user/)

mark_as_advanced(
BUILD_GMOCK
BUILD_GTEST
)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        origin/master
  )

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

file(GLOB TEST_SRC
  src/*.cpp
  src/*.c
  )


file(GLOB COMMON_DOUBLE_SRC 
  doubles/*.cpp
  doubles/*.c
  )

project(supla_common_test)
file(GLOB SRC
  ../src/user/supla_esp_gpio.c
  ../src/user/supla_esp_cfg.c
  ../src/user/supla_esp_cfgmode.c
  ../src/user/supla_esp_devconn.c
  ../src/user/supla_esp_cfgmode_html.c
  ../src/user/supla_esp_state.c
  ../src/user/supla_update.c
  ../src/user/supla_esp_countdown_timer.c
  ../src/user/supla_esp_dns_client.c
  ../src/user/supla_esp_wifi.c
  )

add_executable(supla_common_test ${SRC} ${TEST_SRC} ${COMMON_DOUBLE_SRC})

target_compile_definitions(supla_common_test PUBLIC _ROLLERSHUTTER_SUPPORT)
target_compile_definitions(supla_common_test PUBLIC __FOTA)
target_compile_definitions(supla_common_test PUBLIC CFG_SECTOR=0x3C)
target_compile_definitions(supla_common_test PUBLIC ESPMISSINGINCLUDES_H)
# ignore GPIO_PORT_INIT define:
target_compile_definitions(supla_common_test PUBLIC GPIO_PORT_INIT)

target_link_libraries(supla_common_test
  gmock
  gtest
  gtest_main
  )

add_test(NAME supla_common
  COMMAND supla_common)
