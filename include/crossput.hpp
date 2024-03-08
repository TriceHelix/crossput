/*================================================================================= *
*                                   +----------+                                    *
*                                   | crossput |                                    *
*                                   +----------+                                    *
*                            Copyright 2024 Trice Helix                             *
*                                                                                   *
* This file is part of crossput and is distributed under the BSD-3-Clause License.  *
* Please refer to LICENSE.txt for additional information.                           *
* =================================================================================*/

#ifndef TRICEHELIX_CROSSPUT_H
#define TRICEHELIX_CROSSPUT_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>


namespace crossput
{
    class IDevice;
    class IMouse;
    class IKeyboard;
    class IGamepad;

    #ifdef CROSSPUT_FEATURE_FORCE
    class IForce;
    #endif // CROSSPUT_FEATURE_FORCE


    /// @brief Runtime-unique identifier for various crossput objects such as devices, callbacks, forces, etc.
    ///        This struct should be preferred over using the raw value type, e.g. in associative containers.
    ///        It has specializations for std::hash<> (and std::formatter<> since C++20).
    struct ID
    {
        /// @brief Type of value which the ID uses.
        ///        Although it is currently constant, it may be implementation-defined in the future.
        using value_type = unsigned long long;

        /// @brief Underlying value used for comparison, hashing, etc.
        value_type value;

        constexpr bool operator==(const ID &other) const noexcept { return value == other.value; }
        constexpr bool operator!=(const ID &other) const noexcept { return value != other.value; }
    };


    /// @brief Cross-Platform Device Types.
    ///        Note that all device interfaces have a single constant type because hardware that generates multiple types of input is rare,
    ///        and the input driver or operating system usually create multiple virtual parts in that case.
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class DeviceType : uint8_t
    {
        /// @brief Indicates that the device is not recognized by crossput or does not fit in any of the other categories.
        ///        Note that no interfaces are created for such devices, meaning that this value is currently almost exclusively used internally.
        UNKNOWN = 0,

        /// @brief Mouse device accessible via the IMouse interface.
        MOUSE,

        /// @brief Keyboard device accessible via the IKeyboard interface.
        KEYBOARD,

        /// @brief Gamepad, Controller, or Joystick device accessible via the IGamepad interface.
        GAMEPAD
    };


    /// @brief Cross-Platform Keycodes (influenced by physical keyboard layout and the OS layout settings).
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class Key : uint8_t
    {
        /// @brief Escape
        ESC = 0,
        ENTER,
        BACKSPACE,
        TAB,
        SPACE,
        CAPSLOCK,
        SHIFT_L,
        SHIFT_R,
        ALT_L,
        ALT_R,
        CTRL_L,
        CTRL_R,
        
        NUMROW0,
        NUMROW1,
        NUMROW2,
        NUMROW3,
        NUMROW4,
        NUMROW5,
        NUMROW6,
        NUMROW7,
        NUMROW8,
        NUMROW9,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        MINUS,
        EQUAL,
        BRACE_L,
        BRACE_R,
        SEMICOLON,
        APOSTROPHE,
        GRAVE,
        COMMA,
        DOT,
        SLASH,
        BACKSLASH,

        /// @brief Varies by keyboard, most commonly angle brackets ('<' and '>')
        KEY102,
        
        NUMLOCK,
        SCROLLLOCK,
        PAUSE,
        INSERT,
        /// @brief Delete
        DEL,
        HOME,
        END,
        PAGEUP,
        PAGEDOWN,
        
        /// @brief Left Arrow
        LEFT,
        /// @brief Up Arrow
        UP,
        /// @brief Right Arrow
        RIGHT,
        /// @brief Down Arrow
        DOWN,

        NUMPAD0,
        NUMPAD1,
        NUMPAD2,
        NUMPAD3,
        NUMPAD4,
        NUMPAD5,
        NUMPAD6,
        NUMPAD7,
        NUMPAD8,
        NUMPAD9,
        NUMPAD_DECIMAL,
        NUMPAD_PLUS,
        NUMPAD_MINUS,
        NUMPAD_MULTIPLY,
        NUMPAD_SLASH,
        
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24
    };

    /// @brief Total number of valid cross-platform keys (crossput::Key enum).
    inline constexpr size_t NUM_KEY_CODES = 112;

    /// @brief Inputs which do not have a cross-platform representation are internally handled using this value.
    ///        To check whether a key is actually valid or not, use IsValidKey().
    inline constexpr Key INVALID_KEY = static_cast<Key>(255U);
    constexpr bool IsValidKey(const Key key) noexcept { return static_cast<int>(key) < NUM_KEY_CODES; }


    /// @brief Cross-Platform Button IDs (not influenced by the naming scheme of any gamepad, physical location matters).
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class Button : uint8_t
    {
        /// @brief Circular gamepad button located north - usually labelled Y or X
        NORTH = 0,

        /// @brief Circular gamepad button located south - usually labelled A or B
        SOUTH,

        /// @brief Circular gamepad button located west - usually labelled X or Y
        WEST,

        /// @brief Circular gamepad button located east - usually labelled B or A
        EAST,

        /// @brief Directional Pad: Up
        DPAD_UP,

        /// @brief Directional Pad: Down
        DPAD_DOWN,

        /// @brief Directional Pad: Left
        DPAD_LEFT,

        /// @brief Directional Pad: Right
        DPAD_RIGHT,

        /// @brief Left shoulder button
        L1,

        /// @brief Left trigger
        L2,

        /// @brief Right shoulder button
        R1,

        /// @brief Right trigger
        R2,

        /// @brief Left thumbstick press
        THUMBSTICK_L,

        /// @brief Right thumbstick press
        THUMBSTICK_R,

        /// @brief Left Menu Button (also called "view" on some platforms and controllers)
        SELECT,

        /// @brief Right Menu Button (also called "menu" on some platforms and controllers)
        START
    };

