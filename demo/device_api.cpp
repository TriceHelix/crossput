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


using namespace std::chrono_literals;


const std::string direction_names[16] =
{
    "Neutral",      // 0000
    "Up",           // 0001
    "Down",         // 0010
    "Neutral",      // 0011
    "Left",         // 0100
    "Up-Left",      // 0101
    "Down-Left",    // 0110
    "Left",         // 0111
    "Right",        // 1000
    "Up-Right",     // 1001
    "Down-Right",   // 1010
    "Right",        // 1011
    "Neutral",      // 1100
    "Up",           // 1101
    "Down",         // 1110
    "Neutral"       // 1111
};


void TestMouse(crossput::IMouse *const p_mouse)
{
    // Print information about the mouse.
    std::cout << std::format(
        "- Mouse Information -\n"
        "ID: {}\n"
        "Display Name: {}\n"
        "Number of buttons: {}",
        p_mouse->GetID(),
        p_mouse->GetDisplayName(),
        p_mouse->GetButtonCount())
        << std::endl;

    std::cout << "\n"
        "A message is displayed for every 250 units of mouse movement.\n"
        "Button presses and scrolling will also display messages.\n"
        "To exit the test, hold the right mouse button for at least 3 seconds."
        << std::endl;

    // This sets a minimum analog threshold in range [0.0;1.0] for any button to be considered "pressed".
    // Most devices (and systems) do not support analog buttons, however crossput offers a uniform
    // API for both digital and analog buttons. When an analog value is not available,
    // either 0.0 or 1.0 will be reported depending on the digital state. When a digital state is not
    // available, the analog value is compared against the specified threshold.
    // Note that thresholds can be set globally (for all buttons on a device), or for individual buttons.
    // The default threshold value is always 0.0.
    p_mouse->SetGlobalThreshold(0.5F);

    uint64_t total_x = 0, total_y = 0, steps = 0;
    bool prev_lmb = false, prev_rmb = false, prev_mmb = false;

    while (true)
    {
        std::this_thread::sleep_for(10ms);

        // Update this specific device.
        // IsConnected() will return a constant value between Update() calls,
        // so we update first before checking connectivity.
        p_mouse->Update();

        // Ensure the mouse is connected.
        if (!p_mouse->IsConnected())
        {
            std::cout << "The mouse has disconnected!" << std::endl;
            break;
        }

        // Record absolute mouse movement.
        // GetDelta() retrieves the positional difference between
        // the last two Update() calls.
        int64_t dx, dy;
        p_mouse->GetDelta(dx, dy);
        total_x += std::abs(dx);
        total_y += std::abs(dy);
        const uint64_t st = (total_x + total_y) / 250;
        if (st != steps)
        {
            // Get the internally accumulated mouse position.
            // This position is not necessarily tied to the application lifetime.
            int64_t x, y;
            p_mouse->GetPosition(x, y);
            std::cout << std::format("The mouse has moved 250 units. Current position: ({}|{})", x, y) << std::endl;
        }
        steps = st;

        // Check the scroll delta.
        // Scroll is also internally accumulated like the mouse position,
        // however most use cases for scroll wheel data only care about changes.
        // Up and down (vertical) scrolling is reported as the "y" value.
        // Some scroll wheels can also be scrolled/tilted horizontally,
        // which is reported as the "x" value.
        int64_t sdx, sdy;
        p_mouse->GetScrollDelta(sdx, sdy);
        if (sdx != 0 || sdy != 0)
        {
            std::cout << std::format("Scroll delta: ({}|{})", sdx, sdy) << std::endl;
        }

        // Check for button presses.
        // This overload of GetButtonState() also reports the amount of time since the last state change, in seconds.
        // Initially, the time is reported as infinity (bacause no comparison state exists).
        float lmb_time, rmb_time, mmb_time;
        const bool lmb = p_mouse->GetButtonState(0, lmb_time); // left
        const bool rmb = p_mouse->GetButtonState(1, rmb_time); // right
        const bool mmb = p_mouse->GetButtonState(2, mmb_time); // middle
        if (lmb & (lmb != prev_lmb)) { std::cout << "Left Mouse Button pressed." << std::endl; }
        if (rmb & (rmb != prev_rmb)) { std::cout << "Right Mouse Button pressed." << std::endl; }
        if (mmb & (mmb != prev_mmb)) { std::cout << "Middle Mouse Button pressed." << std::endl; }
        prev_lmb = lmb;
        prev_rmb = rmb;
        prev_mmb = mmb;

        if (rmb && rmb_time >= 3.0F)
        {
            break;
        }
    }

    std::cout << "Exiting mouse test..." << std::endl;
}


