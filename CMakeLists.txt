cmake_minimum_required(VERSION 3.21)
project(Emerge CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch raylib (static, no external install needed) ---
include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5.0           # pin the version
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)   # static lib
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# --- Collect sources (or list them explicitly) ---
file(GLOB EMERGE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/*.c"
)

# --- Executable target ---
add_executable(Emerge ${EMERGE_SOURCES})
target_link_libraries(Emerge PRIVATE raylib)

# --- Windows extras (link runtime statically to avoid redist) ---
if(MSVC)
  target_compile_options(Emerge PRIVATE /W3)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Working directory (so assets load via relative paths) ---
set_property(TARGET Emerge PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Emerge>")

# --- Copy assets after build (robust/guarded) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
  add_custom_command(TARGET Emerge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Emerge>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:Emerge>/assets"
    COMMENT "Copying assets to output folder"
    VERBATIM)
endif()

# --- Set the startup target in generated .sln ---
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Emerge)
