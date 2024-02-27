project (chip8_test)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(./../)
add_executable(test_chip8 test_chip8.cpp)

target_link_libraries(test_chip8 ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES} pthread)