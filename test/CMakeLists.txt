file(GLOB_RECURSE fileList "*.cpp")

add_executable(${BIN_NAME} ${fileList})

target_link_libraries(${BIN_NAME} fmt-header-only gtest ${LIB_NAME})