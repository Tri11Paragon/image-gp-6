cmake_minimum_required(VERSION 3.25)
project(image-gp-6 VERSION 0.0.20)

include(FetchContent)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(lib/blt-gp)
add_subdirectory(lib/blt-graphics)

find_package( OpenCV REQUIRED )

include_directories(include/)
include_directories(lib/stb)
include_directories(lib/ThreatExchange)
include_directories( ${OpenCV_INCLUDE_DIRS} )

file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(image-gp-6 ${PROJECT_BUILD_FILES})

target_compile_options(image-gp-6 PRIVATE -Wall -Wextra -Wpedantic -Wno-comment)
target_link_options(image-gp-6 PRIVATE -Wall -Wextra -Wpedantic -Wno-comment)

target_link_libraries(image-gp-6 PRIVATE BLT BLT_WITH_GRAPHICS blt-gp ${OpenCV_LIBS})

if (${ENABLE_ADDRSAN} MATCHES ON)
    target_compile_options(image-gp-6 PRIVATE -fsanitize=address)
    target_link_options(image-gp-6 PRIVATE -fsanitize=address)
endif ()

if (${ENABLE_UBSAN} MATCHES ON)
    target_compile_options(image-gp-6 PRIVATE -fsanitize=undefined)
    target_link_options(image-gp-6 PRIVATE -fsanitize=undefined)
endif ()

if (${ENABLE_TSAN} MATCHES ON)
    target_compile_options(image-gp-6 PRIVATE -fsanitize=thread)
    target_link_options(image-gp-6 PRIVATE -fsanitize=thread)
endif ()
