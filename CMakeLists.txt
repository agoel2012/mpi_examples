cmake_minimum_required(VERSION 3.16)

project(MPIExamples)

include_directories(include)
add_executable(CommExamples primitives.c)
target_compile_options(CommExamples PRIVATE -g -O3 -Werror -Wall)
target_link_libraries(CommExamples PUBLIC mpich
				 PUBLIC pthread)
