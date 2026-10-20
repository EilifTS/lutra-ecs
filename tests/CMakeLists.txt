add_executable(tests
    tests.cpp
    TBitMask.h
    TByteIO.h
    TContainer.h
    TDeferred.h
    TECS.h
    TFileIO.h
    THandle.h
    THandleFreeList.h
    TSaveRoundTrip.h
    TSaveTypes.h
    TSchema.h
    TSparseSet.h
    TSparseTagSet.h
)
target_compile_features(tests PRIVATE cxx_std_23)
if (MSVC)
    target_compile_options(tests PRIVATE /W4)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra)
endif()
target_link_libraries(tests PRIVATE lutra-ecs::lutra-ecs GTest::gtest)

include(GoogleTest)
gtest_discover_tests(tests)