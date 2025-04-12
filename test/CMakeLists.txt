file(GLOB_RECURSE fileList "*.cpp")

include_directories(${CMAKE_SOURCE_DIR}/test)
add_executable(${BIN_NAME} ${fileList})
target_link_libraries(${BIN_NAME} fmt-header-only gtest event event_pthreads ${LIB_NAME})