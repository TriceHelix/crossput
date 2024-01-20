/*=================================================================================
*       +----------+
*       | crossput |
*       +----------+
* Copyright 2024 Trice Helix
* 
* This file is part of crossput and is distributed under the BSD-3-Clause License.
* Please refer to LICENSE.txt for additional information.
* =================================================================================
*/

#include "shared_api.hpp"


// this is set to false once user "listens" to events,
// preventing the console from being flooded during setup
bool block_events = true;


void WaitForEvents()
{
    block_events = false;

    while (true)
    {
        std::this_thread::sleep_for(10ms);

        // crossput is designed to internally "freeze" data between Update() calls,
        // increasing performance and reliability in cases where a lot of different
        // devices and/or inputs are queried within a short period. However, this means
        // that events that happen between Update() calls are queued and processed
        // all at once. The calling thread is then used to synchronously execute
        // affected event handlers in a non-reliable order. The chronological order
        // of events affecting a single button, key, thumbstick, etc. is still preserved.
        // As a consequence, event handlers should not measure time intervals between
        // events, as the time spans are unreliable and mostly correlate with Update()
        // invocations. The benefit of this design is that mouse movement + scrolling,
        // rapid key/button presses, and analog value changes are reported with as
        // much accuracy as the underlying implementation (hardware, input driver, system)
        // provides. For example, IMouse::GetDelta() only reports a single cumulative
        // value between the last two updates, while mouse movement events may occurr
        // as often as 1000+ times per second in reality. Hence the resolution
        // of the mouse "path" can be much higher when using the event-based API.
        crossput::UpdateAllDevices();
        crossput::DiscoverDevices();
    }
}


// Handler for device status changes (connecting, disconnecting, etc.).
void DeviceStatusChanged(const crossput::IDevice *const p_device, const crossput::DeviceStatusChange status)
{
    if (block_events) { return; }

    std::string message;
    switch (status)
    {
    case crossput::DeviceStatusChange::DISCOVERED:
        message = std::format("Device of type {} discovered. (ID {})", pretty_devtype_names[static_cast<int>(p_device->GetType())], p_device->GetID());
        break;

    case crossput::DeviceStatusChange::CONNECTED:
        message = std::format("Device ({}) connected.", p_device->GetID());
        break;

    case crossput::DeviceStatusChange::DISCONNECTED:
        message = std::format("Device ({}) disconnected.", p_device->GetID());
        break;

    case crossput::DeviceStatusChange::DESTROYED:
        message = std::format("Device ({}) is about to be destroyed.", p_device->GetID());
        break;
    }

    std::cout << message << std::endl;
}


// Handler for mouse movement.
void MouseMoved(const crossput::IMouse *const p_mouse, const int64_t x, const int64_t y, const int64_t dx, const int64_t dy)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Mouse ({}) moved by ({}|{}). Total: ({}|{}).",
        p_mouse->GetID(), dx, dy, x, y
        ) << std::endl;
}


// Handler for mouse scrolling.
void MouseScrolled(const crossput::IMouse *p_mouse, int64_t x, int64_t y, int64_t dx, int64_t dy)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Mouse ({}) scrolled by ({}|{}). Total: ({}|{}).",
        p_mouse->GetID(), dx, dy, x, y
        ) << std::endl;
}


// Handler for mouse button changes.
void MouseButtonChanged(const crossput::IMouse *p_mouse, uint32_t index, float value, bool state)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Mouse ({}) button {} has value {} and state {}.",
        p_mouse->GetID(), index, value, state
        ) << std::endl;
}


// Handler for keyboard key changes.
void KeyboardKeyChanged(const crossput::IKeyboard *p_keyboard, crossput::Key key, float value, bool state)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Keyboard ({}) key {} has value {} and state {}. Total keys pressed: {}",
        p_keyboard->GetID(), static_cast<int>(key), value, state, p_keyboard->GetNumKeysPressed()
        ) << std::endl;
}


// Handler for gamepad button changes.
void GamepadButtonChanged(const crossput::IGamepad *p_gamepad, crossput::Button button, float value, bool state)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Gamepad ({}) button {} has value {} and state {}.",
        p_gamepad->GetID(), static_cast<int>(button), value, state
        ) << std::endl;
}


