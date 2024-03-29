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
    GRPC
    LIBRARIES ${bunsan_common_LIBRARIES}
    INCLUDE_DIRECTORIES ${bunsan_common_INCLUDE_DIRS}
    INSTALL
)

bunsan_add_library(${PROJECT_NAME}
    src/lib/engine.cpp
    src/lib/poller.cpp
    src/web/monitor.cpp
)
bunsan_use_target(${PROJECT_NAME} ${PROJECT_NAME}_pb)

bunsan_add_executable(${PROJECT_NAME}_server
    src/bin/server.cpp
)
bunsan_use_target(${PROJECT_NAME}_server ${PROJECT_NAME})
bunsan_use_bunsan_package(${PROJECT_NAME}_server bunsan_web bunsan_web)
bunsan_use_bunsan_package(${PROJECT_NAME}_server bunsan_protobuf bunsan_protobuf)
target_link_libraries(${PROJECT_NAME}_server PUBLIC cppdb)

if(BunsanWebBuiltin)
    BunsanWebLoad()
else()
    find_package(BunsanWeb CONFIG REQUIRED)
endif()

bunsan_web_add_skin(${PROJECT_NAME}_skin_plain plain
    view/plain/base.tmpl
    view/plain/base_form.tmpl
    view/plain/error.tmpl
    view/plain/edit.tmpl
    view/plain/show.tmpl
)
bunsan_use_target(${PROJECT_NAME}_skin_plain ${PROJECT_NAME})

bunsan_add_executable(${PROJECT_NAME}_agent
    src/lib/agent.cpp
    src/bin/agent.cpp
)
bunsan_use_target(${PROJECT_NAME}_agent ${PROJECT_NAME}_pb)
bunsan_use_bunsan_package(${PROJECT_NAME}_agent bunsan_common bunsan_common)

bunsan_install_headers()
bunsan_install_targets(${PROJECT_NAME} ${PROJECT_NAME}_server ${PROJECT_NAME}_agent)
bunsan_web_install_skins(${PROJECT_NAME}_skin_plain)
bunsan_web_install_static()
bunsan_install_project()

bunsan_include_tests()
