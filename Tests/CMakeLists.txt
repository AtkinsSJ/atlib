find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)

function(atlib_test source)
    get_filename_component(ATLIB_TEST_NAME ${source} NAME_WE)
    add_executable(${ATLIB_TEST_NAME} ${source})
    target_include_directories(${ATLIB_TEST_NAME} PRIVATE "../Source" ${SDL3_INCLUDE_DIRS})
    target_link_libraries(${ATLIB_TEST_NAME} PRIVATE Util ${SDL3_LIBRARIES})

    add_test(NAME ${ATLIB_TEST_NAME}
        COMMAND ${ATLIB_TEST_NAME})
endfunction()

enable_testing()
atlib_test(TestFunction.cpp)
atlib_test(TestHashMap.cpp)
atlib_test(TestHashSet.cpp)
atlib_test(TestLexer.cpp)
atlib_test(TestOwnedPtr.cpp)
atlib_test(TestPriorityQueue.cpp)
atlib_test(TestVariant.cpp)
