# THIS FILE IS UNLINCENSED
#
# This is free and unencumbered software released into the public domain.
# 
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
# 
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
# 
# For more information, please refer to <http://unlicense.org/>

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

# normalize paths
set(search_paths_normalized "")
foreach(path ${search_paths})
    cmake_path(ABSOLUTE_PATH path NORMALIZE)
    list(APPEND search_paths_normalized ${path})
endforeach()

# deduplicate search paths
list(REMOVE_DUPLICATES search_paths_normalized)
set(search_paths ${search_paths_normalized})

# workaround for CMake's limited regex capabilities
string(REPEAT [0-9] 6 any_six_digits)
set(version_regex "^${any_six_digits}$")

foreach(path ${search_paths})
    # path must be directory
    if(NOT (IS_DIRECTORY "${path}"))
        continue()
    endif()

    # path must exist
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

unset(gklib_entries)
unset(gklib_subdirs)
unset(lib_base_path)

# find .lib files
file(GLOB gklib_entries LIST_DIRECTORIES TRUE "${base_path}/GRDK/GameKit/Lib/*")
foreach(entry ${gklib_entries})
    if(IS_DIRECTORY "${entry}")
        list(APPEND gklib_subdirs "${entry}")
    endif()
endforeach()
if(NOT DEFINED gklib_subdirs)
    SearchFail("No subdirectories were found at \"${base_path}/GRDK/GameKit/Lib/\", when exactly one was expected. Is ${fancy_gdk_name} installed correctly?")
endif()
list(LENGTH gklib_subdirs gklib_subdirs_length)
if(${gklib_subdirs_length} GREATER 1)
    SearchFail("Multiple subdirectories were found at \"${base_path}/GRDK/GameKit/Lib/\", when exactly one was expected. Is ${fancy_gdk_name} installed correctly?")
    return()
endif()
list(GET gklib_subdirs 0 lib_base_path)

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
