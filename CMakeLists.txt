cmake_minimum_required(VERSION 2.8.12)   # FindImageMagick needs to set ImageMagick_VERSION_STRING

#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")

#set(CMAKE_C_COMPILER "/local/kuhl-public-share/gcc-10.1/bin/gcc")
#set(CMAKE_CXX_COMPILER "/local/kuhl-public-share/gcc-10.1/bin/g++")


#set(CMAKE_C_COMPILER "clang")
#set(CMAKE_CXX_COMPILER "clang++")

project("opengl-examples" C CXX)  # This project uses C and C++

if(NOT WIN32)
# Add "-pg" to to both the C_FLAGS and the CXX_FLAGS to enable profiling
# -O2 or -O3 is necessary to get the compiler to inline the inlined functions used by vecmat.c and vecmat.h
# -march=native will cause the compiler to use features that are available on the current CPU (SSE, etc)---but will make the resulting executable less portable.
#   -march=native is removed because it doesn't work on IVS as of May 2015
# -pedantic forces warnings when non-C99 extensions are used.

# Options which we may add in future (not supported by older compilers):
# -Wshadow should probably be added eventually
# -Wvla should be included because Visual Studio does not support VLAs, so we avoid using them. They are optional in C11 even though they were part of C99.
set (COMMON_FLAGS " -Wall -Wextra -pedantic -g -Wformat -Wno-unused-parameter -Wno-unused-function") # used for C and C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=gnu99 -Wno-deprecated-declarations")

# Use no-variadic-macros to prevent C++ programs from warning about the variadic msg() macro
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++11 -Wno-variadic-macros")
endif() # end if not windows


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules)

# Write programs and libraries out to "bin" directory. foreach loop is
# needed to make it the destination for all programs on Windows (don't
# use Debug, Release folders)
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin )
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


# Include rpath
# SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


function(kuhl_add_library_path path)
	message(STATUS "Looking for: ${path}")
	file(GLOB POSSIBLE_PATHS ${path})
	foreach(possible_path ${POSSIBLE_PATHS})
		if(EXISTS ${possible_path})
			message(STATUS "      Found: ${possible_path}")
			set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${possible_path} CACHE INTERNAL "")
		endif()
	endforeach()
endfunction(kuhl_add_library_path)

function(kuhl_add_include_path path)
	message(STATUS "Looking for: ${path}")
	file(GLOB POSSIBLE_PATHS ${path})
	foreach(possible_path ${POSSIBLE_PATHS})
		if(EXISTS ${possible_path})
			message(STATUS "      Found: ${possible_path}")
			set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${possible_path} CACHE INTERNAL "")
		endif()
	endforeach()
endfunction(kuhl_add_include_path)

