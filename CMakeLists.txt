cmake_minimum_required(VERSION 3.21)
project(Emerge LANGUAGES CXX)

# Require C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options for raylib FetchContent build
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# Fetch raylib release 
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        4.5.0
)
FetchContent_MakeAvailable(raylib)

# Collect sources 
file(GLOB_RECURSE EMERGE_SRC
  CONFIGURE_DEPENDS
  "src/*.cpp" "src/*.c" "src/*.hpp" "src/*.h"
)

add_executable(Emerge ${EMERGE_SRC})

# Windows: link system libs raylib expects
if (WIN32)
  target_link_libraries(Emerge PRIVATE raylib winmm gdi32 opengl32)
else()
  target_link_libraries(Emerge PRIVATE raylib)
endif()

# Where to find headers 
target_include_directories(Emerge PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if (WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
  if (NOT CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM x64 CACHE STRING "" FORCE)
  endif()
endif()

set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
if (EXISTS "${ASSETS_DIR}")
  add_custom_command(TARGET Emerge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Emerge>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "$<TARGET_FILE_DIR:Emerge>/assets"
  )
endif()
