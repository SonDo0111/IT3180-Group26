cmake_minimum_required(VERSION 3.20)
project(IT3180_Group26 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(apartment_core
    src/core/types.cpp
    src/core/auth.cpp
    src/modules/household/household.cpp
)

target_include_directories(apartment_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(apartment_management
    src/core/main.cpp
)
target_link_libraries(apartment_management PRIVATE apartment_core)

enable_testing()

add_executable(unit_tests
    tests/unit/test_us01_us03_us04.cpp
)
target_link_libraries(unit_tests PRIVATE apartment_core)

add_test(NAME US01_US03_US04_UnitTests COMMAND unit_tests)
