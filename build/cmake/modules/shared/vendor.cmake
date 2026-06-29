include_guard (DIRECTORY)

set (REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." CACHE PATH "Root directory of the repository" FORCE)

set (CCL_VERSION_REQUIREMENT "5.1.1" EXACT)

# Add module directories recursively
list (APPEND VENDOR_MODULE_DIRS "${CMAKE_CURRENT_LIST_DIR}/..")
set (module_files "")

foreach (dir ${VENDOR_MODULE_DIRS})
	file (GLOB_RECURSE dir_module_files LIST_DIRECTORIES true "${dir}/*")
	list (APPEND module_files ${dir_module_files})
endforeach ()

foreach (entry ${module_files})
	if (IS_DIRECTORY ${entry})
		list (APPEND CMAKE_MODULE_PATH ${entry})
	endif ()
endforeach ()

list (APPEND VENDOR_INCLUDE_DIRS "${REPOSITORY_ROOT}")
list (REMOVE_DUPLICATES VENDOR_INCLUDE_DIRS)

# Detect target architecture
if ("${VENDOR_TARGET_ARCHITECTURE}" STREQUAL "")
	include (targetarch)
	target_architecture (VENDOR_TARGET_ARCHITECTURE_DETECTED)
	set (VENDOR_TARGET_ARCHITECTURE "${VENDOR_TARGET_ARCHITECTURE_DETECTED}" CACHE STRING "Target architecture")
endif ()

if ("${VENDOR_PLATFORM}" STREQUAL "")
	if (WIN32)
		set (VENDOR_PLATFORM_DETECTED win)
	elseif (APPLE)
		set (VENDOR_PLATFORM_DETECTED mac)
	elseif (UNIX)
		set (VENDOR_PLATFORM_DETECTED linux)
	endif ()
endif ()

set (VENDOR_PLATFORM ${VENDOR_PLATFORM_DETECTED} CACHE STRING "Target platform, e.g. win or zephyr.")

if ("${VENDOR_HOST_PLATFORM}" STREQUAL "")
	set (VENDOR_HOST_PLATFORM "${VENDOR_PLATFORM}" CACHE STRING "Host platform, e.g. win or mac.")
endif ()

string (REGEX REPLACE ";" "_" VENDOR_NATIVE_COMPONENT_SUFFIX "${VENDOR_PLATFORM}_${VENDOR_TARGET_ARCHITECTURE}")

# Detect fresh project generation (new project or regen with --fresh)
if (NOT VENDOR_GENERATE_FRESH)
	if (NOT VENDOR_INTERNAL_STALE)
		set (VENDOR_INTERNAL_STALE ON CACHE BOOL "Internal flag tracking stale project regeneration")
		set (VENDOR_GENERATE_FRESH ON)
	endif ()
endif ()

# Set build output directories
if ("${VENDOR_OUTPUT_DIRECTORY}" STREQUAL "")
	set (VENDOR_OUTPUT_DIRECTORY "${REPOSITORY_ROOT}/build/cmake" CACHE PATH "Output directory in which to build target files.")
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set (CMAKE_INSTALL_PREFIX "${VENDOR_OUTPUT_DIRECTORY}/${VENDOR_PLATFORM}/${CMAKE_BULD_TYPE}" CACHE PATH "Output directory in which to install built targets." FORCE)
endif ()

set (CPACK_PACKAGE_DIRECTORY "${VENDOR_OUTPUT_DIRECTORY}/archive")

# try to find global platform configuration file
if ("${CCL_PLATFORMMACROS_FILE}" STREQUAL "")
	if ("${VENDOR_PLATFORM}" STREQUAL "")
		message (FATAL_ERROR "VENDOR_PLATFORM not set.")
	endif ()
endif ()

set (platformdir "${CMAKE_CURRENT_LIST_DIR}/../${VENDOR_PLATFORM}")
set (platformdir2 "${CMAKE_CURRENT_LIST_DIR}/${VENDOR_PLATFORM}")
get_filename_component (platformdir ${platformdir} ABSOLUTE)
find_file (CCL_PLATFORMMACROS_FILE NAMES "vendor.${VENDOR_PLATFORM}.cmake" HINTS "${platformdir}" "${platformdir2}" "${CMAKE_CURRENT_LIST_DIR}" DOC "Platform specific cmake file with additional settings.")

# additional compiler and project settings
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property (GLOBAL PROPERTY USE_FOLDERS ON)
set_property (GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmake")

set (VENDOR_CXX_STANDARD 17 CACHE STRING "C++ standard, e.g. \"98\" or \"14\"")
set (CMAKE_CXX_STANDARD ${VENDOR_CXX_STANDARD})

option (VENDOR_USE_CCACHE "Use ccache to speed up builds" OFF)

if (${VENDOR_USE_CCACHE})
	include (ccache)
endif ()

option (VENDOR_ENABLE_LINK_TIME_OPTIMIZATION "Enable link time optimization for release builds" ON)

option (VENDOR_ENABLE_DEBUG_SYMBOLS "Always add debug symbols" ON)

option (VENDOR_ENABLE_PARALLEL_BUILDS "Always place generated files in the build directory to avoid accessing the same file from multiple build jobs" OFF)

# include platform-specific config file
if (EXISTS "${CCL_PLATFORMMACROS_FILE}")
	include ("${CCL_PLATFORMMACROS_FILE}")
else ()
	message (DEBUG "Could not find a vendor specific config file")
endif ()

include ("${CMAKE_CURRENT_LIST_DIR}/githubmacros.cmake")
