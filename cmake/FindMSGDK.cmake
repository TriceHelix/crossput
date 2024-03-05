# FILE PURPOSE: Provide a CMake "find module" to locate parts of the Microsoft Game Development Kit (MSGDK) installed on the system.
# This module is NOT official and is NOT affiliated with Microsoft.
# It should be considered as part of the public domain, regardless of any licenses applying to other contents this file is shipped along.
# Hence, no warranties or similar concepts apply to the functionality provided by the module.

set(fancy_gdk_name "MSGDK (Microsoft Game Development Kit)")

macro(TryMessage arg_severity arg_message)
    if(NOT (${MSGDK_FIND_QUIETLY}))
        message(${arg_severity} "${arg_message}")
    endif()
endmacro()

macro(SearchFail arg_message)
    if(${MSGDK_FIND_REQUIRED})
        message(FATAL_ERROR "${arg_message}")
    elseif(NOT (${MSGDK_FIND_QUIETLY}))
        message(STATUS "${arg_message}")
    endif()

    set(MSGDK_FOUND FALSE)
endmacro()

if(NOT (${WIN32}))
    SearchFail("${fancy_gdk_name} is not supported on platforms that are not Windows or Windows-like.")
    return()
endif()

set(search_paths
    "C:/Program Files/Microsoft GDK/"
    "C:/Program Files (x86)/Microsoft GDK/"
)

if(DEFINED ENV{GameDK})
    list(APPEND search_paths "$ENV{GameDK}")
endif()

if(DEFINED MSDGK_ROOT_DIR)
    list(APPEND search_paths "${MSDGK_ROOT_DIR}/")
endif()

list(REMOVE_DUPLICATES search_paths)

# workaround for CMake's limited regex capabilities
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
    SearchFail("Unable to locate ${fancy_gdk_name}. Searched directories: ${search_paths}")
    return()
endif()

set(use_version 0)
set(base_path "")

if(DEFINED MSGDK_FIND_VERSION)
    if(NOT ${MSGDK_FIND_VERSION} MATCHES ${version_regex})
        SearchFail("Requested ${fancy_gdk_name} version \"${MSGDK_FIND_VERSION}\" does not match the conventional format of six numerical digits.")
        return()
    endif()

    # use specific version
    list(FIND found_versions ${MSGDK_FIND_VERSION} specific_version_index)
    if(${specific_version_index} LESS 0)
        # -> specific version not found
        SearchFail("Unable to locate version ${MSGDK_FIND_VERSION} of ${fancy_gdk_name}. Located ${num_versions} other version(s): ${found_version_paths}")
        return()
    endif()

    TryMessage(STATUS "Using specified version of ${fancy_gdk_name}: ${latest_version}")

    set(use_version ${MSGDK_FIND_VERSION})
    list(GET found_version_paths ${specific_version_index} base_path)
else()
    # use latest version
    set(latest_version -1)
    set(latest_version_index -1)
    set(i 0)
    foreach(v ${found_versions})
        if(${v} GREATER_EQUAL ${latest_version})
            if(${v} EQUAL ${latest_version})
                TryMessage(WARNING "Duplicate ${fancy_gdk_name} versions found (${v}).")
            else()
                set(latest_version ${v})
                set(latest_version_index ${i})
            endif()
        endif()
        math(EXPR i "${i} + 1")
    endforeach()

    TryMessage(STATUS "Using latest version of ${fancy_gdk_name}: ${latest_version}")
    
    set(use_version ${latest_version})
    list(GET found_version_paths ${latest_version_index} base_path)
endif()

# find .lib files
file(GLOB lib_subdirs LIST_DIRECTORIES TRUE "${base_path}/GRDK/GameKit/Lib/*")
list(LENGTH lib_subdirs lib_subdirs_length)
if(NOT ${lib_subdirs_length} EQUAL 1)
    SearchFail("Either zero or multiple subdirectories were found at \"${base_path}/GRDK/GameKit/Lib/\", when a single one was expected. Is the MSGDK installed correctly?")
    return()
endif()

list(GET lib_subdirs 0 lib_base_path)
if(NOT IS_DIRECTORY "${lib_base_path}")
    SearchFail("Path \"${lib_base_path}\" either does not exist or is not a directory.")
    return()
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
