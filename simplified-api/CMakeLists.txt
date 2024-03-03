cmake_minimum_required(VERSION 3.23)

project(c-argparser-simplified LANGUAGES C VERSION 0.1.0)

add_library(${PROJECT_NAME}
    include/arg/arg.h
    arg.c
)
target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET HEADERS
    FILES
    include/arg/gbuf.h
    include/arg/macro.h
)
target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
)
target_include_directories(${PROJECT_NAME} PUBLIC include)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include
    DESTINATION .
)
