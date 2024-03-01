cmake_minimum_required (VERSION 3.8)

project ("LearnCoroutine")

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finput-charset=GBK")
endif()

foreach(src ${SRCS})
	get_filename_component(srcname ${src} NAME_WE)
	add_executable(${srcname} ${src})
endforeach()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)