add_executable(tests_sharedPtr
    test_shared_ptr.cpp
)

target_link_libraries(tests_sharedPtr
    shared_ptr_lib
    GTest::gtest_main
)

target_compile_options(tests_sharedPtr PRIVATE
    -O0
    -g
)

include(GoogleTest)
gtest_discover_tests(tests_sharedPtr)

