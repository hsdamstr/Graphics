include(FindPackageHandleStandardArgs)


if(WIN32)
	find_library(OVR_LIBRARIES LibOVR.lib)
	find_path(OVR_INCLUDE_DIR OVR_CAPI_GL.h)
	find_package_handle_standard_args(OVR DEFAULT_MSG OVR_LIBRARIES OVR_INCLUDE_DIR)

	set(OVR_INCLUDE_DIRS ${OVR_INCLUDE_DIR})
endif()
