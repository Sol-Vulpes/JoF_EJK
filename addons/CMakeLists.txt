# Addons directory CMakeLists.txt
# This file builds all addons

# Get all subdirectories in the addons folder
file(GLOB addon_dirs LIST_DIRECTORIES true "*")

foreach(dir ${addon_dirs})
    if(IS_DIRECTORY ${dir} AND EXISTS "${dir}/CMakeLists.txt")
        message(STATUS "Adding addon: ${dir}")
        add_subdirectory(${dir})
    endif()
endforeach()