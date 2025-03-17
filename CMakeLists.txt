cmake_minimum_required(VERSION 3.15)

# Define the project name
set(PROJECT_NAME WebServer)

# Define project
project(${PROJECT_NAME})
set(CMAKE_BUILD_TYPE Debug)

# set a project version
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 0)
set (${PROJECT_NAME}_VERSION_PATCH 1)
set (${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

# Set compiler standard
set(CMAKE_CXX_STANDARD 20)
# macos should add following additional command
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fexperimental-library -stdlib=libc++")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -I/usr/include/mysql")

include(${PROJECT_SOURCE_DIR}/cmake/target.cmake)
# include(${PROJECT_SOURCE_DIR}/cmake/cmake/FindMySQL.cmake)
include(${PROJECT_SOURCE_DIR}/dependence/dependence.cmake)

# find_package(MySQL MODULE REQUIRED)

add_subdirectory(code)
enable_testing()
add_subdirectory(test)

install(TARGETS ${LIB_NAME}
        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib  # ��̬�ⰲװ·��
        ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib  # ��̬�ⰲװ·��
        PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/include  # ͷ�ļ���װ·��
        )

install(TARGETS ${BIN_NAME}
        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
)

# Config
# install (FILES cmake-examples.conf
#     DESTINATION ${CMAKE_BINARY_DIR}/etc)

############################################################
# Create DEB
############################################################

# Tell CPack to generate a .deb package
set(CPACK_GENERATOR "ZIP")

# Set a Package Maintainer.
# This is required
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Thom Troy")

# Set a Package Version
set(CPACK_PACKAGE_VERSION ${${PROJECT_NAME}_VERSION})

# Include CPack
include(CPack)