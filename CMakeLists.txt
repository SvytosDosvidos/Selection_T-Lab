cmake_minimum_required(VERSION 3.16)

project(graph_analytics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

add_executable(graph_analytics
        src/main.cpp
        src/csv_reader.cpp
        src/graph_builder.cpp
        src/pagerank.cpp
)

target_include_directories(graph_analytics PRIVATE include)
target_link_libraries(graph_analytics PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(graph_analytics PRIVATE
            /W4
            /permissive-
            /utf-8
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(graph_analytics PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
    )
endif()

include(CTest)

option(ENABLE_LARGE_TEST "Enable test with a graph larger than 128 MB" OFF)

if(BUILD_TESTING)
    add_test(
            NAME small_graph
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/small.csv"
    )

    set_tests_properties(
            small_graph
            PROPERTIES
            PASS_REGULAR_EXPRESSION "sum=1"
    )

    add_test(
            NAME dangling_vertex
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/dangling.csv"
    )

    set_tests_properties(
            dangling_vertex
            PROPERTIES
            PASS_REGULAR_EXPRESSION "sum=1"
    )

    add_test(
            NAME big_vertex_ids
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/big_nums.csv"
    )

    set_tests_properties(
            big_vertex_ids
            PROPERTIES
            WILL_FAIL TRUE
    )

    add_test(
            NAME negative_vertex_ids
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/negative_ids.csv"
    )

    set_tests_properties(
            negative_vertex_ids
            PROPERTIES
            PASS_REGULAR_EXPRESSION "sum=1"
    )

    add_test(
            NAME empty_graph
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/empty.csv"
    )

    add_test(
            NAME bad_csv
            COMMAND graph_analytics
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/bad.csv"
    )

    set_tests_properties(
            bad_csv
            PROPERTIES
            WILL_FAIL TRUE
    )

    set_tests_properties(
            small_graph
            dangling_vertex
            big_vertex_ids
            negative_vertex_ids
            empty_graph
            bad_csv
            PROPERTIES
            RUN_SERIAL TRUE
    )

    if(ENABLE_LARGE_TEST)
        add_test(
                NAME large_graph
                COMMAND graph_analytics
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/large_graph.csv"
        )

        set_tests_properties(
                large_graph
                PROPERTIES
                PASS_REGULAR_EXPRESSION "edges: 12000000"
                RUN_SERIAL TRUE
                TIMEOUT 300
                LABELS "large"
        )
    endif()
endif()