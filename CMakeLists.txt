cmake_minimum_required(VERSION 3.14)
project(dfs)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/build)
include_directories(${Protobuf_INCLUDE_DIRS})

add_executable(server
    server/dfs_server.cpp
    build/dfs.pb.cc
    build/dfs.grpc.pb.cc
)

target_link_libraries(server
    gRPC::grpc++
    protobuf::libprotobuf
)


add_executable(client
  client/dfs_client.cpp
  build/dfs.pb.cc
  build/dfs.grpc.pb.cc
)

target_link_libraries(client
  gRPC::grpc++
  protobuf::libprotobuf
)


find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

include_directories(${FUSE3_INCLUDE_DIRS})
link_directories(${FUSE3_LIBRARY_DIRS})

add_executable(fuse_client
  client/fuse_client.cpp
  build/dfs.pb.cc
  build/dfs.grpc.pb.cc
)

target_link_libraries(fuse_client
  gRPC::grpc++
  protobuf::libprotobuf
  ${FUSE3_LIBRARIES}
  pthread
)