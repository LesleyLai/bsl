add_library(common
    ast.hpp
    scanner.hpp
    scanner.cpp)
target_link_libraries(common PRIVATE compiler_warnings)
target_include_directories(common PUBLIC "${CMAKE_SOURCE_DIR}/include")

if(${BEYOND_SHADING_LANGUAGE_BUILD_TESTS_COVERAGE})
    target_compile_options(common PUBLIC -fprofile-arcs -ftest-coverage)
    target_link_libraries(common PUBLIC gcov)
endif()

add_executable(app "main.cpp")
target_link_libraries(app
    PRIVATE common compiler_warnings)
add_clangformat(app)
