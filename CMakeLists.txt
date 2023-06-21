cmake_minimum_required(VERSION 3.0.0)

project(Project)
#set(CMAKE_EXE_LINKER_FLAGS -static)
#set(CMAKE_LIBRARY_LINKER_FLAGS -static)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -stdlib=libc++)



set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)



aux_source_directory(src src)
add_executable(Main ${src})
target_link_libraries(Main c++ c++abi amqpcpp_libc++ ssl crypto)
set_target_properties(Main PROPERTIES LINK_FLAGS "-Wl,-rpath,./lib,-dynamic-linker,./lib/ld-linux-x86-64.so.2")


aux_source_directory(test test)
add_executable(test ${test})
target_link_libraries(test c++ c++abi amqpcpp_libc++)


