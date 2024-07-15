# Install script for directory: /home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/compatibility.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/fs" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/fs/filesystem.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/fs" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/fs/loader.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/fs" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/fs/nbt.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/fs" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/fs/nbt_block.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/averages.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/fixed_point.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/fixed_point_vectors.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/interpolation.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/log_util.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/math.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/matrix.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/math" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/math/vectors.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/parse" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/parse/argparse.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/parse" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/parse/mustache.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/parse" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/parse/obj_loader.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/parse" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/parse/templating.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/profiling" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/profiling/profiler.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/profiling" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/profiling/profiler_v2.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/allocator.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/any.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/array.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/assert.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/binary_tree.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/error.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/expected.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/format.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/hashmap.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/logging.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/memory.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/memory_util.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/meta.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/queue.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/random.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/ranges.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/simd.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/string.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/system.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/thread.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/time.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/types.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/utility.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/uuid.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/std" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/std/vector.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/unicode_emoji.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt/window" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/lib/blt-gp/lib/blt/include/blt/window/window.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/blt" TYPE FILE FILES "/home/brett/Documents/code/c++/image-gp-6/cmake-build-release/config/blt/config.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/brett/Documents/code/c++/image-gp-6/cmake-build-release/lib/blt-gp/lib/blt/libBLT.a")
  endif()
endif()

