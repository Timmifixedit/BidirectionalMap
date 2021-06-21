# adapted from https://github.com/SoPra-Team-10/GameLogic/tree/master/Tests
project(Tests)

enable_testing()
find_package(GTest)
if (GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIR})

    file(GLOB_RECURSE TEST_SOURCES . *.cpp)
    include_directories(${CMAKE_SOURCE_DIR})
    add_executable(${PROJECT_NAME} main.cpp ${SOURCES} ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME} ${LIBS} gmock gtest pthread)

    add_test(
            NAME ${PROJECT_NAME}
            COMMAND ${PROJECT_NAME}
    )
else()
    message(WARNING "GTest not found, you won't be able to run the tests")
endif()