    /// @brief Total number of valid cross-platform buttons (crossput::Button enum).
    inline constexpr size_t NUM_BUTTON_CODES = 16;

    /// @brief Inputs which do not have a cross-platform representation are internally handled using this value.
    ///        To check whether a button is actually valid or not, use IsValidButton().
    inline constexpr Button INVALID_BUTTON = static_cast<Button>(255U);
    constexpr bool IsValidButton(const Button button) noexcept { return static_cast<int>(button) < NUM_BUTTON_CODES; }


    #ifdef CROSSPUT_FEATURE_CALLBACK
    /// @brief Information about an event that involved a change in a device's status.
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class DeviceStatusChange : uint8_t
    {
        /// @brief Device has been discovered and an interface has been created for it.
        DISCOVERED = 0,

        /// @brief Device interface has (re-)connected to the underlying hardware/driver.
        CONNECTED,

        /// @brief Device interface has disconnected from the underlying hardware/driver.
        ///        This can happen as a result of physical disconnection or an error.
        DISCONNECTED,

        /// @brief Device interface is about to be destroyed, invalidating all references/pointers to it.
        DESTROYED
    };


    namespace impl
    {
        template <typename TDevice, typename... TData>
        struct _EventCallbackBase
        {
            using DevT = TDevice;
            using FuncT = std::function<void (const TDevice *const , const TData...)>;
        };

        struct _StatusCallback : _EventCallbackBase<IDevice, DeviceStatusChange> { constexpr static unsigned short CTYPEID = 1; };
        struct _MouseMoveCallback : _EventCallbackBase<IMouse, int64_t, int64_t, int64_t, int64_t> { constexpr static unsigned short CTYPEID = 2; };
        struct _MouseScrollCallback : _EventCallbackBase<IMouse, int64_t, int64_t, int64_t, int64_t> { constexpr static unsigned short CTYPEID = 3; };
        struct _MouseButtonCallback : _EventCallbackBase<IMouse, uint32_t, float, bool> { constexpr static unsigned short CTYPEID = 4; };
        struct _KeyboardKeyCallback : _EventCallbackBase<IKeyboard, Key, float, bool> { constexpr static unsigned short CTYPEID = 5; };
        struct _GamepadButtonCallback : _EventCallbackBase<IGamepad, Button, float, bool> { constexpr static unsigned short CTYPEID = 6; };
        struct _GamepadThumbstickCallback : _EventCallbackBase<IGamepad, uint32_t, float, float> { constexpr static unsigned short CTYPEID = 7; };
    }

    using StatusCallback = impl::_StatusCallback::FuncT;
    using MouseMoveCallback = impl::_MouseMoveCallback::FuncT;
    using MouseScrollCallback = impl::_MouseScrollCallback::FuncT;
    using MouseButtonCallback = impl::_MouseButtonCallback::FuncT;
    using KeyboardKeyCallback = impl::_KeyboardKeyCallback::FuncT;
    using GamepadButtonCallback = impl::_GamepadButtonCallback::FuncT;
    using GamepadThumbstickCallback = impl::_GamepadThumbstickCallback::FuncT;
    #endif // CROSSPUT_FEATURE_CALLBACK


    #ifdef CROSSPUT_FEATURE_FORCE
    /// @brief Type of force applied to a motor of a device.
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class ForceType : uint8_t
    {
        /// @brief Classic rumble effect, causing the device to vibrate.
        ///        While it does not offer much customization, it can serve as a fallback since many gamepads and haptic devices support it.
        RUMBLE = 0,

        /// @brief Constant amount of force.
        CONSTANT,

        /// @brief Amount of force changes over time.
        RAMP,

        /// @brief Periodic Force - Sine Wave.
        SINE,

        /// @brief Periodic Force - Triangle Wave.
        TRIANGLE,

        /// @brief Periodic Force - Square Wave.
        SQUARE,

        /// @brief Periodic Force - Upward Sawtooth Wave.
        SAW_UP,

        /// @brief Periodic Force - Downward Sawtooth Wave.
        SAW_DOWN,

        /// @brief Condition Force - Applied in opposition to a set state.
        SPRING,

        /// @brief Condition Force - mimics Friction.
        FRICTION,

        /// @brief Condition Force - mimics Damping.
        DAMPER,

        /// @brief Condition Force - mimics Inertia.
        INERTIA
    };

    /// @brief Total number of cross-platform force types (crossput::ForceType enum).
    inline constexpr size_t NUM_FORCE_TYPES = 12;


    /// @brief Status of a physical force constrolled via the IForce interface.
    ///        Values of this enum are sequential unsigned integers starting at 0.
    enum class ForceStatus : uint8_t
    {
        /// @brief It cannot be determined whether or not the force is active.
        ///        This status is likely caused by the device driver or runtime environment lacking the functionality to query this type of information.
        UNKNOWN = 0,

        /// @brief The force is known to not be active.
        INACTIVE,

        /// @brief The force is known to be active.
        ACTIVE
    };


