cmake_minimum_required(VERSION 3.16.3) # closely match with Latest Raspberry PI OS cmake distro

set(CMAKE_CXX_STANDARD 14)

project(my8088
        VERSION
        1.0
        DESCRIPTION
        "my8088, an 8088 single board computer user interface."
        LANGUAGES
        CXX
        )

find_package(PkgConfig REQUIRED)
find_package(Curses REQUIRED)
find_package(Threads REQUIRED)
find_library(wiringPi_LIB wiringPi)

add_executable(my8088
        my8088.cpp
        spi8088.cpp
        spi8088.h)

target_link_libraries(my8088 ${CURSES_LIBRARIES} ${wiringPi_LIB} ${CMAKE_THREAD_LIBS_INIT})
