cmake_minimum_required(VERSION 3.13)
project(test)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-Wall)

find_package(PkgConfig REQUIRED)
pkg_check_modules(
    GST REQUIRED 
    gstreamer-1.0
)

message("GST_INCLUDE_DIRS: ${GST_INCLUDE_DIRS}")
message("GST_LIBRARY_DIRS: ${GST_LIBRARY_DIRS}")
message("GST_PLUGINS_GOOD_LIBRARIES: ${GST_PLUGINS_GOOD_LIBRARIES}")

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES 
    main.cpp
)
 
 include_directories(
    ${GST_INCLUDE_DIRS}
)

link_directories(${GST_LIBRARY_DIRS})


# Add executable target with source files listed in SOURCE_FILES variable
add_executable(test ${SOURCE_FILES})

target_link_libraries(test ${GST_LIBRARIES})
