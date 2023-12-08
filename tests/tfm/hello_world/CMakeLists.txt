# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE src/main.c)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DPLATFORM_DEFAULT_ATTEST_HAL=OFF
  -DPLATFORM_DEFAULT_ROTPK=OFF
  -DPLATFORM_DEFAULT_IAK=OFF
  -DPLATFORM_DEFAULT_NV_SEED=OFF
  -DPLATFORM_DEFAULT_OTP=OFF
)
