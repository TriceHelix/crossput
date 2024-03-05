/*================================================================================= *
*                                   +----------+                                    *
*                                   | crossput |                                    *
*                                   +----------+                                    *
*                            Copyright 2024 Trice Helix                             *
*                                                                                   *
* This file is part of crossput and is distributed under the BSD-3-Clause License.  *
* Please refer to LICENSE.txt for additional information.                           *
* =================================================================================*/

#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <thread>
#include "crossput.hpp"


using namespace std::chrono_literals;


// cache
std::vector<crossput::IDevice *> devices;
std::vector<std::string> options;


const std::string pretty_devtype_names[] =
{
    "Unknown",      // DeviceType::UNKNOWN
    "Mouse",        // DeviceType::MOUSE
    "Keyboard",     // DeviceType::KEYBOARD
    "Gamepad"       // DeviceType::GAMEPAD
};


const std::string pretty_ftype_names[] =
{
    "Rumble",           // ForceType::RUMBLE
    "Constant",         // ForceType::CONSTANT
    "Ramp",             // ForceType::RAMP
    "Sine",             // ForceType::SINE
    "Triangle",         // ForceType::TRIANGLE
    "Square",           // ForceType::SQUARE
    "Sawtooth Up",      // ForceType::SAW_UP
    "Sawtooth Down",    // ForceType::SAW_DOWN
    "Spring",           // ForceType::SPRING
    "Friction",         // ForceType::FRICTION
    "Damper",           // ForceType::DAMPER
    "Inertia"           // ForceType::INERTIA
};


unsigned int DisplayOptions(std::ostream &output, std::istream &input, const auto &prompt_str, const auto &option_strs)
{
    std::ostringstream msg_stream;
    unsigned int n = 0;
    msg_stream << prompt_str << '\n';
    for (auto i = option_strs.begin(); i != option_strs.end(); i++)
    {
        msg_stream << std::format("[{}] {}\n", n++, *i);
    }
    msg_stream << ">> ";
    const std::string msg = msg_stream.str();

    std::string line;
    while (true)
    {
        std::cout << msg;
        std::flush(output);

        line.clear();
        std::getline(std::cin, line);

        unsigned int opt;
        try
        {
            opt = static_cast<unsigned int>(std::stoul(line));
            if (opt < option_strs.size()) { return opt; }
            std::cout << "Index " << opt << " is outside of the valid range. Please try again.\n" << std::endl;
        }
        catch (...)
        {
            std::cout << "Unable to parse index \"" << line << "\". Please try again.\n" << std::endl;;
        }
    }
}


crossput::IDevice *UserDeviceSelection(const char *exit_option)
{
    while (true)
    {
        devices.clear();
        options.clear();

        std::this_thread::sleep_for(10ms);

        // Search for input devices available to the current process.
        // The first invocation of this function may also perform initialization
        // and/or system calls to bind to the OS, drivers, etc.
        crossput::DiscoverDevices();

        // Equivalent to invoking all devices' Update() method.
        // This will also attempt to connect each interface to the
        // underlying hardware/drivers if they are currently disconnected.
        crossput::UpdateAllDevices();

        // Store pointers to all devices in the vector.
        // The second argument specifies whether or not to ignore disconnected devices.
        crossput::GetDevices(devices, false);

        options.reserve(2 + devices.size());
        options.push_back(exit_option);
        options.push_back("+ Search for more input devices");

        for (const auto dev : devices)
        {
            // Fetch the status, type, and display name of the device.
            const bool is_connected = dev->IsConnected();
            const crossput::DeviceType type = dev->GetType();
            std::string name = dev->GetDisplayName();
            if (name.empty()) { name = "<Display Name Unavailable>"; }

            options.push_back(std::format("{} ({}) - Connected: {}", name, pretty_devtype_names[static_cast<int>(type)], is_connected));
        }

        const unsigned int opt = DisplayOptions(
            std::cout, std::cin,
            "Select an input device.",
            options
        );

        switch (opt)
        {
        case 0: return nullptr;
        case 1: break; // rescan
        default: return devices[opt - 2];
        }

        std::cout << std::endl;
    }
}
