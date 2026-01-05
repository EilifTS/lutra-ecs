file(GLOB TEST_SOURCES
    tests.cpp
)

add_executable(tests ${TEST_SOURCES})
target_compile_features(tests PRIVATE cxx_std_20)
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tests PRIVATE lutra-ecs GTest::gtest)
