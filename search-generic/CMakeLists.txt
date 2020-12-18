cmake_minimum_required(VERSION 3.8)
project(search_generic CXX)

set(CMAKE_CXX_STANDARD 14)

set(SEARCH_GENERIC_HEADER_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/AstarSearch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/ClosedList.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/debug.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/Node.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/OpenList.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/Operator.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/search/generic/Status.hpp
        )

set(THREAD_POOL_HEADER_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/thread_pool/fixed_function.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/thread_pool/mpmc_bounded_queue.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/thread_pool/thread_pool.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/thread_pool/thread_pool_options.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/thread_pool/worker.hpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include)

add_library(search_generic INTERFACE)
target_sources(search_generic INTERFACE ${SEARCH_GENERIC_HEADER_FILES} ${THREAD_POOL_HEADER_FILES})

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/search/" DESTINATION "include")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib/thread-pool-cpp/include/" DESTINATION "include")
