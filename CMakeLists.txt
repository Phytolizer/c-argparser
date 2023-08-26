cmake_minimum_required(VERSION 3.20)

project(c-argparser LANGUAGES C VERSION 0.1.0)

add_library(c-argparser
    str.c
    arg.c
)
target_compile_features(c-argparser PUBLIC c_std_11)
set_target_properties(c-argparser PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
target_include_directories(c-argparser PUBLIC include)
