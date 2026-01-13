cmake_minimum_required(VERSION 3.20)
project(FRP_PingCheckProxy)

set(CMAKE_CXX_STANDARD 17)

option(STATIC_BUILD "Build with static linking" OFF)

include(FetchContent)

FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(asio)

set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Asio include dir")
set(ASIO_STANDALONE ON CACHE BOOL "Use standalone asio")

FetchContent_Declare(
        Crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(Crow)
FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG        master
)
set(CPR_CURL_USE_LIBPSL OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(OPENSSL_USE_STATIC_LIBS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(cpr)

add_executable(FRP_PingCheckProxy main.cpp)

target_compile_definitions(FRP_PingCheckProxy PUBLIC ASIO_STANDALONE)

target_include_directories(FRP_PingCheckProxy PRIVATE ${ASIO_INCLUDE_DIR})

if(STATIC_BUILD)
    if(LINUX)
        set_target_properties(FRP_PingCheckProxy PROPERTIES LINK_FLAGS "-static")
    elseif(WIN32)
        set_target_properties(FRP_PingCheckProxy PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
    endif()
else()
    if(LINUX OR WIN32)
        set_target_properties(FRP_PingCheckProxy PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++")
    endif()
endif()

target_link_libraries(FRP_PingCheckProxy PUBLIC Crow::Crow cpr::cpr)