cmake_minimum_required(VERSION 3.11)

project(run_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

add_executable(run_server 
               src/app/use_cases_impl.cpp
               src/application.cpp
               src/main.cpp 
               src/boost_json.cpp
               src/api/api_handler.cpp
               src/body_types/body_types.cpp
               src/http_server/http_server.cpp
               src/postgres/postgres.cpp
               src/sync/config_loader.cpp 
               src/sync/event_loader.cpp 
               src/sync/thread_loader.cpp
               src/call_simulator/call_generator.cpp
               src/analytics/analytics.cpp
               src/config/dynamic_config.cpp
)

set(THREAD_PREFER_PTHREAD_FLAG_ON)
find_package(Threads REQUIRED)

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
  set(SYSTEM_LIBS -lstdc++)
else()
  set(SYSTEM_LIBS)
endif()

target_include_directories(run_server PRIVATE CONAN_PKG::boost CONAN_PKG:libpqxx)
target_link_libraries(run_server PRIVATE Threads::Threads CONAN_PKG::boost CONAN_PKG::libpqxx)

target_link_libraries(run_server ${SYSTEM_LIBS})
