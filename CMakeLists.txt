#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt-simple)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c src/app_mqtt.c include/app_mqtt.h)
# NORDIC SDK APP END

zephyr_include_directories(include)
