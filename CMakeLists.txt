cmake_minimum_required(VERSION 3.21)
project(Emerge CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 4.5.0   
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # build static raylib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)


file(GLOB EMERGE_SOURCES
  src/*.cpp
)
add_executable(Emerge ${EMERGE_SOURCES})

target_link_libraries(Emerge PRIVATE raylib)


if(MSVC)
  target_compile_options(Emerge PRIVATE /W3)
  # Statically link the MSVC runtime so no VC++ redist is required
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()


set_property(TARGET Emerge PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
