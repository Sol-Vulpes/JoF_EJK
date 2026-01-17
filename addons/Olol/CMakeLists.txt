cmake_minimum_required(VERSION 3.1)
project(OlolAddon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
	../../codemp
	../../shared
)

# Source files
set(SOURCES
	addon_olol.cpp
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
endif()

# Link libraries if needed (usually not needed for simple addons)
# target_link_libraries(OlolAddon ...)