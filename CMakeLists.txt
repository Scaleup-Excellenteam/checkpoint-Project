cmake_minimum_required(VERSION 3.28)
project(CheckPoint C)

set(CMAKE_C_STANDARD 17)

# Add the cJSON library (assuming you cloned it into the cJSON directory)
add_library(cjson ../cJSON/cJSON.c)

# Add the executable, including your project files
add_executable(CheckPoint ../main.c
        ../Heaps.c
        ../Heaps.h
        FixedSizeHeap.c
        FixedSizeHeap.h
        student_management_system.h
        student_management_system.c
)

# Link the cJSON library to your executable
target_link_libraries(CheckPoint cjson)
