if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
	message(FATAL_ERROR "Don't run cmake here. Run it in the root folder of this repository.")
endif()
cmake_minimum_required(VERSION 2.8.12)


####################################
# Edit the following areas to add or remove programs to compile
#
# If you add a new name here, there must be an .c or .cpp file with the same
# name that contains a main() function.
####################################
set(PROGRAMS_TO_MAKE triangle triangle-shade triangle-color texture texturefilter glinfo teartest picker prerend panorama pong text ogl2-slideshow ogl2-triangle ogl2-texture tracker-stats videoplay zfight viewer slerp explode flock flock-instanced frustum ik tracker-demo distjudge merry infinicity terrain avatar)


# Make a target that lets us copy all of the vert and frag files from this directory into the bin directory.
# file(GLOB ShaderFiles "*.vert" "*.frag")
# add_custom_target(copyGLSL)
# foreach(ShaderFile ${ShaderFiles})
#     add_custom_command(TARGET copyGLSL PRE_BUILD
#         COMMAND ${CMAKE_COMMAND} -E
#         copy ${ShaderFile} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# endforeach()


if(WIN32)
    # Add a cleanup target that we finish setting up below.
	add_custom_target(cleanup)

	# On Windows, we need to copy two DLL files into the same directory as our program. 
	# If we don't, windows will complain about the missing DLL files when we run the exe.
	string(REGEX REPLACE "[.]lib$" ".dll" ASSIMP_DLL_TO_COPY ${ASSIMP_LIBRARIES})
	string(REGEX REPLACE "[.]lib$" ".dll" GLEW_DLL_TO_COPY ${GLEW_LIBRARY})
	#message("copyDllOnWindows: string replace: ${ASSIMP_DLL_TO_COPY} ${GLEW_DLL_TO_COPY}")
	get_filename_component(ASSIMP_DLL_TO_COPY ${ASSIMP_DLL_TO_COPY} NAME)
	get_filename_component(GLEW_DLL_TO_COPY ${GLEW_DLL_TO_COPY} NAME)
	#message("copyDllOnWindows:  basename only: ${ASSIMP_DLL_TO_COPY} ${GLEW_DLL_TO_COPY}")
	file(GLOB ASSIMP_DLL_PATH "${CMAKE_SOURCE_DIR}/../opengl-libs/assimp-*/bin/Debug/${ASSIMP_DLL_TO_COPY}")
	file(GLOB GLEW_DLL_PATH "${CMAKE_SOURCE_DIR}/../opengl-libs/glew-*/bin/Release/x64/${GLEW_DLL_TO_COPY}")
	#message("copyDllOnWindows:  path: ${ASSIMP_DLL_PATH} ${GLEW_DLL_PATH}")

	add_custom_target(copyDllOnWindows)
	if(EXISTS ${ASSIMP_DLL_PATH})
		message("Will copy assimp dll from ${ASSIMP_DLL_PATH} to bin folder after building.")
		add_custom_command(TARGET copyDllOnWindows
		                   COMMAND ${CMAKE_COMMAND} -E echo "Copying to bin: ${ASSIMP_DLL_PATH}"
						   COMMAND ${CMAKE_COMMAND} -E
						   copy ${ASSIMP_DLL_PATH} ${CMAKE_SOURCE_DIR}/bin)
	endif()
	if(EXISTS ${GLEW_DLL_PATH})
		message("Will copy glew dll from ${GLEW_DLL_PATH} to bin folder after building.")
		add_custom_command(TARGET copyDllOnWindows
		                   COMMAND ${CMAKE_COMMAND} -E echo "Copying to bin: ${GLEW_DLL_PATH}"
						   COMMAND ${CMAKE_COMMAND} -E copy ${GLEW_DLL_PATH} ${CMAKE_SOURCE_DIR}/bin)
	endif()
endif()



# Compile the list of programs.
foreach(arg ${PROGRAMS_TO_MAKE})
	if(EXISTS ${CMAKE_SOURCE_DIR}/samples/${arg}.c)   # Figure out if the program is a c or cpp file
		set(SOURCE_FILE ${arg}.c)
	else()
		set(SOURCE_FILE ${arg}.cpp)
	endif()
	add_executable(${arg} ${SOURCE_FILE})

	target_link_libraries(${arg} kuhl)
	target_link_libraries(${arg} ${ASSIMP_LIBRARIES})
	
	# === Optional libraries: ===
	if(VRPN_FOUND)  # Add VRPN to the list if it is available
		target_link_libraries(${arg} ${VRPN_LIBRARIES})
	endif()
	if(OVR_FOUND) # Add Oculus LibOVR to the list if it is available
		target_link_libraries(${arg} ${OVR_LIBRARIES} ${CMAKE_DL_LIBS})
	endif()
	if(ImageMagick_FOUND)
		target_link_libraries(${arg} ${ImageMagick_LIBRARIES})
	endif()
	if(FREETYPE_FOUND)
		target_link_libraries(${arg} ${FREETYPE_LIBRARIES})
	endif()
	if(FFMPEG_FOUND)
		target_link_libraries(${arg} ${FFMPEG_LIBRARIES})
	endif()

	target_link_libraries(${arg} ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} ${M_LIB} ${OPENGL_LIBRARIES} )
	if(APPLE)
		# Some Mac OSX machines need this to ensure that freetype.h is found.
		target_include_directories(${arg} PUBLIC "/opt/X11/include/freetype2/")
	endif()

	set_target_properties(${arg} PROPERTIES LINKER_LANGUAGE "CXX")
	set_target_properties(${arg} PROPERTIES COMPILE_DEFINITIONS "${PREPROC_DEFINE}")

	if(WIN32)
		# Copy dll files after build
		add_dependencies(${arg} copyDllOnWindows)

		# Add stuff to remove to cleanup target.
		add_custom_command(TARGET cleanup
		                   COMMAND ${CMAKE_COMMAND} -E echo "cleanup: ${arg}"
		                   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/${arg}.exe
		                   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/${arg}.ilk
		                   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/${arg}.pdb
						   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/${GLEW_DLL_TO_COPY}
						   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/${ASSIMP_DLL_TO_COPY}
						   COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/bin/kuhl.lib
		)
	endif()
endforeach()

if(WIN32)
	add_custom_command(TARGET cleanup
	                   COMMAND ${CMAKE_COMMAND} -E echo "To submit assignment: Exclude '.git', '.vs' and 'out' when you compress files+folders in ${CMAKE_SOURCE_DIR} into a zip file.")
endif()	
