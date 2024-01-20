# FILE PURPOSE: Provide a CMake "find module" to locate parts of the Microsoft Game Development Kit (MSGDK) installed on the system.
# This module is NOT official and is NOT affiliated with Microsoft.
# It should be considered as part of the public domain, regardless of any licenses applying to other contents this file is shipped along.
# Hence, no warranties or similar concepts apply to the functionality provided by the module.

set(fancy_name "MSGDK (Microsoft Game Development Kit)")

set(search_paths
    "C:/Program Files/Microsoft GDK/"
    "C:/Program Files (x86)/Microsoft GDK/"
)

if(NOT WIN32)
    message(FATAL_ERROR "${fancy_name} is not supported on platforms that are not Windows or Windows-like.")
endif()

if(DEFINED MSDGK_ROOT_DIR)
    list(APPEND search_paths "${MSDGK_ROOT_DIR}/")
endif()

string(REPEAT [0-9] 6 any_six_digits)
set(version_regex "^${any_six_digits}$")

foreach(path ${search_paths})
    # check if directory exists
    cmake_path(ABSOLUTE_PATH path NORMALIZE)
    if(NOT EXISTS "${path}")
        continue()
    endif()

    # iterate over subdirectories
    file(GLOB subdirs LIST_DIRECTORIES TRUE "${path}*")
    foreach(dir ${subdirs})
        if(NOT IS_DIRECTORY "${dir}")
            continue()
        endif()
        
        # identify version dir
        cmake_path(GET dir FILENAME dirname)
        if(${dirname} MATCHES ${version_regex})
            if(EXISTS "${dir}/GRDK/GameKit/") # directory tree check
                list(APPEND found_versions ${dirname})
                list(APPEND found_version_paths "${dir}")
                break()
            endif()
        endif()
    endforeach()
endforeach()

list(LENGTH found_versions num_versions)
if(${num_versions} LESS 1)
    # -> no installs found
    if(NOT MSGDK_FIND_QUIETLY)
        message(FATAL_ERROR "Unable to locate ${fancy_name}. Searched directories: ${search_paths}")
    endif()

    set(MSGDK_FOUND FALSE)
    return()
endif()

set(use_version 0)
set(base_path "")

if(DEFINED MSGDK_FIND_VERSION)
    if((NOT ${MSGDK_FIND_VERSION} MATCHES ${version_regex}) AND (NOT MSGDK_FIND_QUIETLY))
        message(FATAL_ERROR "Requested ${fancy_name} version \"${MSGDK_FIND_VERSION}\" does not match the conventional format of six numerical digits.")
    endif()

    # use specific version
    list(FIND found_versions ${MSGDK_FIND_VERSION} specific_version_index)
    if(${specific_version_index} LESS 0)
        # -> specific version not found
        if(NOT MSGDK_FIND_QUIETLY)
            message(FATAL_ERROR "Unable to locate version ${MSGDK_FIND_VERSION} of ${fancy_name}. Located ${num_versions} other version(s): ${found_version_paths}")
        endif()

        set(MSGDK_FOUND FALSE)
        return()
    endif()

    if(NOT MSGDK_FIND_QUIETLY)
        message(STATUS "Using specified version of ${fancy_name}: ${latest_version}")
    endif()

    set(use_version ${MSGDK_FIND_VERSION})
    list(GET found_version_paths ${specific_version_index} base_path)
else()
    # use latest version
    set(latest_version -1)
    set(latest_version_index -1)
    set(i 0)
    foreach(v ${found_versions})
        if(${v} GREATER_EQUAL ${latest_version})
            if(${v} EQUAL ${latest_version} AND NOT MSGDK_FIND_QUIETLY)
                message(WARNING "Duplicate ${fancy_name} versions found (${v}).")
            else()
                set(latest_version ${v})
                set(latest_version_index ${i})
            endif()
        endif()
        math(EXPR i "${i} + 1")
    endforeach()

    if(NOT MSGDK_FIND_QUIETLY)
        message(STATUS "Using latest version of ${fancy_name}: ${latest_version}")
    endif()
    
    set(use_version ${latest_version})
    list(GET found_version_paths ${latest_version_index} base_path)
endif()

# find .lib files
file(GLOB lib_subdirs LIST_DIRECTORIES TRUE "${base_path}/GRDK/GameKit/Lib/*")
list(LENGTH lib_subdirs lib_subdirs_length)
if(NOT ${lib_subdirs_length} EQUAL 1)
    message(AUTHOR_WARNING "Multiple lib subdirectories found, please investigate.")
endif()
list(GET lib_subdirs 0 lib_base_path)
if((NOT IS_DIRECTORY "${lib_base_path}") AND (NOT MSGDK_FIND_QUIETLY))
    message(FATAL_ERROR "Path \"${lib_base_path}\" either does not exist or is not a directory.")
endif()

# create main target
add_library(MSGDK::MSGDK UNKNOWN IMPORTED)
set_target_properties(MSGDK::MSGDK PROPERTIES IMPORTED_LOCATION "${lib_base_path}/xgameruntime.lib")
target_include_directories(MSGDK::MSGDK INTERFACE "${base_path}/GRDK/GameKit/Include")

# create GameInput target
add_library(MSGDK::GameInput UNKNOWN IMPORTED)
add_dependencies(MSGDK::GameInput MSGDK::MSGDK)
set_target_properties(MSGDK::GameInput PROPERTIES IMPORTED_LOCATION "${lib_base_path}/GameInput.lib")
target_link_libraries(MSGDK::GameInput INTERFACE MSGDK::MSGDK)

set(MSGDK_VERSION ${use_version})
set(MSDGK_ROOT_DIR "${base_path}/../")
cmake_path(ABSOLUTE_PATH MSDGK_ROOT_DIR NORMALIZE)
set(MSGDK_FOUND TRUE)
