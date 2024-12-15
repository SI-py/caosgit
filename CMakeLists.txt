cmake_minimum_required(VERSION 3.10)
project(caosgit VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(caosgit
    main.cpp
    command_parser.cpp
    repository.cpp
    utils.cpp
)

if (UNIX)
    target_compile_options(caosgit PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()