# ================================================================================= #
#                                   +----------+                                    #
#                                   | crossput |                                    #
#                                   +----------+                                    #
#                            Copyright 2024 Trice Helix                             #
#                                                                                   #
# This file is part of crossput and is distributed under the BSD-3-Clause License.  #
# Please refer to LICENSE.txt for additional information.                           #
# ================================================================================= #

cmake_minimum_required(VERSION 3.27)

project(crossput
    LANGUAGES CXX
    VERSION 0.1.1
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# options
option(
    CROSSPUT_FEATURE_CALLBACK
    "If true, enables the callback feature.\
    This adds an event-driven API for handling input from specific sources via a subscription model.\
    The feature has a negative impact on overall performance, which is why projects that do not use the feature should disable it."
    TRUE
)

option(
    CROSSPUT_FEATURE_FORCE
    "If true, enables the force feature.\
    This adds an abstraction of rumble and force-feedback to the API, simply referred to as forces by crossput.\
    Usually only Gamepads have force capabilities, though the API is suitable for interacting with any force-capable hardware.\
    The feature's overall performance impact is negligible, so keeping it enabled is recommended."
    TRUE
)

option(
    CROSSPUT_FEATURE_AGGREGATE
    "If true, enables the aggregate feature.\
    This adds functionality to interact with multiple devices via the same interface, effectively treating them as a single source of input.\
    Aggregates require little to zero additional logic to act as drop-in replacements for single input sources.\
    Features \"CALLBACK\" and \"FORCE\" are also compatible with aggregation, meaning that event-based code and force effects are fully supported.\
    The feature has a minor negative performance impact on some parts of the API."
    TRUE
)

option(
    CROSSPUT_BUILD_DEMO
    "If true, builds all available demonstration executables. Some might require certain features to be enabled."
    FALSE
)

if(${MSVC})
    # fix MSVC "feature" that incorrectly reports cpp-standard
    add_compile_options(/Zc:__cplusplus)
endif()

# crossput main library
add_library(crossput SHARED "")
target_include_directories(crossput PUBLIC "include")
target_sources(crossput PRIVATE "src/impl.cpp")

set_target_properties(
    crossput
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
    COMPILE_WARNING_AS_ERROR TRUE
)

# enable/disable features
if(${CROSSPUT_FEATURE_CALLBACK})
    target_compile_definitions(crossput PUBLIC "CROSSPUT_FEATURE_CALLBACK")
endif()

if(${CROSSPUT_FEATURE_FORCE})
    target_compile_definitions(crossput PUBLIC "CROSSPUT_FEATURE_FORCE")
endif()

if(${CROSSPUT_FEATURE_AGGREGATE})
    target_compile_definitions(crossput PUBLIC "CROSSPUT_FEATURE_AGGREGATE")
endif()

# platform-dependent details
if(${WIN32})
    target_sources(crossput PRIVATE "src/impl_windows.cpp")

    # export all symbols
    set_target_properties(crossput PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

    # link to GDK
    find_package(MSGDK REQUIRED)
    add_dependencies(crossput MSGDK::GameInput)
    target_link_libraries(crossput PUBLIC MSGDK::GameInput)
    target_compile_definitions(crossput PRIVATE "NOMINMAX")
elseif(${LINUX})
    target_sources(crossput PRIVATE "src/impl_linux.cpp")
else()
    message(FATAL_ERROR "Unsupported target platform.")
endif()

# compiler-specific details
if(${MSVC})
    # enable most warnings
    target_compile_options(crossput PRIVATE /W4)

    # allow inheritance via dominance
    target_compile_options(crossput PRIVATE /wd4250)
else()
    # enable most warnings
    target_compile_options(crossput PRIVATE -Wall -Wextra -Wpedantic)

    # allow missing initializer values
    target_compile_options(crossput PRIVATE -Wno-missing-field-initializers)
endif()

function(add_demo_executable arg_name arg_source)
    add_executable(${arg_name} "${arg_source}")
    set_target_properties(
        ${arg_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED TRUE
    )
    add_dependencies(${arg_name} crossput)
    target_link_libraries(${arg_name} PRIVATE crossput)
endfunction()

# demonstration executables
if(${CROSSPUT_BUILD_DEMO})
    # standard device-centric API
    add_demo_executable(device-demo "demo/device_api.cpp")

    if(${CROSSPUT_FEATURE_CALLBACK})
        # optional event-centric API
        add_demo_executable(event-demo "demo/event_api.cpp")
    endif()

    if(${CROSSPUT_FEATURE_FORCE})
        # optional force API
        add_demo_executable(force-demo "demo/force_api.cpp")
    endif()
endif()