function(kuhl_add_paths path)

	if(NOT EXISTS ${path})
		return()
	endif()

	# GLFW
	if(NOT GLFW_glfw_LIBRARY)
		kuhl_add_library_path(${path}/glfw/src)
		kuhl_add_include_path(${path}/glfw/include)
		if(WIN32) # GLFW - Windows
			kuhl_add_include_path(${path}/glfw-*/include)
			kuhl_add_library_path(${path}/glfw-*/src/Debug)   # compiled from source
			kuhl_add_library_path(${path}/glfw-*/src/Release) # compiled from source
			kuhl_add_library_path(${path}/glfw-*/lib-vc2019)  # precompiled
			kuhl_add_library_path(${path}/glfw-*/lib-vc2017)  # precompiled
			kuhl_add_library_path(${path}/glfw-*/lib-vc2015)  # precompiled
			kuhl_add_library_path(${path}/glfw-*/lib-vc2013)  # precompiled
			kuhl_add_library_path(${path}/glfw-*/lib-vc2012)  # precompiled
		endif()
	endif()

	# GLEW
	if(NOT GLEW_LIBRARY)
		kuhl_add_library_path(${path}/glew/lib)
		kuhl_add_include_path(${path}/glew/include)
		if(WIN32) # GLEW - Windows
			# We also copy an assimp DLL in samples/CMakeLists.txt
			kuhl_add_library_path(${path}/glew-*/lib/Release/x64)
			kuhl_add_include_path(${path}/glew-*/include)
		endif()
	endif()

	# Assimp
	if(NOT ASSIMP_LIBRARIES)
		kuhl_add_library_path(${path}/assimp/bin)   # used lib folder prior to assimp 5.1 (changed commit be4fe13)
		kuhl_add_include_path(${path}/assimp/include)
		if(WIN32) # ASSIMP - Windows
			# We also copy an assimp DLL in samples/CMakeLists.txt

			# location if standalone cmake program is used
			kuhl_add_library_path(${path}/assimp-*/lib/Debug)
			kuhl_add_include_path(${path}/assimp-*/include)

		    # location if VS cmake is used (does not work?)
			#kuhl_add_library_path(${path}/assimp-*/out/build/x64-Debug/lib)  
			#kuhl_add_include_path(${path}/assimp-*/out/build/x64-Debug/include)
		endif()
	endif()

	# ImageMagick
	if(NOT DEFINED ImageMagick_MagickWand_LIBRARY)
		# NOTE: cmake currently searches for IM 6 before IM 7. So, if
		# IM6 is installed on the system but IM 7 is self-compiled
		# locally, it will use system IM 6.
		kuhl_add_library_path(${path}/ImageMagick/MagickWand/.libs)
		kuhl_add_include_path(${path}/ImageMagick/MagickWand)

		# Windows: Finds ImageMagick by default as long as you select
		# the checkbox so development files/libraries are installed.

		# macOS homebrew: Installs imagemagick7, works by default.
	endif()

	# VRPN
	if(NOT DEFINED VRPN_LIBRARY_VRPN)
		if(WIN32)
			kuhl_add_library_path(${path}/vrpn/build/Debug)
			kuhl_add_library_path(${path}/vrpn/build/quat/Debug)
			kuhl_add_library_path(${path}/vrpn/pc_win32/Debug)
			kuhl_add_library_path(${path}/vrpn/quat/pc_win32/Debug)
			kuhl_add_include_path(${path}/vrpn)
			kuhl_add_include_path(${path}/vrpn/quat)
		else()
			kuhl_add_library_path(${path}/vrpn/build)
			kuhl_add_library_path(${path}/vrpn/build/quat)
			kuhl_add_include_path(${path}/vrpn)
			kuhl_add_include_path(${path}/vrpn/quat)
		endif()
	endif()

	# Oculus SDK for windows
	if(NOT DEFINED OVR_LIBRARIES AND DEFINED WIN32)
		kuhl_add_include_path(${path}/OculusSDK/LibOVR/Include)
		kuhl_add_include_path(${path}/ovr_sdk_win*/LibOVR/Include)
		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			kuhl_add_library_path(${path}/OculusSDK/LibOVR/Lib/Windows/x64/Debug/VS*/)
			kuhl_add_library_path(${path}/ovr_sdk_win*/LibOVR/Lib/Windows/x64/Debug/VS*/)
			#kuhl_add_library_path(${path}/OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2017/)
			#kuhl_add_library_path(${path}/OculusSDK/LibOVR/Lib/Windows/x64/Release)
		else()
			kuhl_add_library_path(${path}/OculusSDK/LibOVR/Lib/Windows/Win32/Debug/VS*)
			kuhl_add_library_path(${path}/ovr_sdk_win*/LibOVR/Lib/Windows/Win32/Debug/VS*)
			#kuhl_add_library_path(${path}/OculusSDK/LibOVR/Lib/Windows/Win32/Release)
		endif()
	endif()

endfunction(kuhl_add_paths)

if(NOT WIN32)
	kuhl_add_paths(/home/kuhl/public-ogl) # CCSR, self-administered machines
	kuhl_add_paths(/local/kuhl-public-share/opengl) # MTU campus linux lab machines
endif()
kuhl_add_paths(${CMAKE_CURRENT_SOURCE_DIR}/..) # Alongside this folder
kuhl_add_paths(${CMAKE_CURRENT_SOURCE_DIR}/../opengl-libs) # Alongside this folder in a folder named opengl-libs

# SITE_NAME sets the HOSTNAME variable to the name of this computer.
SITE_NAME(HOSTNAME)

