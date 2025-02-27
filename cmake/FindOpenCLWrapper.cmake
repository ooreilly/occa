###############################################################################
# FIND module wrapper around finding OpenCL
# This Find module is also distributed alongside the occa package config file!
###############################################################################

# Look in some default places for OpenCL and set OPENCL_ROOT if not already set
if(NOT OPENCL_ROOT)
  # Search in user specified path first
  find_path(OPENCL_ROOT
    NAMES CL/cl.h
    PATHS
    ENV   OPENCL_PATH
    DOC   "OPENCL root location"
    NO_DEFAULT_PATH)

  # Now search in default path
  find_path(OPENCL_ROOT
    NAMES CL/cl.h
    PATHS 
      /usr 
      /opt/rocm/opencl 
      /usr/local/cuda 
      /opt/intel/oneapi/compiler/latest/linux
    PATH_SUFFIXES sycl
    DOC   "OPENCL root location")
endif()

# Trick CMake's default OpenCL module to look in our directory
set(ENV{AMDAPPSDKROOT} ${OPENCL_ROOT})

find_package(OpenCL)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OpenCLWrapper
    REQUIRED_VARS
    OpenCL_FOUND
    )
