cmake_minimum_required(VERSION 3.20)
project(flight_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

add_executable(flight_backend
    src/main.cpp
)

if (WIN32)
  target_link_libraries(flight_backend ws2_32)
endif()