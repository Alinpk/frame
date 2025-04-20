# Define lib name
set(LIB_NAME targetX_lib)
# Define executable file name
set(BIN_NAME targetX)

add_library(${LIB_NAME} STATIC)
target_include_directories(${LIB_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/code/include
        ${LIBEVENT_INCLUDE_DIRS}
)
include(FindPkgConfig)

target_link_libraries(${LIB_NAME} PRIVATE fmt::fmt)
target_link_libraries(${LIB_NAME} PRIVATE event)

# file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/include/*.h)
# set_target_properties(${LIB_NAME} PROPERTIES PUBLIC_HEADER ${HEADER_FILES})
# get_target_property(temp_var ${LIB_NAME} PUBLIC_HEADER)
# message(public header include :: ${temp_var})

# To add src file in current folder to library
function(build_prj_lib_with_pub)
    # Get file list
    file(GLOB_RECURSE fileList "*.cpp")
    # Add file list into lib
    target_sources(${LIB_NAME}
        PUBLIC ${fileList}
    )
endfunction()
