cmake_minimum_required(VERSION 3.1)
project(OlolAddon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories - use absolute paths
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../codemp
	${CMAKE_CURRENT_SOURCE_DIR}/../../shared
	${CMAKE_BINARY_DIR}/codemp/versionfile
)

# Source files
set(SOURCES
	addon_olol.cpp
	../../shared/qcommon/q_math.c
)

# Create the DLL
add_library(OlolAddon SHARED ${SOURCES})

# Set output name
set_target_properties(OlolAddon PROPERTIES
	OUTPUT_NAME "Olol"
	PREFIX ""
	SUFFIX ".dll"
)

# Compiler definitions
target_compile_definitions(OlolAddon PRIVATE
	USE_OPENAL=1
	FINAL_BUILD=1
	Q3_LITTLE_ENDIAN=1
)

# For Windows builds
if(WIN32)
	target_compile_definitions(OlolAddon PRIVATE
		WIN32
		_WINDOWS
	)

	# Disable some warnings that might come from the game headers
	target_compile_options(OlolAddon PRIVATE
		/wd4244  # conversion from 'type1' to 'type2', possible loss of data
		/wd4305  # truncation from 'type1' to 'type2'
		/wd4018  # signed/unsigned mismatch
		/wd4996  # deprecated functions
	)
endif()

# Link libraries if needed (usually not needed for simple addons)
# target_link_libraries(OlolAddon ...)