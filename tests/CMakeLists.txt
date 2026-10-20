add_executable(tests
    tests.cpp
    TBitMask.h
    TDeferred.h
    TECS.h
    THandle.h
    THandleFreeList.h
    TSparseSet.h
    TSparseTagSet.h
)
target_compile_features(tests PRIVATE cxx_std_20)
if (MSVC)
    target_compile_options(tests PRIVATE /W4)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra)
endif()
target_link_libraries(tests PRIVATE lutra-ecs::lutra-ecs GTest::gtest)

include(GoogleTest)
gtest_discover_tests(tests)