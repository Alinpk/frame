#!/bin/bash

cur_name=$(basename $0)
project_source_path=$(cd $(dirname $0)/..; pwd)
build_path=${project_source_path}/build
output_path=${project_source_path}/output

clean(){
    if [ -d ${build_path} ]
    then
        _path=$(pwd)
        cd ${build_path}; make clean;
        cd ${_path}; rm -rf ${build_path};
    fi
    if [ -d ${output_path} ]
    then
        rm -rf ${output_path};
    fi
}

rebuild(){
    clean
    mkdir -p ${build_path}; cd ${build_path};
    cmake ${project_source_path};
    make install;
    mkdir ${output_path};
    cp -R ${build_path}/bin/ ${output_path};
}

build(){
    if [ -d ${build_path} ]
    then
        cd ${build_path};make; make install;
    else
        rebuild;
        exit
    fi
    if [ -d ${output_path} ]
    then
        cp -R ${build_path}/bin/ ${output_path};
    else
        mkdir ${output_path}; cp -R ${build_path}/bin/ ${output_path};
    fi
}

run(){
    if [ -d ${output_path} ]
    then
        ${output_path}/bin/WebServer
    fi
}

case ${cur_name} in
rebuild)
    rebuild
    ;;
clean)
    clean
    ;;
build)
    build
    ;;
llt)
    run
    ;;
..)
    ;;
esac