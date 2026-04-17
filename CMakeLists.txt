cmake_minimum_required(VERSION 3.20)
project(omni_media_linker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(omni_media_linker
        src/main.cpp
        src/common.cpp
        src/core/dispatcher.cpp
        src/core/mapper.cpp
        src/os/actions.cpp
        src/os/scanner.cpp
        src/util/config_loader.cpp
        include/common.h
        include/core/dispatcher.h
        include/core/mapper.h
        include/os/actions.h
        include/os/scanner.h
        include/util/config_loader.h)

target_include_directories(omni_media_linker PRIVATE include)
target_link_libraries(omni_media_linker PRIVATE user32)

if(WIN32)
    if(MSVC)
        set(variables
                CMAKE_CXX_FLAGS_RELEASE
                CMAKE_C_FLAGS_RELEASE)
        foreach(variable ${variables})
            string(REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endforeach()
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    endif()
endif()
