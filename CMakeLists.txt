cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
#project(qplt LANGUAGES CXX VERSION 1.0.0)

option(CUDA "Use CUDA for rendering" ON)

if(CUDA)
  project(qplt LANGUAGES CXX CUDA)
else()
  project(qplt LANGUAGES CXX)
endif()


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Set a default build type if none was specified
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
	set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
	STRING "Choose the type of build." FORCE)
# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()


if(CUDA)
#    find_package(CUDA REQUIRED)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_RESOLVE_DEVICE_SYMBOLS ON)
#set(CMAKE_CUDA_ARCHITECTURES OFF)
#set(CUDA_NVCC_FLAGS "${CMAKE_CXX_FLAGS} -rdc=true -c")
#	include(CheckLanguage)
#	check_language(CUDA)
	if(CMAKE_CUDA_COMPILER)
#		enable_language(CUDA)
		set(CMAKE_CUDA_STANDARD 14)
        
		cmake_policy(SET CMP0104 OLD)
	else()
		message(STATUS "No CUDA support")
		set(CUDA OFF CACHE BOOL "Use CUDA for rendering" FORCE) # Overwrite option
	endif()
endif()

if (NOT WIN32)
	set (CMAKE_CXX_FLAGS ${CMKAE_CXX_FLAGS} "-Wall -Wpedantic -fdiagnostics-color=always")
else()
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	set (CMAKE_CXX_FLAGS ${CMKAE_CXX_FLAGS} "-DAIW_WIN32 -DAIW_NO_ZLIB -DM_PI=3.14159265358979323846 -DM_2_PI='(2/M_PI)' -DMS_WIN64")
endif()

#find_package(ZLIB REQUIRED)

#add_library(aiwlib STATIC "src/debug.cpp" "src/sphere.cpp" "src/configfile.cpp" "src/segy.cpp"
#	"src/isolines.cpp" "src/checkpoint.cpp" "src/mixt.cpp" "src/racs.cpp" "src/farfield.cpp"

add_library(aiwlib OBJECT "src/debug.cpp")

target_include_directories(aiwlib PUBLIC "src/") # Due to relative paths
set_target_properties(aiwlib PROPERTIES POSITION_INDEPENDENT_CODE 1)

if (CUDA)
	set_source_files_properties("src/qplt/mesh_cu.cpp" PROPERTIES LANGUAGE CUDA)
	set_source_files_properties("src/qplt/imaging.cpp" "src/qplt/accessor.cpp" "src/qplt/base.cpp"
		"src/qplt/mesh.cpp" "src/qplt/vtexture.cpp" PROPERTIES LANGUAGE CXX)
endif()
add_library(qplt_static STATIC "src/qplt/imaging.cpp" "src/qplt/accessor.cpp" "src/qplt/base.cpp"
	"src/qplt/mesh.cpp" "src/qplt/vtexture.cpp" "src/qplt/mesh_cu.cpp")
target_include_directories(qplt_static PUBLIC "src/qplt")
set_target_properties(qplt_static PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(CUDA)
	set_target_properties(qplt_static PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
	#set_target_properties(qplt_cu_objs PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
else()
	find_package(OpenMP REQUIRED)
	target_link_libraries(qplt_static PUBLIC OpenMP::OpenMP_CXX)
endif()

find_package(Python3 COMPONENTS Interpreter Development REQUIRED) 
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.14")
	cmake_policy(SET CMP0086 NEW)
endif()
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
set_property(SOURCE "swig/qplt/core.i" PROPERTY SWIG_FLAGS -I"${CMAKE_CURRENT_SOURCE_DIR}")
set_source_files_properties("swig/qplt/core.i" PROPERTIES CPLUSPLUS ON)
swig_add_library(qplt_python LANGUAGE python SOURCES "swig/qplt/core.i" OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/aiwlib/qplt")
if(NOT WIN32)
	set_property(TARGET qplt_python PROPERTY OUTPUT_NAME qplt)
	set_property(TARGET qplt_python PROPERTY PREFIX _)
endif()
target_include_directories(qplt_python PUBLIC ${Python3_INCLUDE_DIRS} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(qplt_python PUBLIC aiwlib PUBLIC ${Python3_LIBRARIES} PRIVATE qplt_static PUBLIC ${CMAKE_DL_LIBS}) 
#add_custom_command(TARGET qplt_python_swig_compilation
#	POST_BUILD
#	COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/patch_swig.py" ${swig_generated_file_fullname}
#	COMMENT "Patching ${swig_generated_file_fullname}")
#add_custom_command(TARGET qplt_python_swig_compilation
#	POST_BUILD
#	COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/patch_swig.py" "${CMAKE_CURRENT_BINARY_DIR}/core.py" "${CMAKE_CURRENT_BINARY_DIR}/aiwlib/qplt/core.py"
#	MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/core.py"
#	COMMENT "Patching core.py")
#message("Generated file name:${swig_generated_file_fullname}")

set_property(TARGET qplt_python PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/aiwlib/qplt")
#add_executable(qplt-remote src/bin/qplt-remote.cpp)
#target_link_libraries(qplt-remote PRIVATE qplt_static PUBLIC aiwlib PUBLIC ${CMAKE_DL_LIBS})

file(COPY python3/aiwlib bin/qplt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
