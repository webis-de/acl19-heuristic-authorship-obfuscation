cmake_minimum_required(VERSION 3.4)
project(obfuscation_search)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_PREFIX_PATH})

add_definitions(-Wall)

if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DNDEBUG)
endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

find_package(Boost COMPONENTS locale serialization filesystem program_options regex thread REQUIRED)
#find_package(Netspeak3 REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
        ${CMAKE_SOURCE_DIR}/obfuscation
        ${CMAKE_SOURCE_DIR}/search-generic/src
        ${CMAKE_SOURCE_DIR}/search-generic/src/lib/thread-pool-cpp/include)
#        ${Netspeak3_INCLUDE_DIR})

add_subdirectory(search-generic)

set(SOURCE_FILES
        main.cpp
        obfuscation/Context.cpp
        obfuscation/State.cpp
        obfuscation/Obfuscator.cpp
        obfuscation/ComputeCostH.cpp
        obfuscation/GoalCheck.hpp
        obfuscation/util/dekker.hpp
        obfuscation/util/LayeredOStream.cpp
        obfuscation/util/DiffString.cpp
        obfuscation/util/NgramProfile.cpp
        obfuscation/operators/ObfuscationOperator.cpp
        obfuscation/operators/AbstractWordOperator.cpp
        obfuscation/operators/NetspeakOperator.cpp
        obfuscation/operators/ContextlessSynonymOperator.cpp
#        obfuscation/util/netspeak.cpp
#        obfuscation/operators/WordRemovalOperator.cpp
#        obfuscation/operators/WordReplacementOperator.cpp
        obfuscation/operators/NgramRemovalOperator.cpp
        obfuscation/operators/ContextlessHypernymOperator.cpp
        obfuscation/operators/CharMapOperator.cpp
        obfuscation/operators/CharacterFlipOperator.cpp)

add_executable(obfuscate ${SOURCE_FILES} ${Netspeak3_PROTO_CPP})
target_link_libraries(obfuscate ${Boost_LIBRARIES} ${Netspeak3_LIBRARIES} ${OPENSSL_LIBRARIES} search_generic)
