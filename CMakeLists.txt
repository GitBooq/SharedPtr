cmake_minimum_required(VERSION 3.14)

project(SharedPtr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "Suppress developer warnings")

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
option(ENABLE_SANITIZERS "Enable sanitizers" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    # exclude self assignment as error
    #add_compile_options(-Wno-error=self-assign-overloaded)
endif()

include_directories(${INCLUDE_DIR})
add_library(shared_ptr_lib INTERFACE)

target_include_directories(shared_ptr_lib INTERFACE
    ${INCLUDE_DIR}
)

if (ENABLE_SANITIZERS)
    target_compile_options(shared_ptr_lib INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
    )
    target_link_options(shared_ptr_lib INTERFACE
        -fsanitize=address,undefined
    )
endif()


# GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)