# Doxygen documentation
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" ON)
if(BUILD_DOCUMENTATION)
	find_package(Doxygen)
	if(DOXYGEN_FOUND AND (NOT (${DOXYGEN_EXECUTABLE} MATCHES "DOXYGEN_EXECUTABLE-NOTFOUND")) AND EXISTS ${CMAKE_SOURCE_DIR}/Doxyfile)
		#-- Add a custom target to run Doxygen when ever the project is built
		add_custom_target (docs
		    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
		    SOURCES ${CMAKE_SOURCE_DIR}/Doxyfile)
		# Add or remove 'ALL' after 'docs' above to enable/disable doxygen generation when running "make"

		# Don't print warning if Doxygen doesn't work since many people don't use them.
	endif()
endif()

if (NOT WIN32)
	# --- math library ---
	find_library(M_LIB m)
endif()

# --- OpenGL ---
set(OpenGL_GL_PREFERENCE "GLVND") # fix cmake warning on Ubuntu 19.04
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# --- GLFW3 ---
find_package(GLFW REQUIRED)
include_directories(${GLFW_INCLUDE_DIR})

# --- GLEW ---
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

# --- ASSIMP ---
find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIRS})



# --- ImageMagick (recommended, optional) ---
#
# See help/IMAGEMAGICK_INSTALL.txt for information about the versions
# that this code works with.
find_package(ImageMagick COMPONENTS MagickWand)
if(ImageMagick_MagickWand_FOUND)
	include_directories(${ImageMagick_INCLUDE_DIRS})
	# Assume version 7 if version string isn't defined. It doesn't get defined on Windows.
	if(DEFINED ImageMagick_VERSION_STRING AND "${ImageMagick_VERSION_STRING}" VERSION_LESS 7.0.0)
		set(IMAGEMAGICK_FOUND_DEFINITION "KUHL_UTIL_USE_IMAGEMAGICK;IMAGEMAGICK_LEGACY")
	else()
		set(IMAGEMAGICK_FOUND_DEFINITION "KUHL_UTIL_USE_IMAGEMAGICK")
	endif()
else()
	set(IMAGEMAGICK_FOUND_DEFINITION "")
endif()



# --- FreeType ---
find_package(Freetype)
if (FREETYPE_FOUND)
	include_directories(${FREETYPE_INCLUDE_DIRS})
	set(FREETYPE_FOUND_DEFINITION "KUHL_UTIL_USE_FREETYPE")
else()
	set(FREETYPE_FOUND_DEFINITION "")
endif()


# --- VRPN ---
#
# macOS: As of Oct 2017, using Clang as a compiler results in an error
# when compiling against VRPN.
find_package(VRPN)
if(VRPN_FOUND)
	include_directories(${VRPN_INCLUDE_DIRS})
	set(MISSING_VRPN_DEFINITION "")
else()
	set(MISSING_VRPN_DEFINITION "MISSING_VRPN")
endif()


# --- FFMPEG ---
find_package(FFmpeg)
if(FFMPEG_FOUND)
	include_directories(${FFMPEG_INCLUDE_DIRS})
	set(HAVE_FFMPEG_DEFINITION "HAVE_FFMPEG")
	else()
	set(HAVE_FFMPEG_DEFINITION "")
endif()


# --- LibOVR (Oculus Rift) ---
set(MISSING_OVR_DEFINITION "MISSING_OVR")
if(WIN32)
	find_package(OVR)
	if(OVR_FOUND)
		include_directories(${OVR_INCLUDE_DIRS})
		set(MISSING_OVR_DEFINITION "")
	endif()
endif()

# Set the preprocessor flags.
set(PREPROC_DEFINE "${FREETYPE_FOUND_DEFINITION};${ASSIMP_FOUND_DEFINITION};${MISSING_VRPN_DEFINITION};${MISSING_OVR_DEFINITION};${IMAGEMAGICK_FOUND_DEFINITION};${HAVE_FFMPEG_DEFINITION}")

# Look in lib folder for libraries and header files
include_directories("lib")


# Process cmake files in the subdirectories.
# build libkuhl.a
add_subdirectory(${PROJECT_SOURCE_DIR}/lib) 
# build sample programs
add_subdirectory(${PROJECT_SOURCE_DIR}/samples)
# build fake vrpn server
add_subdirectory(${PROJECT_SOURCE_DIR}/vrpn)
# build self tests
add_subdirectory(${PROJECT_SOURCE_DIR}/selftests)

