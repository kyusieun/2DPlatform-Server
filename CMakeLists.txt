cmake_minimum_required(VERSION 3.28)
project(2DPlatform-Server LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# 헤더 파일 디렉토리 추가
include_directories(${PROJECT_SOURCE_DIR}/src)

# 소스 파일 목록
set(SOURCES
    src/main.cpp
    src/handle_client.cpp
    src/server_types.cpp
)

# 실행 파일 생성
add_executable(server ${SOURCES})
target_compile_features(server PRIVATE cxx_std_17)
target_link_libraries(server PRIVATE SFML::Network SFML::System)
