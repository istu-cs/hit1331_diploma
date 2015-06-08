cmake_minimum_required(VERSION 3.0)

include(${CMAKE_SOURCE_DIR}/system-config.cmake OPTIONAL)

project(mon)

add_subdirectory(third_party/bunsan_cmake)
add_subdirectory(third_party/bunsan_testing)
add_subdirectory(third_party/bunsan_common)
add_subdirectory(third_party/bunsan_protobuf)
add_subdirectory(third_party/bunsan_web)

if(BunsanCMakeBuiltin)
    BunsanCMakeLoad()
else()
    find_package(BunsanCMake CONFIG REQUIRED)
endif()

bunsan_add_protobuf_cxx_library(
    TARGET ${PROJECT_NAME}_pb
    PROTOS
        mon/server.proto
    LIBRARIES ${bunsan_common_LIBRARIES}
    PLUGINS grpc=/usr/bin/grpc_cpp_plugin # FIXME
    INCLUDE_DIRECTORIES ${bunsan_common_INCLUDE_DIRS}
    INSTALL
)

bunsan_add_library(${PROJECT_NAME}
    src/lib/stub.cpp
)
bunsan_use_target(${PROJECT_NAME} ${PROJECT_NAME}_pb)
bunsan_use_bunsan_package(${PROJECT_NAME} bunsan_protobuf bunsan_protobuf)

bunsan_add_executable(${PROJECT_NAME}_server
    src/bin/server.cpp
)
bunsan_use_target(${PROJECT_NAME}_server ${PROJECT_NAME})

bunsan_add_executable(${PROJECT_NAME}_agent
    src/bin/agent.cpp
)
bunsan_use_target(${PROJECT_NAME}_agent ${PROJECT_NAME})

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${PROJECT_NAME}_agent)
bunsan_install_project()

bunsan_include_tests()
