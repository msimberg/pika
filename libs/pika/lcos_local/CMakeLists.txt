# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Default location is $PIKA_ROOT/libs/lcos_local/include
set(lcos_local_headers
    pika/lcos_local/and_gate.hpp
    pika/lcos_local/channel.hpp
    pika/lcos_local/composable_guard.hpp
    pika/lcos_local/conditional_trigger.hpp
    pika/lcos_local/detail/preprocess_future.hpp
    pika/lcos_local/receive_buffer.hpp
    pika/lcos_local/trigger.hpp
)

set(lcos_local_sources composable_guard.cpp preprocess_future.cpp)

include(pika_add_module)
pika_add_module(
  pika lcos_local
  SOURCES ${lcos_local_sources}
  HEADERS ${lcos_local_headers}
  MODULE_DEPENDENCIES
    pika_config_local
    pika_execution
    pika_executors
    pika_futures
    pika_lock_registration
    pika_pack_traversal
    pika_errors
    pika_memory
  CMAKE_SUBDIRS examples tests
)