    /// @brief Envelope that defines the duration and gain over time of a force.
    ///        Each "gain" value should be in range [0.0;1.0].
    ///        Each "time" value is expressed in seconds and should be in range [0.0;MAX_TIME].
    ///        The sum of all times may never exceed MAX_TIME.
    struct ForceEnvelope
    {
        /// @brief Maximum total time any force can be active, in seconds.
        static constexpr float MAX_TIME = 32.0F;

        float attack_time;
        float attack_gain;
        float sustain_time;
        float sustain_gain;
        float release_time;
        float release_gain;
    };


    /// @brief Parameters for ForceType::RUMBLE forces.
    struct RumbleForceParams
    {
        /// @brief Intensity of the low-frequency rumble motor in range [0.0;1.0], if present.
        float low_frequency;

        /// @brief Intensity of the high-frequency rumble motor in range [0.0;1.0], if present.
        float high_frequency;
    };


    /// @brief Parameters for ForceType::CONSTANT forces.
    struct ConstantForceParams
    {
        ForceEnvelope envelope;

        /// @brief Raw amount of force applied (affected by gain).
        float magnitude;
    };


    /// @brief Parameters for ForceType::RAMP forces.
    struct RampForceParams
    {
        ForceEnvelope envelope;

        /// @brief Raw amount of force applied at the beginning of the ramp (affected by gain).
        float magnitude_start;

        /// @brief Raw amount of force applied at the end of the ramp (affected by gain).
        float magnitude_end;
    };


    /// @brief Parameters for ForceType::SINE, ForceType::TRIANGLE, ForceType::SQUARE, ForceType::SAW_UP, and ForceType::SAW_DOWN forces.
    struct PeriodicForceParams
    {
        ForceEnvelope envelope;

        /// @brief Peak amount of force applied (affected by gain).
        float magnitude;

        /// @brief Frequency of the wave in Hz.
        float frequency;

        /// @brief Horizontal shift of the wave in range [0.0;1.0].
        float phase;

        /// @brief Vertical shift of the wave in terms of magnitude.
        float offset;
    };


    /// @brief Parameters for ForceType::SPRING, ForceType::FRICTION, ForceType::DAMPER, and ForceType::INERTIA forces.
    struct ConditionForceParams
    {
        /// @brief Raw amount of force applied (affected by gain, saturation, coefficient, deadzone).
        float magnitude;
        
        /// @brief Maximum amount of force applied in the left/negative area.
        float left_saturation;

        /// @brief Maximum amount of force applied in the right/positive area.
        float right_saturation;

        /// @brief Force multiplier used on the left/negative area in range [-1.0;1.0].
        float left_coefficient;

        /// @brief Force multiplier used on the right/positive area in range [-1.0;1.0].
        float right_coefficient;

        /// @brief Area around the center in which no force is applied in range [0.0;1.0].
        float deadzone;

        /// @brief Deadzone offset in range [-1.0;1.0].
        float center;
    };


    /// @brief Parameters that describe a force.
    ///        Only one of the fields "constant", "ramp", "periodic", and "condition" may be populated at any given time, depending on the type of force.
    struct ForceParams
    {
        ForceType type;
        union
        {
            RumbleForceParams rumble;
            ConstantForceParams constant;
            RampForceParams ramp;
            PeriodicForceParams periodic;
            ConditionForceParams condition;
        };
    };
    #endif // CROSSPUT_FEATURE_FORCE


    /// @brief Cross-Platform input device interface which acts as a proxy betwen the system's input implementation (drivers) and the program.
    ///        An object with this interface will always implement one of the following interfaces as well: IMouse, IKeyboard, or IGamepad.
    class IDevice
    {
    public:
        /// @return The runtime-unique ID of this device, which is not related to the underlying hardware.
        ///         It has no meaning beyond the lifetime of the program.
        virtual ID GetID() const noexcept = 0;

        /// @return Exact type of device. This enables predictable dynamic_cast behaviour on IDevice pointers.
        virtual DeviceType GetType() const noexcept = 0;

        /// @return Display name of this device, which may be provided by the hardware, input driver, operating system, or not at all.
        ///         Note that the exact format and character encoding of the string is completely implementation defined.
        ///         Will return an empty string if this device is disconnected.
        virtual std::string GetDisplayName() const = 0;

        /// @return True if this device is connected to the underlying hardware and/or driver, false otherwise.
        ///         Aggregates are only considered connected if all member devices were connected during the last update.
        virtual bool IsConnected() const = 0;

        /// @brief Update the input data of this device to the most recent state provided by the underlying hardware/driver.
        ///        If this device is currently disconnected, this will also attempt to reconnect.
        ///        Aggregates update all of their member devices and perform some additional operations to freeze input data until the next update.
        ///        Invoking this method during a callback will throw an exception.
        virtual void Update() = 0;

        #ifdef CROSSPUT_FEATURE_CALLBACK
        /// @brief Whenever this device's status changes, the callback is invoked.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *device, const DeviceStatusChange status)
        /// @return Runtime-unique ID used to unregister the callback.
        virtual ID RegisterStatusCallback(const StatusCallback &&callback) = 0;

        /// @brief Whenever this device's status changes to the specified kind, the callback is invoked.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *device, const DeviceStatusChange status)
        /// @return Runtime-unique ID used to unregister the callback.
        virtual ID RegisterStatusCallback(const StatusCallback &&callback, const DeviceStatusChange type) = 0;
        #endif // CROSSPUT_FEATURE_CALLBACK

