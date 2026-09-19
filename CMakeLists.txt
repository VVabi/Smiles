cmake_minimum_required(VERSION 3.20)

project(Smiles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

add_library(smiles_lib
    src/smiles_lib.cpp
    src/preprocessor/preprocessor.cpp
    src/string_utils/string_utils.cpp
)

target_include_directories(smiles_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(smiles
    src/smiles.cpp
)
target_link_libraries(smiles PRIVATE smiles_lib)

add_executable(smiles_playground
    src/smiles_playground.cpp
)
target_link_libraries(smiles_playground PRIVATE smiles_lib)

FetchContent_Declare(
    googletest
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(smiles_tests
    tests/smiles_tests.cpp
    tests/preprocessor_tests.cpp
)
target_link_libraries(smiles_tests PRIVATE smiles_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(smiles_tests)
