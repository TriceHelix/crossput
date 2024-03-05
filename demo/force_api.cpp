/*================================================================================= *
*                                   +----------+                                    *
*                                   | crossput |                                    *
*                                   +----------+                                    *
*                            Copyright 2024 Trice Helix                             *
*                                                                                   *
* This file is part of crossput and is distributed under the BSD-3-Clause License.  *
* Please refer to LICENSE.txt for additional information.                           *
* =================================================================================*/

#include <algorithm>
#include "shared_api.hpp"


// cache
std::vector<crossput::ForceType> sftypes;


void TestRumble(crossput::IForce *const p_force, const float duration)
{
    // Rumble-capable devices typically have low-frequency and high-frequency motors,
    // which are addressed together via a single force interface.
    // The strength of the physical rumble is affected by the intensity values and the motor's gain.
    // You can design more intricate effects by modifying the parameters over time,
    // applying each change via WriteParams().
    auto &params = p_force->Params();
    params.rumble.low_frequency = 1.0F;
    params.rumble.high_frequency = 1.0F;

    // Equivalent to SetActive(true).
    // This also applies the changes made to the force's parameters automatically.
    p_force->Start();

    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>(duration * 1E6F)));

    // Rumble forces do not provide an automatic "timeout", so it has to be manually stopped.
    // This has the same effect as setting the high and low intensities to 0.0.
    // Note: Some platforms or hardware have a maximum duration for forces,
    // meaning that you may need to re-apply the parameters every now and then to
    // ensure the effect persists for prolonged durations (~30+ seconds).
    // However, such cases are obviously rare because...
    // Who in their right mind would want multiple minutes of uninterrupted rumble?!
    p_force->Stop();

    std::cout << "Rumble test completed!" << std::endl;
}


void TestConstant(crossput::IForce *const p_force, const float duration)
{
    // TODO
    std::cout << "The constant force test has not been implemented yet due to the developer not owning any hardware capable of performing the test. :(" << std::endl;
}


void TestRamp(crossput::IForce *const p_force, const float duration)
{
    // TODO
    std::cout << "The ramp force test has not been implemented yet due to the developer not owning any hardware capable of performing the test. :(" << std::endl;
}


void TestPeriodic(crossput::IForce *const p_force, const float duration)
{
    // TODO
    std::cout << "The periodic force test has not been implemented yet due to the developer not owning any hardware capable of performing the test. :(" << std::endl;
}


void TestCondition(crossput::IForce *const p_force, const float duration)
{
    // TODO
    std::cout << "The condition force test has not been implemented yet due to the developer not owning any hardware capable of performing the test. :(" << std::endl;
}


int main()
{
    std::cout << "Welcome to the crossput force API demo.\n---" << std::endl;

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

        // Query the number of targetable motors on the device.
        const uint32_t motor_count = p_device->GetMotorCount();

        if (motor_count == 0)
        {
            std::cout << "The selected device does not have any force-capable motors.\n" << std::endl;
            continue;
        }

        uint32_t target_motor;
        if (motor_count > 1)
        {  
            // motor selection prompt
            std::string line;
            const std::string msg = std::format("Select one of the {} motors to test (index).\n>> ", motor_count);
            while (true)
            {
                std::cout << msg;
                std::flush(std::cout);

                line.clear();
                std::getline(std::cin, line);

                uint32_t opt;
                try
                {
                    opt = static_cast<uint32_t>(std::stoul(line));
                    if (opt < motor_count) { target_motor = opt; break; }
                    std::cout << "Index " << opt << " is outside of the valid range. Please try again.\n" << std::endl;
                }
                catch (...)
                {
                    std::cout << std::format("Unable to parse index \"{}\". Please try again.\n", line)  << std::endl;;
                }
            }
        }
        else
        {
            // only one motor exists
            target_motor = 0;
        }

        // gain prompt
        float gain;
        std::string line;
        const std::string msg = std::format("Set the master gain of the selected motor (between 0.0 and 1.0).\n>> ", motor_count);
        while (true)
        {
            std::cout << msg;
            std::flush(std::cout);

            line.clear();
            std::getline(std::cin, line);

            try
            {
                gain = std::stof(line);
                break;
            }
            catch (...)
            {
                std::cout << std::format("Unable to parse gain value \"{}\". Please try again.\n", line)  << std::endl;;
            }
        }

        gain = std::clamp(gain, 0.0F, 1.0F);
        p_device->SetGain(target_motor, gain);
        std::cout << std::format("Set the gain of motor {} to {}.\n", target_motor, gain) << std::endl;

        options.clear();
        options.push_back("< BACK");

        // Find all supported force types of the target motor.
        sftypes.clear();
        for (unsigned int i = 0; i < crossput::NUM_FORCE_TYPES; i++)
        {
            const crossput::ForceType ft = static_cast<crossput::ForceType>(i);
            if (p_device->SupportsForce(target_motor, ft))
            {
                sftypes.push_back(ft);
                options.push_back(pretty_ftype_names[i]);
            }
        }

        const unsigned int opt = DisplayOptions(
            std::cout, std::cin,
            "Select a type of force.",
            options
        );

        if (opt > 0)
        {
            const crossput::ForceType type = sftypes[opt - 1];

            // duration prompt
            float duration;
            std::string line;
            const std::string msg = std::format("Set the duration of the force (in seconds).\n>> ", motor_count);
            while (true)
            {
                std::cout << msg;
                std::flush(std::cout);

                line.clear();
                std::getline(std::cin, line);

                try
                {
                    duration = std::stof(line);
                    break;
                }
                catch (...)
                {
                    std::cout << std::format("Unable to parse duration \"{}\". Please try again.\n", line)  << std::endl;;
                }
            }

            duration = std::clamp(duration, 0.0F, crossput::ForceEnvelope::MAX_TIME);
            std::cout << std::format("Force duration: {} seconds", duration) << std::endl;

            // Attempt to create a force of the selected type.
            // For various reasons, such as limited device resources or internal errors,
            // force creation may fail even if the force type is supported.
            // Whether or not the pointer to the force interface is valid and the force exists
            // is indicated by the boolean return value of the method.
            crossput::IForce *p_force;
            if (p_device->TryCreateForce(target_motor, type, p_force))
            {
                std::cout << std::format("Successfully created force of type {} with ID {}.", pretty_ftype_names[static_cast<int>(type)], p_force->GetID()) << std::endl;

                switch (type)
                {
                case crossput::ForceType::RUMBLE: TestRumble(p_force, duration); break;
                case crossput::ForceType::CONSTANT: TestConstant(p_force, duration); break;
                case crossput::ForceType::RAMP: TestRamp(p_force, duration); break;

                case crossput::ForceType::SINE:
                case crossput::ForceType::TRIANGLE:
                case crossput::ForceType::SQUARE:
                case crossput::ForceType::SAW_UP:
                case crossput::ForceType::SAW_DOWN:
                    TestPeriodic(p_force, duration);
                    break;

                case crossput::ForceType::SPRING:
                case crossput::ForceType::FRICTION:
                case crossput::ForceType::DAMPER:
                case crossput::ForceType::INERTIA:
                    TestCondition(p_force, duration);
                    break;
                }

                // Once the force is no longer used, clean up its underlying resources
                // via DestroyForce(). Alternatively, all forces affecting a particular
                // device can be destroyed via DestroyAllForces().
                p_device->DestroyForce(p_force->GetID());
            }
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