void GamepadThumbstickChanged(const crossput::IGamepad *p_gamepad, uint32_t index, float x, float y)
{
    if (block_events) { return; }

    std::cout << std::format(
        "Gamepad ({}) thumbstick {} has value ({}|{}).",
        p_gamepad->GetID(), index, x, y
        ) << std::endl;
}


void SubscribeToGlobalEvents()
{
    // These functions register a function (or invocable object) to a kind of event.
    // Whenever any device causes an event of that kind, all globally registered
    // callbacks are invoked. Most of these functions also enable the caller to
    // specify a "filter" value. That way only events that affect a specific
    // key, button, thumbstick, etc. cause the callback to be invoked.
    crossput::RegisterGlobalStatusCallback(&DeviceStatusChanged);
    crossput::RegisterGlobalMouseMoveCallback(&MouseMoved);
    crossput::RegisterGlobalMouseScrollCallback(&MouseScrolled);
    crossput::RegisterGlobalMouseButtonCallback(&MouseButtonChanged);
    crossput::RegisterGlobalKeyboardKeyCallback(&KeyboardKeyChanged);
    crossput::RegisterGlobalGamepadButtonCallback(&GamepadButtonChanged);
    crossput::RegisterGlobalGamepadThumbstickCallback(&GamepadThumbstickChanged);

    std::cout << "Subscribed to all events." << std::endl;
}


void SubscribeToDeviceEvents(crossput::IDevice *const p_device)
{
    p_device->RegisterStatusCallback(&DeviceStatusChanged);

    switch (p_device->GetType())
    {
    case crossput::DeviceType::MOUSE:
        {
            crossput::IMouse *const p_mouse = dynamic_cast<crossput::IMouse *>(p_device);
            p_mouse->RegisterMoveCallback(&MouseMoved);
            p_mouse->RegisterScrollCallback(&MouseScrolled);
            p_mouse->RegisterButtonCallback(&MouseButtonChanged);
        }
        
        break;

    case crossput::DeviceType::KEYBOARD:
        {
            crossput::IKeyboard *const p_keyboard = dynamic_cast<crossput::IKeyboard *>(p_device);
            p_keyboard->RegisterKeyCallback(&KeyboardKeyChanged);
        }
        break;

    case crossput::DeviceType::GAMEPAD:
        {
            crossput::IGamepad *const p_gamepad = dynamic_cast<crossput::IGamepad *>(p_device);
            p_gamepad->RegisterButtonCallback(&GamepadButtonChanged);
            p_gamepad->RegisterThumbstickCallback(&GamepadThumbstickChanged);
        }
        break;
    }

    std::cout << std::format("Subscribed to all events of Device {}.", p_device->GetID()) << std::endl;
}


int main()
{
    std::cout << "Welcome to the crossput event API demo.\n---" << std::endl;

    while (true)
    {
        options =
        {
            "< EXIT",
            "Subscribe to all Events",
            "Subscribe to all Events of a specific Device",
            "Listen (after subscribing to events)"
        };

        const unsigned int opt = DisplayOptions(
            std::cout, std::cin,
            "Select an action to perform.",
            options
        );

        switch (opt)
        {
        case 0: return 0;
        case 1: SubscribeToGlobalEvents(); break;
        case 3: WaitForEvents(); // this is an infinite loop
        
        case 2:
            // Ask user to select an input device.
            // The implementation can be found in "shared_api.hpp".
            crossput::IDevice *const p_device = UserDeviceSelection("< BACK");
            if (p_device != nullptr) { SubscribeToDeviceEvents(p_device); }
            break;
        }

        std::cout << std::endl;
    }

    // Release underlying resources of all devices.
    // In a regular application, individual devices would be destroyed
    // when they are no longer in use (pointers and references are invalidated).
    // This is especially useful on platforms with very limited memory.
    // It is also good practice to clean up everything before the process exits.
    crossput::DestroyAllDevices();

    // Unsubscribe from all events.
    // Single callbacks can also be unregistered via UnregisterCallback() using
    // the ID value returned from any callback registry function/method.
    // Device-specific callbacks were already unregistered during DestroyAllDevices().
    // But again, it is good practice to clean up before the process exits.
    crossput::UnregisterAllCallbacks();

    return 0;
}