        #ifdef CROSSPUT_FEATURE_FORCE
        /// @return Number of motors capable of performing force effects.
        ///         This may not reflect the actual amount of physical motors present on the hardware.
        ///         Aggregates report the total number of motors on all of their members.
        ///         The value will always be zero if this device is disconnected or does not support any type of force.
        virtual uint32_t GetMotorCount() const = 0;

        /// @return Factor in range [0.0;1.0] applied to the magnitude of all forces targeting the specified motor.
        ///         Always returns 0.0 if this device is disconnected or the motor does not exist.
        ///         If this device is not an aggregate, the gain of all motors is always set to 1.0 when this device (re-)connects.
        ///         Aggregates report the gain of the underlying members' motors.
        virtual float GetGain(const uint32_t motor_index) const = 0;

        /// @brief Does nothing if this device is disconnected or the motor does not exist.
        ///        Aggregates invoke SetGain() on the underlying member.
        /// @param gain Factor in range [0.0;1.0] applied to the magnitude of all forces created by the specified motor.
        virtual void SetGain(const uint32_t motor_index, float gain) = 0;

        /// @brief Does nothing if this device is disconnected.
        /// @return True if this device is connected and the specified motor supports forces of the specified type, false otherwise.
        ///         Aggregates check the underlying members' capabilities.
        virtual bool SupportsForce(const uint32_t motor_index, const ForceType type) const = 0;

        /// @brief Attempt to create a force effect of the specified type, targeting the specified motor on this device.
        ///        Devices typically only have a very limited resources for force-feedback,
        ///        meaning that there is a device-dependent maximum number of forces that can be created.
        ///        To free up resources, ensure that forces which are no longer used are cleaned up via DestroyForce().
        ///        Aggregates create the force on a member and keep additional records to allow management of the interface via both the member and aggregate.
        ///        Force creation will always fail if this device is disconnected.
        /// @param p_force Pointer to the force used to start, stop, or modify the force. If this method returns false, the value will be nullptr.
        /// @return True if the type of force is supported by the specified motor and the force was successfully created, false otherwise.
        virtual bool TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force) = 0;

        /// @brief Try to access a force that was created by this device via its unique ID.
        ///        This method works on disconnected devices.
        /// @param p_force If this method returns true, a valid pointer to the force. Otherwise, the value is undefined.
        /// @return True if the force was found, false otherwise.
        virtual bool TryGetForce(const ID id, IForce *&p_force) const = 0;

        /// @brief Destroys a force created by this device via its unique ID.
        ///        This immediately stops applying the force and frees up resources in order for more forces to be created.
        ///        All pointers and references to the force are invalidated.
        ///        This method works on disconnected devices.
        /// @param id Value returned by IForce::GetID().
        virtual void DestroyForce(const ID id) = 0;

        /// @brief Destroys all forces that were created by this device.
        ///        This immediately stops applying the forces and frees up resources in order for more forces to be created.
        ///        All pointers and references to affected forces are invalidated.
        ///        This method works on disconnected devices.
        virtual void DestroyAllForces() = 0;
        #endif // CROSSPUT_FEATURE_FORCE

        #ifdef CROSSPUT_FEATURE_AGGREGATE
        /// @return True if this device is an aggregate of multiple other devices (potentially also aggregates), false otherwise.
        virtual bool IsAggregate() const noexcept = 0;
        #endif // CROSSPUT_FEATURE_AGGREGATE

