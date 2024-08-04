# crossput

---

## NOTICE OF DEPRECATION

This repository is no longer used for development. Instead, the project has been internally merged into [Empty Engine](https://github.com/EmptyEngine).
Critical fixes may still be applied to the standalone version, however this project will not receive any sort of 1.0 release on its own.
To those discovering crossput in the future, I sincerely hope that this little codebase is of use to you in some form.
Thank you for your interest.

---

## Cross-Platform Input from Mice, Keyboards, and Gamepads in C++

Supports Windows PC, Xbox, and all major Linux distributions.

---

### Features

- Device-centric API for interacting with mice, keyboards, and gamepads (controllers/joysticks)
- Fast, lightweight, suitable for real-time applications (e.g. video games)
- No runtime dependencies (besides operating system)
- Compatible with C++11 and newer (C++20 only required during compilation)

### Optional Features

- Event-centric API which enables subscription to certain types of input or sources of input
- Rumble and Force-Feedback support for capable hardware of any type
- Aggregation API for treating a group of devices as a single entity

Any of the optional features can be disabled at compile time to reduce binary size or improve overall performance.
Whether a feature is available to the user is indicated via preprocessor defines (`CROSSPUT_FEATURE_<...>`).

---

### Building

Requires CMake 3.27+ for configuration and C++20 support from your compiler of choice. Tested with MSVC 19 and GCC 13.
Simply add the repository to your project (e.g. via the ExternalProject function) and link to target `crossput`.
Must be built as a dynamic library.

#### Prerequisites

Currently, building for Windows (PC, Xbox) requires installing the [Microsoft Game Development Kit (GDK)](https://github.com/microsoft/GDK).
crossput only needs access to a single header in the GDK, however it is unfortunately not available under an open-source license (yet).
According to Microsoft, the GDK will be part of the Windows SDK in the future, so this requirement is subject to change.

#### Configuration

[CMakeLists.txt](./CMakeLists.txt) exposes the following options for customization:

- `CROSSPUT_FEATURE_CALLBACK` (default: true)
If true, enables the callback feature.
This adds an event-driven API for handling input from specific sources via a subscription model.
The feature has a negative impact on overall performance, which is why projects that do not use the feature should disable it.

- `CROSSPUT_FEATURE_FORCE` (default: true)
If true, enables the force feature.
This adds an abstraction of rumble and force-feedback to the API, simply referred to as forces by crossput.
Usually only Gamepads have force capabilities, though the API is suitable for interacting with any force-capable hardware.
The feature's overall performance impact is negligible, so keeping it enabled is recommended.

- `CROSSPUT_FEATURE_AGGREGATE` (default: true)
If true, enables the aggregate feature.
This adds functionality to interact with multiple devices via the same interface, effectively treating them as a single source of input.
Aggregates require little to zero additional logic to act as drop-in replacements for single input sources.
Features "CALLBACK" and "FORCE" are also compatible with aggregation, meaning that event-based code and force effects are fully supported.
The feature has a minor negative performance impact on some parts of the API.

- `CROSSPUT_BUILD_DEMO` (default: false)
If true, builds all available demonstration executables. Some might require certain features to be enabled.

---

### Restrictions

- No thread-safety is provided. Reading input data from multiple threads is entirely safe. However, modifying *any* device, (un-)registering callbacks, creating/destroying devices or forces, or any other operations which could introduce race conditions, are not supported in parallel. A simple solution is correctly separating device management and data processing in your application. For example, a video game might update all devices at the beginning of its "update loop" using the main thread. Later on, multiple worker threads could read data from the devices and adjust player movement, UI, etc. As long as these "stages" are strictly separated and do not overlap, no race conditions will occur.

- On Linux, the active user requires read/write permissions for `/dev/input`, which can be accomplished by adding them to the "input" group.
Often times this will already be the case (for example, Steam does it automatically), however failing to do so will lead to runtime exceptions.

- The aggregation feature is 100% finished but lacks a demo executable.

---

### Usage

Header [crossput.hpp](./include/crossput.hpp) contains all global functions and interfaces required to interact with input devices.
Everything is thoroughly documented within the header, with external documentation to be added in the future.
The [demo executables](./demo/) also showcase some (though not all) of crossput's features.
Their purpose is mainly to troubleshoot hardware and software issues in an isolated environment.
Here is a snippet of code to help you get started with crossput's core functionality:

```c++
#include "crossput.hpp"

crossput::IMouse *mouse = nullptr;
crossput::IKeyboard *keyboard = nullptr;
crossput::IGamepad *gamepad = nullptr;

// This code assigns devices to the global variables.
// In your application, it is likely a good idea to perform this setup at launch
// and re-run it in case a device disconnects, for example.
{
    // Search for input devices available to the current process.
    // The first invocation also typically initializes some udnerlying data structures and may be slower than proceeding calls.
    crossput::DiscoverDevices();

    // Input data is frozen between device updates, meaning you should update them frequently in real-time applications.
    // Because new devices found in DiscoverDevices() are initially always disconnected, it might be a good idea
    // to update all devices after discovery to determine whether or not to use them (e.g. preferring connected devices).
    // After narrowing down the amount of devices which are actuall in use, consider only invoking Update()
    // on the individual devices to improve performance.
    crossput::UpdateAllDevices();

    // Pointers to all mice, keyboards, and gamepads which have been found are copied to the vectors below.
    // Alternatively, you may also choose to interact with devices via the crossput::IDevice interface,
    // which is agnostic to the underlying type of device. Pointers to IDevice interfaces can be
    // dynamically casted to the specific types later on. To access all interfaces this way,
    // invoke crossput::GetDevices().
    // Note: All of these functions have an additional parameter which lets you specify whether or not
    // to include disconnected devices. The default behaviour is to include them in the query.
    std::vector<crossput::IMouse *> mice;
    std::vector<crossput::IKeyboard *> keyboards;
    std::vector<crossput::IGamepad *> gamepads;
    crossput::GetMice(mice);
    crossput::GetKeyboards(keyboards);
    crossput::GetGamepads(gamepads);

    // Now a device of each type is chosen.
    // Picking the first available may not always be a good idea due to platform-specific reasons,
    // which is why you might keep the previously gathered "candidates" until one of them receives input,
    // or is manually selected by the user of your application, or via different means entirely.

    if (mice.size() > 0)
    {
        // Select a mouse...
        mouse = mice[0];
    }

    if (keyboards.size() > 0)
    {
        // Select a keyboard...
        keyboard = keyboards[0];
    }

    if (gamepads.size() > 0)
    {
        // Select a gamepad...
        gamepad = gamepads[0];
    }

    // If the desired device has not been detected, or the user of your application
    // wants to choose a different input device, simply re-run this code at a later time.
    // Devices which have been discovered will keep being available until manually
    // destroyed by you, which can be achieved via crossput::DestroyDevice(<id>)
    // or crossput::DestroyAllDevices().
}

// This code would be executed regularly during the application lifetime.
// While not updating devices for extended periods of time is completely fine,
// the "quality" of input may obviously suffer in some situations.
{
    // Input data is static between Update() calls, so invoking the method once
    // before fetching and processing the desired data is sufficient.
    // Also make sure to check if a device is connected before reading any
    // data from it, as many methods on a disconnected device simply exit early,
    // leaving variables in an undefined state or returning a default value.

    if (mouse != nullptr)
    {
        mouse->Update();
        if (mouse->IsConnected())
        {
            // Mice keep track of the total movement and scroll troughout their lifetime. 
            // Note that the unit is arbitrary and not tied to pixels or similar measurements.
            int64_t pos_x, pos_x, scroll_x, scroll_x;
            mouse->GetPosition(pos_x, pos_y);
            mouse->GetScroll(scroll_x, scroll_y);

            // Sometimes the delta (difference) of position or scroll may be more useful.
            // It represents the change in value between the last two updates.
            int64_t delta_x, delta_y, scroll_delta_x, scroll_delta_y;
            mouse->GetDelta(delta_x, delta_y);
            mouse->GetScrollDelta(scroll_delta_x, scroll_delta_y);

            // Number of buttons on the mouse, which indicates the maximum index allowed
            // as an argument to GetButtonState(...) or GetButtonValue(...).
            // The left, right, and middle mouse buttons use indices 0, 1, and 2 respectively.
            // Additional buttons are addressed via index 3 and greater.
            uint32_t num_buttons = mouse->GetButtonCount();

            // The digital state of a mouse button is read like this.
            // Analog buttons require a threshold to be set, which is used to determine whether they are pressed or not.
            // To set the threshold of a button, use SetButtonThreshold(...) or SetGlobalThreshold(...) to target all buttons.
            // This particular overload also provides the amount of time (seconds) since the last state change.
            float lmb_time;
            bool lmb_pressed = mouse->GetButtonState(0, lmb_time);

            // Despite mice rarely supporting analog buttons, crossput provides simulated analog values
            // for hardware of any kind. If the specified button is digital, this method simply returns
            // 1.0 or 0.0 based on the state.
            float lmb_value = mouse->GetButtonValue(0);
        }
    }

    if (keyboard != nullptr)
    {
        keyboard->Update();
        if (keyboard->IsConnected())
        {
            // Self-explanatory.
            // May have an upper limit for reasons detailed in the method's documentation.
            uint32_t keys_pressed = keyboard->GetNumKeysPressed();

            // Keyboard keys work similarly to mouse buttons,
            // though they are addressed via enum values instead of indices.
            float esc_time;
            bool esc_state = keyboard->GetKeyState(crossput::Key::ESC, esc_time);
            float esc_value = keyboard->GetKeyValue(crossput::Key::ESC);
        }
    }

    if (gamepad != nullptr)
    {
        gamepad->Update();
        if (gamepad->IsConnected())
        {
            // Self-explanatory.
            uint32_t num_thumbsticks = gamepad->GetThumbstickCount();

            // Gamepad buttons (including triggers) work similarly to mouse buttons and keyboard keys,
            // except that they are addressed via values of the Button enum.
            // Note that in the current version of crossput, the L2 and R2 triggers
            // are the most likely to have any sort of native analog value support.
            // This is purely due to native API/driver restrictions and may change in the future.
            float r2_time;
            bool r2_state = gamepad->GetButtonState(crossput::Button::R2, r2_time);
            float r2_value = gamepad->GetButtonValue(crossput::Button::R2);

            // Thumbsticks, unlike buttons, have a value range between -1.0 and +1.0 on both of their axes.
            // They are indexed from left to right, where possible.
            float thumbstick_x, thumbstick_y;
            gamepad->GetThumbstick(0, thumbstick_x, thumbstick_x);
        }
    }
}

```
