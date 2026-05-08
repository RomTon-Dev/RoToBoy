cmake_minimum_required(VERSION 3.10)

project(MyProject C)

# Use C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include header files
include_directories(include)

# Source files
set(SOURCES
    src/main.c
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
)
