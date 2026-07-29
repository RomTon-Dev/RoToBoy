cmake_minimum_required(VERSION 3.10)

# 2. Define the project name and programming language
project(MyProject LANGUAGES C)

# 3. Set the C standard and compilation options
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_SOURCES
    src/cartridge.c
    src/mmu.c
    src/cpu.c
)

# 4. Define the core library (for ctypes) and the executable
# We separate logic into a library so both the executable and Python can use it
add_library(MyProjectLib SHARED 
    ${PROJECT_SOURCES}
)

# Move the .so to tests/lib
set_target_properties(MyProjectLib PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_data"
)

add_executable(MyProject 
    src/main.c
)

# Link the library to the executable
target_link_libraries(MyProject PRIVATE MyProjectLib)

# 5. Tell CMake where to find your header files
target_include_directories(MyProjectLib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_include_directories(MyProject PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

# 6. Add compiler warnings
target_compile_options(MyProjectLib PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(MyProject PRIVATE -Wall -Wextra -Wpedantic)

# 7. Enable testing and add pytest
enable_testing()
find_package(Python REQUIRED COMPONENTS Interpreter)

add_test(
    NAME MyProjectTests
    COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
