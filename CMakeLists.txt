cmake_minimum_required(VERSION 3.22)
project(chip8_emulation)

set(CMAKE_CXX_STANDARD 14)

add_executable(chip8_emulation main.cpp chip8/chip8.h)