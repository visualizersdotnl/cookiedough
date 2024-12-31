cmake_minimum_required(VERSION 3.25)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_OSX_DEPLOYMENT_TARGET 13.61)

if (APPLE)
    set (CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang" CACHE STRING "C compiler" FORCE)
    set (CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE STRING "C++ compiler" FORCE)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "../target/osx")
endif()

if (LINUX)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "../target/linux")
endif()

set(CMAKE_C_FLAGS "-Wall -fno-exceptions -O3 -ffast-math")
set(CMAKE_CXX_FLAGS "-Wall -fno-exceptions -msse4.1")

if (APPLE)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libomp")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(cookiedough)

file(GLOB CKD_SOURCEFILES *.cpp)
file(GLOB ROCKET_SOURCEFILES ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/rocket-stripped/lib/*.c)

add_executable(cookiedough ${CKD_SOURCEFILES} ${ROCKET_SOURCEFILES})

target_compile_definitions(cookiedough PUBLIC
	CMAKE_BUILD=1 
    $<$<CONFIG:Debug>:
        _DEBUG=1
    >
)

if (APPLE)
    target_link_options(cookiedough PUBLIC -L/opt/homebrew/lib -L${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/bass24-osx/arm64 -rpath ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/bass24-osx/arm64 -fopenmp=libomp)
endif()

if (LINUX)
    target_link_options(cookiedough PUBLIC -L${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/bass24-linux/x64 -fopenmp)
    # $ORIGIN/lib/libmylib.so
endif()

target_link_libraries(cookiedough PUBLIC bass SDL2 IL)