void TestKeyboard(crossput::IKeyboard *const p_keyboard)
{
    // Print information about the keyboard.
    std::cout << std::format(
        "- Keyboard Information -\n"
        "ID: {}\n"
        "Display Name: {}",
        p_keyboard->GetID(),
        p_keyboard->GetDisplayName())
        << std::endl;

    std::cout << "\n"
        "A message will be displayed when the WASD or arrow keys are pressed.\n"
        "To exit the test, press the Escape key."
        << std::endl;

    // This sets a minimum analog threshold in range [0.0;1.0] for any key to be considered "pressed".
    // Most devices (and systems) do not support analog keys, however crossput offers a uniform
    // API for both digital and analog keys. When an analog value is not available,
    // either 0.0 or 1.0 will be reported depending on the digital state. When a digital state is not
    // available, the analog value is compared against the specified threshold.
    // Note that thresholds can be set globally (for all keys on a device), or for individual keys.
    // The default threshold value is always 0.0.
    p_keyboard->SetGlobalThreshold(0.5F);

    unsigned int prev_direction = 0;

    while (true)
    {
        std::this_thread::sleep_for(10ms);

        // Update this specific device.
        // IsConnected() will return a constant value between Update() calls,
        // so we update first before checking connectivity.
        p_keyboard->Update();

        // Ensure the keyboard is connected.
        if (!p_keyboard->IsConnected())
        {
            std::cout << "The keyboard has disconnected!" << std::endl;
            break;
        }

        // Check for key presses.
        const bool up = p_keyboard->GetKeyState(crossput::Key::W) || p_keyboard->GetKeyState(crossput::Key::UP);
        const bool down = p_keyboard->GetKeyState(crossput::Key::S) || p_keyboard->GetKeyState(crossput::Key::DOWN);
        const bool left = p_keyboard->GetKeyState(crossput::Key::A) || p_keyboard->GetKeyState(crossput::Key::LEFT);
        const bool right = p_keyboard->GetKeyState(crossput::Key::D) || p_keyboard->GetKeyState(crossput::Key::RIGHT);

        // Combine the directional states into a single index,
        // which maps to a string name for that combination.
        const int direction =
              static_cast<unsigned int>(up)
            | (static_cast<unsigned int>(down) << 1)
            | (static_cast<unsigned int>(left) << 2)
            | (static_cast<unsigned int>(right) << 3);

        if (direction != prev_direction)
        {
            std::cout << direction_names[direction] << std::endl;
            prev_direction = direction;
        }

        // End testing if Escape is pressed.
        if (p_keyboard->GetKeyState(crossput::Key::ESC))
        {
            break;
        }
    }
}


