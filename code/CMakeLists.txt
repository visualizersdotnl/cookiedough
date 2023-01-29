cmake_minimum_required(VERSION 3.25)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang" CACHE STRING "C compiler" FORCE)
set (CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++" CACHE STRING "C++ compiler" FORCE)

project(cookiedough)
file(GLOB CKD_SOURCEFILES *.cpp)
file(GLOB ROCKET_SOURCEFILES ../3rdparty/rocket-stripped/lib/*.c)

add_executable(cookiedough ${CKD_SOURCEFILES} ${ROCKET_SOURCEFILES})
target_compile_options(cookiedough PUBLIC -fopenmp=libomp -fno-exceptions -O3 -DNDEBUG)
target_link_options(cookiedough PUBLIC -L/opt/homebrew/lib -L../../3rdparty/bass24-osx/arm64 -rpath ../../3rdparty/bass24-osx/arm64 -lBASS -lSDL2 -lIL -fopenmp=libomp)