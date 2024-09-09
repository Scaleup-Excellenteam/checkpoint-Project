cmake_minimum_required(VERSION 3.28)
project(CheckPoint C)

# Set the C standard to C17
set(CMAKE_C_STANDARD 17)

# Add the cJSON library (assuming you cloned cJSON into the 'cJSON' directory)
add_library(cjson cJSON/cJSON.c)
include_directories(cJSON)

# Include the directory for cJSON headers
include_directories(cJSON)

# Add your project executable, including the necessary source files
add_executable(CheckPoint main.c
        FixedSizeHeap.c
        FixedSizeHeap.h
        student_management_system.c
        student_management_system.h
)

# Link the cJSON library to your executable
target_link_libraries(CheckPoint cjson)