void TestGamepad(crossput::IGamepad *const p_gamepad)
{
    // Print information about the gamepad.
    const uint32_t num_thumbsticks = p_gamepad->GetThumbstickCount();
    std::cout << std::format(
        "- Gamepad Information -\n"
        "ID: {}\n"
        "Display Name: {}\n"
        "Thumbstick count: {}",
        p_gamepad->GetID(),
        p_gamepad->GetDisplayName(),
        num_thumbsticks)
        << std::endl;

    std::cout << "\n"
        "A message is displayed when A, B, X, Y, or any of the Dpad buttons are pressed.\n"
        "Press SELECT (left menu button) to print the values of all thumbsticks.\n"
        "To exit the test, press START (right menu button)."
        << std::endl;

    // This sets a minimum analog threshold in range [0.0;1.0] for any button/trigger to be considered "pressed".
    // Most devices (and systems) do not support analog buttons/triggers, however crossput offers a uniform
    // API for both digital and analog buttons/triggers. When an analog value is not available,
    // either 0.0 or 1.0 will be reported depending on the digital state. When a digital state is not
    // available, the analog value is compared against the specified threshold.
    // Note that thresholds can be set globally (for all buttons/triggers on a device), or for individual buttons/triggers.
    // The default threshold value is always 0.0.
    p_gamepad->SetGlobalThreshold(0.5F);

    unsigned int prev_direction = 0;
    bool select_prev = false;
    
    while (true)
    {
        std::this_thread::sleep_for(10ms);

        // Update this specific device.
        // IsConnected() will return a constant value between Update() calls,
        // so we update first before checking connectivity.
        p_gamepad->Update();

        // Ensure the gamepad is connected.
        if (!p_gamepad->IsConnected())
        {
            std::cout << "The gamepad has disconnected!" << std::endl;
            break;
        }

        // Check for button presses.
        const bool up = p_gamepad->GetButtonState(crossput::Button::NORTH) || p_gamepad->GetButtonState(crossput::Button::DPAD_UP);
        const bool down = p_gamepad->GetButtonState(crossput::Button::SOUTH) || p_gamepad->GetButtonState(crossput::Button::DPAD_DOWN);
        const bool left = p_gamepad->GetButtonState(crossput::Button::WEST) || p_gamepad->GetButtonState(crossput::Button::DPAD_LEFT);
        const bool right = p_gamepad->GetButtonState(crossput::Button::EAST) || p_gamepad->GetButtonState(crossput::Button::DPAD_RIGHT);

        // Combine the directional states into a single index,
        // which maps to a string name for that combination.
        const int direction =
              static_cast<unsigned int>(up)
            | (static_cast<unsigned int>(down) << 1)
            | (static_cast<unsigned int>(left) << 2)
            | (static_cast<unsigned int>(right) << 3);

        if (direction != prev_direction)
        {
            std::cout << direction_names[direction] << std::endl;
            prev_direction = direction;
        }

        const bool select = p_gamepad->GetButtonState(crossput::Button::SELECT);
        if (select & select != select_prev)
        {
            // Read the thumbsticks.
            // Note that the vector resulting from the x and y values is not
            // clamped/normalized in any way. While the maximum vector length
            // of a typical thumbstick with circular bounds is ~1.0,
            // the reported values may be larger due to measuring inaccuracies.
            // Emulators or hardware with unusual physical bounds
            // can cause the vector length to exceed 1.0 as well.
            std::ostringstream thumbstick_data;
            for (uint32_t i = 0; i < num_thumbsticks; i++)
            {
                float x, y;
                p_gamepad->GetThumbstick(i, x, y);
                thumbstick_data << std::format("[Thumbstick {}] ({}|{})\n", i + 1, x, y);
            }
            std::cout << thumbstick_data.str();
        }
        select_prev = select;

        // End testing if START is pressed.
        if (p_gamepad->GetButtonState(crossput::Button::START))
        {
            break;
        }
    }
}


int main()
{
    std::cout << "Welcome to the crossput device API demo.\n---" << std::endl;

    // Ask user to select an input device.
    // The implementation can be found in "shared_api.hpp".
    crossput::IDevice *p_device;
    while ((p_device = UserDeviceSelection("< EXIT")) != nullptr)
    {
        if (!p_device->IsConnected())
        {
            std::cout << "Please select a connected device.\n" << std::endl;
            continue;
        }

        switch (p_device->GetType())
        {
        // Dynamically casting an IDevice pointer to a more specific interface
        // is always safe if IDevice::GetType() returns the corresponding type.
        case crossput::DeviceType::MOUSE: TestMouse(dynamic_cast<crossput::IMouse *>(p_device)); break;
        case crossput::DeviceType::KEYBOARD: TestKeyboard(dynamic_cast<crossput::IKeyboard *>(p_device)); break;
        case crossput::DeviceType::GAMEPAD: TestGamepad(dynamic_cast<crossput::IGamepad *>(p_device)); break;
        }
        std::cout << std::endl;
    }

    // Release underlying resources of all devices.
    // In a regular application, individual devices would be destroyed
    // when they are no longer in use (pointers and references are invalidated).
    // This is especially useful on platforms with very limited memory.
    // It is also good practice to clean up everything before the process exits.
    crossput::DestroyAllDevices();

    return 0;
}
