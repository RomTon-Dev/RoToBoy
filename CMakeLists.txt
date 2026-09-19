cmake_minimum_required(VERSION 3.10)

# 2. Define the project name and programming language
project(RoToBoy LANGUAGES C)

# 3. Set the C standard and compilation options
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find the SDL2 library (Required for window management and rendering)
find_package(SDL2 REQUIRED)

# Include ALL subsystem source files
set(PROJECT_SOURCES
    src/apu.c
    src/cartridge.c
    src/cpu.c
    src/joypad.c
    src/mmu.c
    src/ppu.c
    src/timer.c
    src/window.c
)

# 4. Define the core library (for ctypes) and the executable
# We separate logic into a library so both the executable and Python can use it
add_library(RoToBoyLib SHARED 
    ${PROJECT_SOURCES}
)

# Move the .so to tests/test_data
set_target_properties(RoToBoyLib PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_data"
)

add_executable(RoToBoy 
    src/main.c
)

# Link the core library and SDL2 to the executable and library
target_link_libraries(RoToBoyLib PUBLIC ${SDL2_LIBRARIES})
target_link_libraries(RoToBoy PRIVATE RoToBoyLib ${SDL2_LIBRARIES})

# 5. Tell CMake where to find your header files, including SDL2
target_include_directories(RoToBoyLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${SDL2_INCLUDE_DIRS}
)
target_include_directories(RoToBoy PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${SDL2_INCLUDE_DIRS}
)

# 6. Add compiler warnings
target_compile_options(RoToBoyLib PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(RoToBoy PRIVATE -Wall -Wextra -Wpedantic)

# 7. Enable testing and add pytest
enable_testing()
find_package(Python REQUIRED COMPONENTS Interpreter)

add_test(
    NAME RoToBoyTests
    COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
