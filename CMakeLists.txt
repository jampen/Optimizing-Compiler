cmake_minimum_required(VERSION 3.15)

include(FetchContent)

project(CyrexC VERSION 1.0 LANGUAGES CXX)
enable_testing()

# define a macro that helps defining an option
macro(CyrexC_set_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()


if(ENABLE_SANITIZERS)
    string(APPEND CMAKE_CXX_FLAGS " -fno-omit-frame-pointer -fno-sanitize-recover=all -fsanitize=undefined")
endif()


FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

set(BUILD_MOCK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(MagicEnum
    SYSTEM 
    GIT_REPOSITORY https://github.com/Neargye/magic_enum
    GIT_TAG v0.9.7)
FetchContent_MakeAvailable(MagicEnum)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG v3.2
)
FetchContent_MakeAvailable(argparse)

file(GLOB_RECURSE SRC_FILES "${PROJECT_SOURCE_DIR}/src/cyrexc/*.cpp")

#
# We compile CyrexC_lib and link CyrexC 
#
add_library(CyrexC_lib STATIC ${SRC_FILES})
target_compile_features(CyrexC_lib PUBLIC cxx_std_23)
target_include_directories(CyrexC_lib PUBLIC "${PROJECT_SOURCE_DIR}/src/cyrexc")
target_link_libraries(CyrexC_lib PUBLIC argparse magic_enum::magic_enum)

CyrexC_set_option(CLANG_TIDY_EXECUTABLE clang-tidy STRING "Override clang-tidy executable, requires minimum version 14")
add_custom_target(tidy
    COMMAND ${CMAKE_COMMAND} -DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ./cmake/Tidy.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} VERBATIM)

add_executable(CyrexC "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(CyrexC PRIVATE CyrexC_lib )

# --- TESTING ---
file(GLOB_RECURSE TEST_SOURCES "${PROJECT_SOURCE_DIR}/tests/*.cpp")

add_executable(RunTests ${TEST_SOURCES})
target_include_directories(RunTests PUBLIC "${PROJECT_SOURCE_DIR}/src/" "${PROJECT_SOURCE_DIR}/tests/" )
target_link_libraries(RunTests PUBLIC  GTest::gtest_main  GTest::gmock_main CyrexC_lib)

# Auto discover TEST macros
include(GoogleTest)
gtest_discover_tests(RunTests)
