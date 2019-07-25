find_path(Netspeak3_INCLUDE_DIR netspeak/NetspeakRS3.hpp
        PATHS $ENV{HOME}/code-in-progress/netspeak/netspeak3-cpp/src /usr/local/include)

find_path(AitoolsBigHashMap_INCLUDE_DIR aitools/bighashmap/BigHashMap.hpp
        PATHS $ENV{HOME}/code-in-progress/aitools/aitools3-aq-bighashmap-cpp/src /usr/local/include)

find_path(AitoolsInvertedIndex3_INCLUDE_DIR aitools/invertedindex/Indexer.hpp
        PATHS $ENV{HOME}/code-in-progress/aitools/aitools3-aq-invertedindex3-cpp/src /usr/local/include)

find_library(Netspeak3_LIBRARY netspeak3
        PATHS $ENV{HOME}/code-in-progress/netspeak/netspeak3-cpp/out/lib /usr/local/lib)

find_package(Protobuf REQUIRED)
find_path(Netspeak3_PROTO_PATH NetspeakMessages.proto
        PATHS $ENV{HOME}/code-in-progress/netspeak/netspeak3-cpp/conf /usr/local/share/netspeak3 /usr/share/netspeak3)

find_library(CMP_LIBRARY cmph)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/netspeak3/netspeak)
execute_process(COMMAND protoc --cpp_out=${CMAKE_BINARY_DIR}/netspeak3/netspeak
        --proto_path=${Netspeak3_PROTO_PATH} ${Netspeak3_PROTO_PATH}/NetspeakMessages.proto)
set(Netspeak3_PROTO_CPP ${CMAKE_BINARY_DIR}/netspeak3/netspeak/NetspeakMessages.pb.cc)
set(Netspeak3_PROTO_HEADERS ${CMAKE_BINARY_DIR}/netspeak3/netspeak/NetspeakMessages.pb.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Netspeak3 DEFAULT_MSG Netspeak3_LIBRARY Netspeak3_INCLUDE_DIR)
set(Netspeak3_LIBRARIES ${Netspeak3_LIBRARY} ${Protobuf_LIBRARIES} ${CMP_LIBRARY})

include_directories(${Netspeak3_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/netspeak3
        ${AitoolsBigHashMap_INCLUDE_DIR}
        ${AitoolsInvertedIndex3_INCLUDE_DIR})

mark_as_advanced(Netspeak3_LIBRARY
        Netspeak3_DIR
        Netspeak3_PROTO_PATH
        Netspeak3_INCLUDE_DIR
        AitoolsBigHashMap_INCLUDE_DIR
        AitoolsInvertedIndex3_INCLUDE_DIR)
