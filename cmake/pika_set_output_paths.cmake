# Copyright (c) 2012 Thomas Heller
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(PIKA_SET_OUTPUT_PATH ON)

if(MSVC)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PIKA_WITH_BINARY_DIR}/Release/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PIKA_WITH_BINARY_DIR}/Release/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PIKA_WITH_BINARY_DIR}/Release/lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PIKA_WITH_BINARY_DIR}/Debug/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PIKA_WITH_BINARY_DIR}/Debug/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PIKA_WITH_BINARY_DIR}/Debug/lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PIKA_WITH_BINARY_DIR}/MinSizeRel/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${PIKA_WITH_BINARY_DIR}/MinSizeRel/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${PIKA_WITH_BINARY_DIR}/MinSizeRel/lib")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PIKA_WITH_BINARY_DIR}/RelWithDebInfo/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PIKA_WITH_BINARY_DIR}/RelWithDebInfo/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PIKA_WITH_BINARY_DIR}/RelWithDebInfo/lib")
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PIKA_WITH_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PIKA_WITH_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PIKA_WITH_BINARY_DIR}/lib")
endif()
