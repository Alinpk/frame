function(git_download url dir)
    if(NOT EXISTS ${dir})
        execute_process(COMMAND mkdir ${dir})
        execute_process(COMMAND git clone ${url} ${dir})
    endif()
endfunction()

list(APPEND URL "https://github.com/google/googletest.git" "https://github.com/fmtlib/fmt.git")
list(APPEND DIR googletest fmt)

set(DEPEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependence")

foreach(repo IN ZIP_LISTS DIR URL)
    set(dir ${DEPEND_DIR}/${repo_0})
    set(url ${repo_1})

    git_download(${url} ${dir})
    add_subdirectory(${dir})
endforeach()