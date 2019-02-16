project(tests)

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${GMOCK_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR})

add_executable(runTests
        test_points_conti.cpp)

target_link_libraries(runTests ${GTEST_BOTH_LIBRARIES})
target_link_libraries(runTests SpatialWeights)