    protected:
        IDevice() = default;
        virtual ~IDevice() = default;
    };


    /// @brief Cross-Platform mouse interface.
    class IMouse : public virtual IDevice
    {
    public:
        /// @brief Query the cumulative position of the mouse cursor.
        ///        Disconnected devices set x and y to 0.
        ///        The coordinate unit is in no way related to screen coordinates.
        /// @param x Horizontal cursor position. (negative < | > positive)
        /// @param x Vertical cursor position. (negative v | ^ positive)
        virtual void GetPosition(int64_t &x, int64_t &y) const = 0;

        /// @brief Query the change in position of the mouse cursor between the last two Update() calls.
        ///        The coordinate unit is in no way related to screen coordinates.
        /// @param x Horizontal cursor delta. (negative < | > positive)
        /// @param x Vertical cursor delta. (negative v | ^ positive)
        virtual void GetDelta(int64_t &x, int64_t &y) const = 0;

        /// @brief Query the cumulative scroll of the mouse wheel.
        ///        The coordinate unit is in no way related to screen coordinates.
        /// @param x Horizontal scroll. (negative < | > positive)
        /// @param x Vertical scroll. (negative v | ^ positive)
        virtual void GetScroll(int64_t &x, int64_t &y) const = 0;

        /// @brief Query the change of the scroll wheel between the last two Update() calls.
        ///        The coordinate unit is in no way related to screen coordinates.
        /// @param x Horizontal scroll delta. (negative < | > positive)
        /// @param x Vertical scroll delta. (negative v | ^ positive)
        virtual void GetScrollDelta(int64_t &x, int64_t &y) const = 0;

        /// @return Number of buttons the mouse has, which may not match the amount present on physical hardware.
        virtual uint32_t GetButtonCount() const = 0;

        /// @brief Set the threshold of a single mouse button clamped between 0.0 and 1.0.
        /// @param index 0, 1, and 2 are mapped to the left, right, and middle (scroll wheel) mouse buttons respectively.
        ///              Larger indices are mapped to any additional buttons that the mouse may have.
        ///              If the index is out of range, nothing happens.
        /// @param threshold The button will only be considered "pressed" if its normalized state value is greater than this threshold.
        virtual void SetButtonThreshold(const uint32_t index, float threshold) = 0;

        /// @brief Set the threshold of all mouse buttons.
        /// @param threshold Mouse buttons will only be considered "pressed" if their normalized state value is greater than this threshold.
        virtual void SetGlobalThreshold(float threshold) = 0;

        /// @param index 0, 1, and 2 are mapped to the left, right, and middle (scroll wheel) mouse buttons respectively.
        ///              Larger indices are mapped to any additional buttons that the mouse may have.
        ///              If the index is out of range, 0.0 is returned.
        /// @return The threshold value previously set via SetButtonThreshold() or SetGlobalThreshold(). The default is 0.0.
        virtual float GetButtonThreshold(const uint32_t index) const = 0;

        /// @param index 0, 1, and 2 are mapped to the left, right, and middle (scroll wheel) mouse buttons respectively.
        ///              Larger indices are mapped to any additional buttons that the mouse may have.
        /// @return Normalized state value of the mouse button in range [0.0;1.0].
        ///         If the button is digital, 1.0 or 0.0 is returned based on GetButtonState().
        ///         If the button is invalid, 0.0 is returned.
        virtual float GetButtonValue(const uint32_t index) const = 0;

        /// @param index 0, 1, and 2 are mapped to the left, right, and middle (scroll wheel) mouse buttons respectively.
        ///              Larger indices are mapped to any additional buttons that the mouse may have.
        /// @param time Total time in seconds since the last state change (state value crossing threshold). If the index is out of range, the time is set to infinity.
        /// @return True if the mouse button is currently "pressed" (its normalized state value is greater than its threshold), false otherwise.
        virtual bool GetButtonState(const uint32_t index, float &time) const = 0;

        /// @param index 0, 1, and 2 are mapped to the left, right, and middle (scroll wheel) mouse buttons respectively.
        ///              Larger indices are mapped to any additional buttons that the mouse may have.
        /// @return True if the mouse button is currently "pressed" (its normalized state value is greater than its threshold), false otherwise.
        inline bool GetButtonState(const uint32_t index) const
        {
            float t;
            return GetButtonState(index, t);
        }

        #ifdef CROSSPUT_FEATURE_CALLBACK
        /// @brief Whenever the mouse is moved, the callback is invoked.
        ///        The values provided to the callback may be more precise (multiple smaller increments) than the total "delta" between updates.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const int64_t x, const int64_t y, const int64_t delta_x, const int64_t delta_y)
        /// @return ID used to unregister the callback.
        virtual ID RegisterMoveCallback(const MouseMoveCallback &&callback) = 0;

        /// @brief Whenever the mouse is scrolled, the callback is invoked.
        ///        The values provided to the callback may be more precise (multiple smaller increments) than the total "delta" between updates.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const int64_t x, const int64_t y, const int64_t delta_x, const int64_t delta_y)
        /// @return ID used to unregister the callback.
        virtual ID RegisterScrollCallback(const MouseScrollCallback &&callback) = 0;

        /// @brief Whenever the digital state or analog value of any button changes, the callback is invoked.
        ///        The button value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const uint32_t button_index, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterButtonCallback(const MouseButtonCallback &&callback) = 0;

        /// @brief Whenever the digital state or analog value of the specified button changes, the callback is invoked.
        ///        The button value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const uint32_t button_index, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterButtonCallback(const MouseButtonCallback &&callback, const uint32_t button_index) = 0;
        #endif // CROSSPUT_FEATURE_CALLBACK

    protected:
        IMouse() = default;
        virtual ~IMouse() = default;
    };


    /// @brief Cross-Platform keyboard interface.
    class IKeyboard : public virtual IDevice
    {
    public:
        /// @return The number of keys currently pressed. This only includes keys identifiable via values of the Key enum.
        ///         This value may have an upper limit lower than the actual amount of physical keys due to hardware and/or software constraints (16 is a common limit).
        virtual uint32_t GetNumKeysPressed() const = 0;

        /// @brief Set the threshold of a single key clamped between 0.0 and 1.0.
        /// @param threshold The key will only be considered "pressed" if its normalized state value is greater than this threshold.
        virtual void SetKeyThreshold(const Key key, float threshold) = 0;

        /// @brief Set the threshold of all keys.
        /// @param threshold Keys will only be considered "pressed" if their normalized state value is greater than this threshold.
        virtual void SetGlobalThreshold(float threshold) = 0;

        /// @return The threshold value previously set via SetKeyThreshold() or SetGlobalThreshold(). The default is 0.0.
        virtual float GetKeyThreshold(const Key key) const = 0;

        /// @return Normalized state value of the key in range [0.0;1.0].
        ///         If the key is digital, 1.0 or 0.0 is returned based on GetKeyState().
        ///         If the key is invalid, 0.0 is returned.
        virtual float GetKeyValue(const Key key) const = 0;

        /// @param time Total time in seconds since the last state change (state value crossing threshold). If the key is invalid, the time is set to infinity.
        /// @return True if the key is currently "pressed" (its normalized state value is greater than the threshold), false otherwise.
        virtual bool GetKeyState(const Key key, float &time) const = 0;

        /// @return True if the key is currently "pressed" (its normalized state value is greater than the threshold), false otherwise.
        inline bool GetKeyState(const Key key) const
        {
            float t;
            return GetKeyState(key, t);
        }

        #ifdef CROSSPUT_FEATURE_CALLBACK
        /// @brief Whenever the digital state or analog value of any key changes, the callback is invoked.
        ///        The key value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const Key key, const float value, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterKeyCallback(const KeyboardKeyCallback &&callback) = 0;

        /// @brief Whenever the digital state or analog value of the specified key changes, the callback is invoked.
        ///        The key value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const Key key, const float value, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterKeyCallback(const KeyboardKeyCallback &&callback, const Key key) = 0;
        #endif // CROSSPUT_FEATURE_CALLBACK

    protected:
        IKeyboard() = default;
        virtual ~IKeyboard() = default;
    };


    /// @brief Cross-platform gamepad interface.
    class IGamepad : public virtual IDevice
    {
    public:
        /// @brief Set the threshold of a single button or trigger clamped between 0.0 and 1.0.
        /// @param threshold The button or trigger will only be considered "pressed" if its normalized state value is greater than this threshold.
        virtual void SetButtonThreshold(const Button button, float threshold) = 0;

        /// @brief Set the threshold of all buttons and triggers.
        /// @param threshold Buttons and triggers will only be considered "pressed" if their normalized state value is greater than this threshold.
        virtual void SetGlobalThreshold(float threshold) = 0;

        /// @return The threshold value previously set via SetButtonThreshold() or SetGlobalThreshold(). The default is 0.0.
        virtual float GetButtonThreshold(const Button button) const = 0;

        /// @return Normalized state value of the button or trigger in range [0.0;1.0].
        ///         If the button or trigger is digital, 1.0 or 0.0 is returned based on GetButtonState().
        ///         If the button or trigger is invalid, 0.0 is returned.
        virtual float GetButtonValue(const Button button) const = 0;

        /// @param time Total time in seconds since the last state change (state value crossing threshold). If the button or trigger is invalid, the time is set to infinity.
        /// @return True if the button or trigger is currently "pressed" (its normalized state value is greater than its threshold), false otherwise.
        virtual bool GetButtonState(const Button button, float &time) const = 0;

        /// @return True if the button or trigger is currently "pressed" (its normalized state value is greater than its threshold), false otherwise.
        inline bool GetButtonState(const Button button) const
        {
            float t;
            return GetButtonState(button, t);
        }

        /// @return Number of thumbsticks that can be queried via GetThumbstick().
        ///         This may not be the number of physical thumbsticks of the hardware.
        virtual uint32_t GetThumbstickCount() const = 0;

        /// @brief Query the position of a thumbstick. The magnitude of the resulting vector may exceed 1.0.
        ///        Aggregates treat each member's thumbsticks as separate, meaning that no averaging or summing of values is performed.
        /// @param index Thumbsticks are generally indexed left-to-right. If the index is out of range, the position will be set to 0.0.
        /// @param x Horizontal value in range [-1.0;+1.0]. (negative < | > positive)
        /// @param y Vertical value in range [-1.0;+1.0].  (negative v | ^ positive)
        virtual void GetThumbstick(const uint32_t index, float &x, float &y) const = 0;

        #ifdef CROSSPUT_FEATURE_CALLBACK
        /// @brief Whenever the digital state or analog value of any button or trigger changes, the callback is invoked.
        ///        The button or trigger value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const Button button, const float value, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterButtonCallback(const GamepadButtonCallback &&callback) = 0;

        /// @brief Whenever the digital state or analog value of the specified button or trigger changes, the callback is invoked.
        ///        The button or trigger value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const Button button, const float value, const bool state)
        /// @return ID used to unregister the callback.
        virtual ID RegisterButtonCallback(const GamepadButtonCallback &&callback, const Button button) = 0;

        /// @brief Whenever any thumbstick is moved, the callback is invoked.
        ///        The values provided to the callback may be an intermediate position between updates that is not equal to the final thumbstick position.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const uint32_t thumbstick_index, const float x, const float y)
        /// @return ID used to unregister the callback.
        virtual ID RegisterThumbstickCallback(const GamepadThumbstickCallback &&callback) = 0;

        /// @brief Whenever the specified thumbstick is moved, the callback is invoked.
        ///        The values provided to the callback may be an intermediate position between updates that is not equal to the final thumbstick position.
        ///        Invoking this method during a callback will throw an exception.
        /// @param callback void (const IDevice *this_device, const uint32_t thumbstick_index, const float x, const float y)
        /// @return ID used to unregister the callback.
        virtual ID RegisterThumbstickCallback(const GamepadThumbstickCallback &&callback, const uint32_t index) = 0;
        #endif // CROSSPUT_FEATURE_CALLBACK

    protected:
        IGamepad() = default;
        virtual ~IGamepad() = default;
    };


    #ifdef CROSSPUT_FEATURE_FORCE
    /// @brief Cross-platform force interface.
    ///        Represents a single instance of force applied by a single motor of a device.
    ///        This may cause a constant force, a vibration/rumble effect, a spring, or something else entirely.
    class IForce
    {
    public:
        /// @return Unique ID of the force used for IDevice::DestroyForce().
        virtual ID GetID() const noexcept = 0;

        /// @return Type of force.
        virtual ForceType GetType() const noexcept = 0;

        /// @return Pointer to device on which the force acts.
        ///         If the force was created via an aggregate, this will return an underlying member and not the aggregate itself.
        ///         Returns nullptr if the force has been orphaned (IsOrphaned() == true).
        virtual IDevice *GetDevice() const noexcept = 0;

        /// @return True if the target device has disconnected since the creation of the force, false otherwise.
        virtual bool IsOrphaned() const noexcept = 0;

        /// @return Current status of the physical force. Will always be ForceStatus::INACTIVE if the force has been orphaned.
        virtual ForceStatus GetStatus() const = 0;

        /// @return Index of the motor on the target device that is used to physically apply the force.
        ///         Some devices/platforms do not distinguish between motors, in which case 0 is returned by all forces.
        ///         Forces created via aggregates will likely return a different value than the one specified during creation.
        virtual uint32_t GetMotorIndex() const noexcept = 0;

        /// @return Access to the parameters of the force.
        ///         After modifying any of the values, call WriteParams() to apply all changes.
        virtual ForceParams &Params() = 0;

        /// @brief Upload the current force parameters to the hardware.
        ///        Will always fail if the force has been orphaned or the type set within the parameters has changed.
        /// @return True if the current parameters have been uploaded to the hardware, false otherwise.
        virtual bool WriteParams() = 0;

        /// @brief Starts or stops applying the force.
        ///        Does nothing if the force already matches the activity or has been orphaned.
        /// @param active True will implicitly invoke WriteParams() and start applying the force, false will stop it.
        virtual void SetActive(bool active) = 0;

        /// @brief Starts applying the force. Does nothing if the force has been orphaned.
        inline void Start() { SetActive(true); }

        /// @brief Stops applying the force. Does nothing if the force has been orphaned.
        inline void Stop() { SetActive(false); }

    protected:
        IForce() = default;
        virtual ~IForce() = default;
    };
    #endif // CROSSPUT_FEATURE_FORCE


    // GLOBAL DEVICE MANAGEMENT

    /// @brief Search for input devices which are not accessible via an IDevice interface yet.
    ///        Note that all created interfaces initially appear to be disconnected until they are updated for the first time.
    ///        Invoking this function during a callback will throw an exception.
    /// @returns Number of new devices discovered.
    size_t DiscoverDevices();

    /// @brief Effectively invokes the Update() method on all devices (including aggregates).
    ///        Depending on the aggregation structure, this may cause a single device to be updated multiple times.
    ///        Invoking this function during a callback will throw an exception.
    void UpdateAllDevices();

    /// @brief Remove all devices from the internal registry and delete their interface objects.
    ///        All device pointers and references are invalidated, including aggregates.
    ///        New interfaces for the underlying hardware may be created by DiscoverDevices().
    ///        Invoking this function during a callback will throw an exception.
    void DestroyAllDevices();

    /// @param ignore_disconnected If true, disconnected devices are occluded from the total count.
    /// @return Total number of devices (including agregates), optionally ignoring disconnected devices.
    size_t GetDeviceCount(const bool ignore_disconnected = false);

    /// @brief Copy pointers to all available devices to the given vector.
    /// @param ignore_disconnected If true, filters out disconnected devices from the results.
    /// @return Number of new entries in the vector.
    size_t GetDevices(std::vector<IDevice *> &devices, const bool ignore_disconnected = false);

    /// @brief Copy pointers to all available mouse devices to the given vector.
    /// @param ignore_disconnected If true, filters out disconnected devices from the results.
    /// @return Number of new entries in the vector.
    size_t GetMice(std::vector<IMouse *> &mice, const bool ignore_disconnected = false);

    /// @brief Copy pointers to all available keyboard devices to the given vector.
    /// @param ignore_disconnected If true, filters out disconnected devices from the results.
    /// @return Number of new entries in the vector.
    size_t GetKeyboards(std::vector<IKeyboard *> &keyboards, const bool ignore_disconnected = false);

    /// @brief Copy pointers to all available gamepad devices to the given vector.
    /// @param ignore_disconnected If true, filters out disconnected devices from the results.
    /// @return Number of new entries in the vector.
    size_t GetGamepads(std::vector<IGamepad *> &gamepads, const bool ignore_disconnected = false);

    /// @brief Access a device by its runtime-unique ID.
    /// @returns Pointer to device (or aggregate) if it exists, nullptr otherwise.
    IDevice *GetDevice(const ID id);

    /// @brief Remove the specified device from the internal registry and delete its interface object.
    ///        All device pointers and references are invalidated, including aggregates.
    ///        A new interface for the underlying hardware may be created within DiscoverDevices().
    ///        This function does nothing if the ID cannot be resolved.
    ///        Invoking this function during a callback will throw an exception.
    void DestroyDevice(const ID id);


    // GLOBAL EVENT API

    #ifdef CROSSPUT_FEATURE_CALLBACK
    /// @brief Unregister a global or device-specific callback of any kind.
    ///        Invoking this function during a callback will throw an exception.
    void UnregisterCallback(const ID id);

    /// @brief Unregister all global and device-specific callbacks of any kind.
    ///        Invoking this function during a callback will throw an exception.
    void UnregisterAllCallbacks();

    /// @brief Whenever any device status changes, the callback is invoked.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const DeviceStatusChange status)
    /// @return Runtime-unique ID used to unregister the callback.
    ID RegisterGlobalStatusCallback(const StatusCallback &&callback);

    /// @brief Whenever any device status changes to the specified kind, the callback is invoked.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const DeviceStatusChange status)
    /// @return Runtime-unique ID used to unregister the callback.
    ID RegisterGlobalStatusCallback(const StatusCallback &&callback, const DeviceStatusChange type);

    /// @brief Whenever any mouse is moved, the callback is invoked.
    ///        The values provided to the callback may be more precise (multiple smaller increments) than the total "delta" between updates.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const int64_t position_x, const int64_t position_y, const int64_t delta_x, const int64_t delta_y)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalMouseMoveCallback(const MouseMoveCallback &&callback);

    /// @brief Whenever any mouse is scrolled, the callback is invoked.
    ///        The values provided to the callback may be more precise (multiple smaller increments) than the total "delta" between updates.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const int64_t delta_x, const int64_t delta_y)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalMouseScrollCallback(const MouseScrollCallback &&callback);

    /// @brief Whenever the digital state or analog value of any mouse button changes on any mouse, the callback is invoked.
    ///        The button value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const uint32_t button_index, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalMouseButtonCallback(const MouseButtonCallback &&callback);

    /// @brief Whenever the digital state or analog value of the specified mouse button changes on any mouse, the callback is invoked.
    ///        The button value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const uint32_t button_index, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalMouseButtonCallback(const MouseButtonCallback &&callback, const uint32_t button_index);

    /// @brief Whenever the digital state or analog value of any key changes on any keyboard, the callback is invoked.
    ///        The key value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const Key key, const float value, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalKeyboardKeyCallback(const KeyboardKeyCallback &&callback);

    /// @brief Whenever the digital state or analog value of the specified key changes on any keyboard, the callback is invoked.
    ///        The key value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const Key key, const float value, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalKeyboardKeyCallback(const KeyboardKeyCallback &&callback, const Key key);

    /// @brief Whenever the digital state or analog value of any button or trigger changes on any gamepad, the callback is invoked.
    ///        The button or trigger value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const Button button, const float value, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalGamepadButtonCallback(const GamepadButtonCallback &&callback);

    /// @brief Whenever the digital state or analog value of the specified button or trigger changes on any gamepad, the callback is invoked.
    ///        The button or trigger value/state provided to the callback may be an intermediate between updates that is not equal to the final value/state.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const Button button, const float value, const bool state)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalGamepadButtonCallback(const GamepadButtonCallback &&callback, const Button button);

    /// @brief Whenever any thumbstick is moved on any gamepad, the callback is invoked.
    ///        The values provided to the callback may be an intermediate position between updates that is not equal to the final thumbstick position.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const uint32_t thumbstick_index, const float x, const float y)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalGamepadThumbstickCallback(const GamepadThumbstickCallback &&callback);

    /// @brief Whenever the specified thumbstick is moved on any gamepad, the callback is invoked.
    ///        The values provided to the callback may be an intermediate position between updates that is not equal to the final thumbstick position.
    ///        Invoking this function during a callback will throw an exception.
    /// @param callback void (const IDevice *device, const uint32_t thumbstick_index, const float x, const float y)
    /// @return ID used to unregister the callback.
    ID RegisterGlobalGamepadThumbstickCallback(const GamepadThumbstickCallback &&callback, const uint32_t index);
    #endif // CROSSPUT_FEATURE_CALLBACK


    // GLOBAL AGGREGATE API

    #ifdef CROSSPUT_FEATURE_AGGREGATE
    /// @brief Create an interface which aggregates the input and features of all member devices.
    ///        The type of all members must be identical, otherwise aggregation will fail.
    ///        Aggregates themselves are also allowed to be members, however be aware that circular aggregation causes undefined behaviour.
    ///        Invoking this function during a callback will throw an exception.
    /// @param ids IDs of the member devices. Duplicate IDs will cause undefined behaviour.
    /// @param type_hint When set to a value other than DeviceType::UNKNOWN, ensures that the final aggregate's
    ///                  device type matches this value. When this is not the case, aggregation will fail.
    /// @return Pointer to a new aggregate if multiple members are specified.
    ///    [OR] Pointer to an existing aggregate if the combination of members has already been aggregated.
    ///    [OR] Pointer to an existing device if it is the only specified member.
    ///    [OR] nullptr if no members are specified or any kind of failure occurrs during aggregation.
    IDevice *Aggregate(const std::vector<ID> &ids, const DeviceType type_hint = DeviceType::UNKNOWN);
    #endif // CROSSPUT_FEATURE_AGGREGATE
}


template <>
struct std::hash<crossput::ID>
{
    inline size_t operator()(const crossput::ID &id) const noexcept(noexcept(std::hash<crossput::ID::value_type>().operator()(id.value)))
    {
        return std::hash<crossput::ID::value_type>().operator()(id.value);
    }
};


#if __cplusplus >= 202002L // C++20
#include <format>
template <typename TChar>
struct std::formatter<crossput::ID, TChar>
{
private:
    using formatter_type = std::formatter<crossput::ID::value_type, TChar>;

public:
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) const
    {
        return formatter_type().parse(ctx);
    }

    template<typename FormatContext>
    inline auto format(const crossput::ID &id, FormatContext &ctx) const noexcept(noexcept(std::formatter<crossput::ID::value_type>().format(id.value, ctx)))
    {
        return formatter_type().format(id.value, ctx);
    }
};
#endif // C++20

#endif // TRICEHELIX_CROSSPUT_H
