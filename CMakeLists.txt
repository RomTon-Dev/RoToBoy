cmake_minimum_required(VERSION 3.10)

# 2. Define the project name and programming language
project(MyProject LANGUAGES C)

# 3. Set the C standard (e.g., C99 or C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 4. Define your executable and list your source files
# It is better to list them explicitly than to use file(GLOB ...)
add_executable(MyProject 
    src/main.c 
    src/cartridge.c
)

# 5. Tell CMake where to find your header files
target_include_directories(MyProject PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
