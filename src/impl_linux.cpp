/*================================================================================= *
*                                   +----------+                                    *
*                                   | crossput |                                    *
*                                   +----------+                                    *
*                            Copyright 2024 Trice Helix                             *
*                                                                                   *
* This file is part of crossput and is distributed under the BSD-3-Clause License.  *
* Please refer to LICENSE.txt for additional information.                           *
* =================================================================================*/

#include "common.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_set>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>


// clock used for timestamps
#define CROSSPUT_TSCLOCKID CLOCK_REALTIME

// fetch bit at offset within an array
#define GETBIT_(array, at) (((reinterpret_cast<const unsigned char *>(array)[(at) / 8]) >> ((at) % 8)) & 1)


constexpr bool AnyBit(const unsigned char *const ptr, const size_t len)
{
    // likely SIMD optimized with fixed-size array
    for (size_t i = 0; i < len; i++) { if (ptr[i] != 0) { return true; } }
    return false;
}


const std::string DEV_INPUT_DIR = "/dev/input";
const std::regex REGEX_EVENTX_FN("^event[0-9]+$");


constexpr bool input_id_equal(const input_id &left, const input_id &right)
{
    return (left.bustype == right.bustype)
        && (left.vendor == right.vendor)
        && (left.product == right.product)
        && (left.version == right.version);
}


constexpr uint64_t input_id_to_uint64(const input_id &value)
{
    return (static_cast<uint64_t>(value.bustype) << 48)
        | (static_cast<uint64_t>(value.vendor) << 32)
        | (static_cast<uint64_t>(value.product) << 16)
        | static_cast<uint64_t>(value.version);
}


// Of the sections marked as "a", "b", and "c", only one will be filled with valid data that can be used to identify an input device.
// Which of the sections to use is indicated by "idm".
struct LinuxHardwareID
{
    // indicator for which identification method to use
    int idm;
    union
    {
        struct
        {
            char unique_id[128];
        } a;

        struct
        {
            char phys_location[128];
            input_id metadata;
        } b;

        struct
        {
            unsigned int eventx;
        } c;
    };
    

    inline bool operator==(const LinuxHardwareID &other) const
    {
        if (idm != other.idm) { return false; }

        switch (idm)
        {
        case 0:
            return std::strcmp(a.unique_id, other.a.unique_id) == 0;

        case 1:
            return (std::strcmp(b.phys_location, other.b.phys_location) == 0)
                && input_id_equal(b.metadata, other.b.metadata);

        case 2:
            return c.eventx == other.c.eventx;

        default:
            return false;
        }
    }


    constexpr ~LinuxHardwareID() {}
};

constexpr LinuxHardwareID INVALID_LXHWID { .idm = -1 };


// LinuxHardwareID from open eventX file
void GetHWID(LinuxHardwareID &id, const int fd, const unsigned int x)
{
    assert(fd >= 0);

    int stat1, stat2;

    // A (driver-provided UID)
    id.idm = 0;
    std::memset(id.a.unique_id, 0, sizeof(id.a.unique_id));
    stat1 = ioctl(fd, EVIOCGUNIQ(sizeof(LinuxHardwareID::a.unique_id) - 1), &id.a.unique_id);
    if (stat1 >= 0) { return; }

    // B (device info + physical location)
    id.idm = 1;
    std::memset(id.b.phys_location, 0, sizeof(id.b.phys_location));
    stat1 = ioctl(fd, EVIOCGPHYS(sizeof(LinuxHardwareID::b.phys_location) - 1), &id.b.phys_location);
    stat2 = ioctl(fd, EVIOCGID, &id.b.metadata);
    if (stat1 >= 0 && stat2 >= 0) { return; }

    // C (eventX fallback, weakest identity)
    id.idm = 2;
    id.c.eventx = x;
}


template<>
struct std::hash<LinuxHardwareID>
{
public:
    size_t operator()(const LinuxHardwareID &id) const
    {
        switch (id.idm)
        {
        case 0:
            return fnv1a_strhash(id.a.unique_id);

        case 1:
            return (fnv1a_strhash(id.b.phys_location) * 23) + input_id_to_uint64(id.b.metadata);

        case 2:
            return std::hash<unsigned int>().operator()(id.c.eventx);

        default:
            return 0;
        }
    }
};


namespace crossput
{
    class LinuxDevice;
    class LinuxForce;


    // Linux Keycode -> crossput Key
    constexpr Key keycode_mapping[] =
    {
/*0000*/INVALID_KEY,
        Key::ESC,
        Key::NUMROW1,
        Key::NUMROW2,
        Key::NUMROW3,
        Key::NUMROW4,
        Key::NUMROW5,
        Key::NUMROW6,
        Key::NUMROW7,
        Key::NUMROW8,
/*0010*/Key::NUMROW9,
        Key::NUMROW0,
        Key::MINUS,
        Key::EQUAL,
        Key::BACKSPACE,
        Key::TAB,
        Key::Q,
        Key::W,
        Key::E,
        Key::R,
/*0020*/Key::T,
        Key::Y,
        Key::U,
        Key::I,
        Key::O,
        Key::P,
        Key::BRACE_L,
        Key::BRACE_R,
        Key::ENTER,
        Key::CTRL_L,
/*0030*/Key::A,
        Key::S,
        Key::D,
        Key::F,
        Key::G,
        Key::H,
        Key::J,
        Key::K,
        Key::L,
        Key::SEMICOLON,
/*0040*/Key::APOSTROPHE,
        Key::GRAVE,
        Key::SHIFT_L,
        Key::BACKSLASH,
        Key::Z,
        Key::X,
        Key::C,
        Key::V,
        Key::B,
        Key::N,
/*0050*/Key::M,
        Key::COMMA,
        Key::DOT,
        Key::SLASH,
        Key::SHIFT_R,
        Key::NUMPAD_MULTIPLY,
        Key::ALT_L,
        Key::SPACE,
        Key::CAPSLOCK,
        Key::F1,
/*0060*/Key::F2,
        Key::F3,
        Key::F4,
        Key::F5,
        Key::F6,
        Key::F7,
        Key::F8,
        Key::F9,
        Key::F10,
        Key::NUMLOCK,
/*0070*/Key::SCROLLLOCK,
        Key::NUMPAD7,
        Key::NUMPAD8,
        Key::NUMPAD9,
        Key::NUMPAD_MINUS,
        Key::NUMPAD4,
        Key::NUMPAD5,
        Key::NUMPAD6,
        Key::NUMPAD_PLUS,
        Key::NUMPAD1,
/*0080*/Key::NUMPAD2,
        Key::NUMPAD3,
        Key::NUMPAD0,
        Key::NUMPAD_DECIMAL,
        INVALID_KEY,
        INVALID_KEY,
        Key::KEY102,
        Key::F11,
        Key::F12,
        INVALID_KEY,
/*0090*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::ENTER,
        Key::CTRL_R,
        Key::NUMPAD_SLASH,
        INVALID_KEY,
/*0100*/Key::ALT_R,
        Key::ENTER, // KEY_LINEFEED is obsolete, just remap to Enter
        Key::HOME,
        Key::UP,
        Key::PAGEUP,
        Key::LEFT,
        Key::RIGHT,
        Key::END,
        Key::DOWN,
        Key::PAGEDOWN,
/*0110*/Key::INSERT,
        Key::DEL,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::ENTER, // KEY_KPENTER does not have a Windows equivalent, just remap to Enter
        INVALID_KEY,
        Key::PAUSE,
/*0120*/INVALID_KEY,
        Key::NUMPAD_DECIMAL, // KEY_KPCOMMA is intl. variant of Numpad Decimal
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0130*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0140*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0150*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0160*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0170*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0180*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::F13,
        Key::F14,
        Key::F15,
        Key::F16,
        Key::F17,
        Key::F18,
        Key::F19,
/*0190*/Key::F20,
        Key::F21,
        Key::F22,
        Key::F23,
        Key::F24,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0200*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0210*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0220*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0230*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0240*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0250*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY
    };

    constexpr size_t KEYCODE_MAPPING_LEN = sizeof(keycode_mapping) / sizeof(keycode_mapping[0]);


    // crossput Key -> Linux Keycode
    constexpr std::array<unsigned short, NUM_KEY_CODES> reverse_keycode_mapping = []()
    {
        std::array<unsigned short, NUM_KEY_CODES> values;
        for (unsigned short i = 0; i <= 255; i++)
        {
            const Key k = keycode_mapping[i];
            if (IsValidKey(k)) { values[static_cast<int>(k)] = i; }
        }
        return values;
    }();


    // Linux Keycode -> crossput Button
    constexpr Button button_mapping[] =
    {
        Button::SOUTH,          // BTN_SOUTH
        Button::EAST,           // BTN_EAST
        INVALID_BUTTON,         // BTN_C
        Button::NORTH,          // BTN_NORTH
        Button::WEST,           // BTN_WEST
        INVALID_BUTTON,         // BTN_Z
        Button::L1,             // BTN_TL
        Button::R1,             // BTN_TR
        Button::L2,             // BTN_TL2
        Button::R2,             // BTN_TR2
        Button::SELECT,         // BTN_SELECT
        Button::START,          // BTN_START
        INVALID_BUTTON,         // BTN_MODE
        Button::THUMBSTICK_L,   // BTN_THUMBL
        Button::THUMBSTICK_R    // BTN_THUMBR
    };

    constexpr size_t BUTTON_MAPPING_LEN = sizeof(button_mapping) / sizeof(button_mapping[0]);


    // Linux Keycode (dpad) -> crossput Button
    constexpr Button button_mapping_dpad[] =
    {
        Button::DPAD_UP,    // BTN_DPAD_UP
        Button::DPAD_DOWN,  // BTN_DPAD_DOWN
        Button::DPAD_LEFT,  // BTN_DPAD_LEFT
        Button::DPAD_RIGHT  // BTN_DPAD_RIGHT
    };

    constexpr size_t BUTTON_MAPPING_DPAD_LEN = sizeof(button_mapping_dpad) / sizeof(button_mapping_dpad[0]);


    // crossput Button -> Linux Keycode
    constexpr std::array<unsigned short, NUM_BUTTON_CODES> reverse_button_mapping = []()
    {
        std::array<unsigned short, NUM_BUTTON_CODES> values;

        // regular buttons
        for (unsigned short i = BTN_SOUTH; i <= BTN_THUMBR; i++)
        {
            const Button b = button_mapping[i - BTN_SOUTH];
            if (IsValidButton(b)) { values[static_cast<int>(b)] = i; }
        }

        // dpad
        for (unsigned short i = BTN_DPAD_UP; i <= BTN_DPAD_RIGHT; i++)
        {
            const Button b = button_mapping_dpad[i - BTN_DPAD_UP];
            if (IsValidButton(b)) { values[static_cast<int>(b)] = i; }
        }

        return values;
    }();


    // key codes used to recognize mouse input devices
    constexpr unsigned short devrecog_mouse[] =
    {
        BTN_LEFT,
        BTN_RIGHT,
        BTN_MIDDLE,
        BTN_SIDE,
        BTN_EXTRA,
        BTN_FORWARD,
        BTN_SIDE,
        BTN_TASK
    };

    constexpr size_t DEVRECOG_MOUSE_LEN = sizeof(devrecog_mouse) / sizeof(devrecog_mouse[0]);


    // key codes that are usually not found on any supported devices
    // -> if a large amount of key code capabilities is within this list, a device will likely be discarded during type deduction
    constexpr unsigned short devrecog_avoid[] =
    {
        BTN_DIGI,
        BTN_TOOL_AIRBRUSH,
        BTN_TOOL_BRUSH,
        BTN_TOOL_FINGER,
        BTN_TOOL_LENS,
        BTN_TOOL_PEN,
        BTN_TOOL_PENCIL,
        BTN_TOOL_RUBBER,
        BTN_STYLUS,
        BTN_STYLUS2,
        BTN_STYLUS3,
        BTN_TOUCH,
        BTN_TOOL_DOUBLETAP,
        BTN_TOOL_TRIPLETAP,
        BTN_TOOL_QUADTAP,
        BTN_TOOL_QUINTTAP,
        BTN_WHEEL,
        BTN_GEAR_DOWN,
        BTN_GEAR_UP
    };

    constexpr size_t DEVRECOG_AVOID_LEN = sizeof(devrecog_avoid) / sizeof(devrecog_avoid[0]);


    #ifdef CROSSPUT_FEATURE_FORCE
    // ForceType -> ff_effect::type
    constexpr unsigned short effect_type_mapping[NUM_FORCE_TYPES] =
    {
        FF_RUMBLE,
        FF_CONSTANT,
        FF_RAMP,
        FF_PERIODIC,
        FF_PERIODIC,
        FF_PERIODIC,
        FF_PERIODIC,
        FF_PERIODIC,
        FF_SPRING,
        FF_FRICTION,
        FF_DAMPER,
        FF_INERTIA
    };


    constexpr unsigned short periodic_waveform_mapping[NUM_FORCE_TYPES] =
    {
        0,
        0,
        0,
        FF_SINE,
        FF_TRIANGLE,
        FF_SQUARE,
        FF_SAW_UP,
        FF_SAW_DOWN,
        0,
        0,
        0,
        0
    };
    #endif // CROSSPUT_FEATURE_FORCE


    // all hardware IDs that are in use
    std::unordered_set<LinuxHardwareID> nat_device_ids;


    inline timestamp_t GetTimestampNow()
    {
        timespec t;
        return (clock_gettime(CROSSPUT_TSCLOCKID, &t) == 0)
            ? ((static_cast<timestamp_t>(t.tv_sec) * static_cast<timestamp_t>(1000000)) + static_cast<timestamp_t>(t.tv_nsec / static_cast<long>(1000)))
            : 0;
    }


    constexpr timestamp_t GetEventTimestamp(const input_event &ev)
    {
        return (static_cast<timestamp_t>(ev.input_event_sec) * static_cast<timestamp_t>(1000000)) + static_cast<timestamp_t>(ev.input_event_usec);
    }


    // used to normalize values of analog buttons/triggers/thumbsticks
    struct AbsValNorm
    {
        int32_t min;
        int32_t max;
        double inv_delta_n;
        double inv_delta_p;


        constexpr AbsValNorm() = default;

        AbsValNorm(const input_absinfo &info)
        {
            min = info.minimum;
            max = info.maximum;
            inv_delta_n = std::max(0.0, 1.0 / static_cast<double>(std::min(max, static_cast<int32_t>(0)) - min));
            inv_delta_p = std::max(0.0, 1.0 / static_cast<double>(max - std::max(static_cast<int32_t>(0), min)));
        }
    };


    inline bool AbsValueNormFromIoctl(const int fd, const int code, AbsValNorm &norm)
    {
        input_absinfo info;
        const bool status = ioctl(fd, EVIOCGABS(code), &info) >= 0;
        if (status) { norm = AbsValNorm(info); }
        return status;
    }


    constexpr float NormalizeAbsValue(const AbsValNorm &norm, int32_t raw_value)
    {
        raw_value = std::clamp(raw_value, norm.min, norm.max);
        return raw_value < 0
            ? std::clamp(static_cast<float>(static_cast<double>(raw_value - std::min(norm.max, static_cast<int32_t>(0))) * norm.inv_delta_n), -1.0F, 0.0F)
            : std::clamp(static_cast<float>(static_cast<double>(raw_value - std::max(static_cast<int32_t>(0), norm.min)) * norm.inv_delta_p), 0.0F, 1.0F);
    }

    constexpr float NormalizeAbsValue(const input_absinfo &info)
    {
        const int32_t raw_value = std::clamp(info.value, info.minimum, info.maximum);

        int32_t flr;
        double inv_delta;
        if (raw_value < 0)
        {
            flr = std::min(info.maximum, static_cast<int32_t>(0));
            inv_delta = std::max(0.0, 1.0 / static_cast<double>(flr - info.minimum));
        }
        else
        {
            flr = std::max(static_cast<int32_t>(0), info.minimum);
            inv_delta = std::max(0.0, 1.0 / static_cast<double>(info.maximum - flr));
        }

        return std::clamp(static_cast<float>(static_cast<double>(raw_value - flr) * inv_delta), -1.0F, 1.0F);
    }


    inline bool AbsValueFromIoctl(const int fd, const int code, float &abs_value)
    {
        input_absinfo info;
        const bool status = ioctl(fd, EVIOCGABS(code), &info) >= 0;
        if (status) { abs_value = NormalizeAbsValue(info); }
        return status;
    }


    class LinuxDevice :
        public virtual BaseInterface,
        public virtual DeviceCallbackManager,
        public virtual DeviceForceManager<LinuxDevice>
    {
        friend LinuxForce;

    protected:
        const LinuxHardwareID hardware_id_;
        std::vector<input_event> pending_events_;
        timestamp_t last_update_timestamp_ = 0;
        int file_desc_ = -1;
        #ifdef CROSSPUT_FEATURE_FORCE
        std::unordered_map<int16_t, LinuxForce *> force_mapping;
        std::bitset<NUM_FORCE_TYPES> supported_forces_;
        float gain_ = 0.0F;
        unsigned short force_count_noorph_ = 0;
        bool supports_gain_ = false;
        bool supports_autocenter_ = false;
        #endif // CROSSPUT_FEATURE_FORCE

    public:
        constexpr const LinuxHardwareID &GetHardwareID() { return hardware_id_; }
        constexpr bool IsAggregate() const noexcept override final { return false; }

        std::string GetDisplayName() const override final;
        void Update() override final;

        #ifdef CROSSPUT_FEATURE_FORCE
        constexpr uint32_t GetMotorCount() const override final;
        constexpr float GetGain(const uint32_t motor_index) const override final;
        void SetGain(const uint32_t motor_index, float gain) override final;
        constexpr bool SupportsForce(const uint32_t motor_index, const ForceType type) const override final;
        bool TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force) override final;
        #endif // CROSSPUT_FEATURE_FORCE

        virtual ~LinuxDevice()
        {
            nat_device_ids.erase(hardware_id_);
            CloseDevFile();
        }

    protected:
        LinuxDevice(const LinuxHardwareID &hardware_id) :
            hardware_id_(hardware_id)
        {
            pending_events_.reserve(16);
            #ifdef CROSSPUT_FEATURE_FORCE
            force_mapping.reserve(16);
            #endif // CROSSPUT_FEATURE_FORCE
        }

        virtual constexpr void PreInputHandling() {}

        // device-specific event handler implementation that uses the vector of stored events
        // note: vector is only cleared on SYN_DROPPED, so its contents are entirely managed by the implementation
        virtual void HandlePendingEvents() = 0;

        // device-specific buffer overrun implementation that is invoked when SYN_DROPPED is read
        virtual void HandleBufferOverrun(const timestamp_t timestamp) = 0;

        void CloseDevFile();
        bool TryConnect();
        void Disconnect();

        constexpr virtual void OnConnected() {}
        constexpr virtual void OnDisconnected() {}

        #ifdef CROSSPUT_FEATURE_FORCE
        void HandleFFStatusEvent(const input_event &ev);

        inline bool SetGainImpl(const float gain)
        {
            gain_ = gain;
            if (supports_gain_)
            {
                input_event ev
                {
                    .type = EV_FF,
                    .code = FF_GAIN,
                    .value = static_cast<int>(gain * static_cast<float>(0xFFFF))
                };

                return 0 <= write(file_desc_, &ev, sizeof(input_event));
            }

            return false;
        }

        inline bool DisableAutocenterImpl()
        {
            if (supports_autocenter_)
            {
                input_event ev
                {
                    .type = EV_FF,
                    .code = FF_AUTOCENTER,
                    .value = 0
                };

                return 0 <= write(file_desc_, &ev, sizeof(input_event));
            }

            return false;
        }
        #endif // CROSSPUT_FEATURE_FORCE
    };


    class LinuxMouse final :
        public virtual IMouse,
        public virtual LinuxDevice,
        public virtual TypedInterface<DeviceType::MOUSE>,
        public virtual MouseCallbackManager
    {
    public:
        static constexpr size_t NUM_BUTTONS = 8;

    private:
        TSTV button_data_[NUM_BUTTONS] = {};
        MouseData data_ = {};

    public:
        LinuxMouse(const LinuxHardwareID &hardware_id) : LinuxDevice(hardware_id) {}

        constexpr void GetPosition(int64_t &x, int64_t &y) const override;
        constexpr void GetDelta(int64_t &x, int64_t &y) const override;
        constexpr void GetScroll(int64_t &x, int64_t &y) const override;
        constexpr void GetScrollDelta(int64_t &x, int64_t &y) const override;
        constexpr uint32_t GetButtonCount() const override;
        constexpr void SetButtonThreshold(const uint32_t index, float threshold) override;
        constexpr void SetGlobalThreshold(float threshold) override;
        constexpr float GetButtonThreshold(const uint32_t index) const override;
        constexpr float GetButtonValue(const uint32_t index) const override;
        constexpr bool GetButtonState(const uint32_t index, float &time) const override;

    private:
        void PreInputHandling() override;
        void HandlePendingEvents() override;
        void HandleBufferOverrun(const timestamp_t timestamp) override;
        void OnDisconnected() override;
    };


    class LinuxKeyboard final :
        public virtual IKeyboard,
        public virtual LinuxDevice,
        public virtual TypedInterface<DeviceType::KEYBOARD>,
        public virtual KeyboardCallbackManager
    {
    private:
        TSTV key_data_[NUM_KEY_CODES] = {};
        uint32_t num_keys_pressed_ = 0;

    public:
        LinuxKeyboard(const LinuxHardwareID &hardware_id) : LinuxDevice(hardware_id) {}

        constexpr uint32_t GetNumKeysPressed() const override;
        constexpr void SetKeyThreshold(const Key key, float threshold) override;
        constexpr void SetGlobalThreshold(float threshold) override;
        constexpr float GetKeyThreshold(const Key key) const override;
        constexpr float GetKeyValue(const Key key) const override;
        constexpr bool GetKeyState(const Key key, float &time) const override;

    private:
        void HandlePendingEvents() override;
        void HandleBufferOverrun(const timestamp_t timestamp) override;
        void OnDisconnected() override;
    };


    class LinuxGamepad final :
        public virtual IGamepad,
        public virtual LinuxDevice,
        public virtual TypedInterface<DeviceType::GAMEPAD>,
        public virtual GamepadCallbackManager
    {
    public:
        static constexpr size_t NUM_TRIGGERS = 4;
        static constexpr size_t NUM_THUMBSTICKS = 2;

    private:
        TSTV button_data_[NUM_BUTTON_CODES] = {};
        AbsValNorm thumbstick_norms_[NUM_THUMBSTICKS * 2] = {};
        AbsValNorm trigger_norms_[NUM_TRIGGERS] = {};
        float thumbstick_values_[NUM_THUMBSTICKS * 2] = {};
        std::pair<AbsValNorm, AbsValNorm> dpad_norm_ = {};

        // mapping between button -> analog normalizers
        // (for determining whether a button is already recieving analog events)
        const AbsValNorm *button_to_normalizer_[NUM_BUTTON_CODES] = {};
        

    public:
        LinuxGamepad(const LinuxHardwareID &hardware_id) : LinuxDevice(hardware_id) {}

        constexpr void SetButtonThreshold(const Button button, float threshold) override;
        constexpr void SetGlobalThreshold(float threshold) override;
        constexpr float GetButtonThreshold(const Button button) const override;
        constexpr float GetButtonValue(const Button button) const override;
        constexpr bool GetButtonState(const Button button, float &time) const override;
        constexpr uint32_t GetThumbstickCount() const override;
        constexpr void GetThumbstick(const uint32_t index, float &x, float &y) const override;

    private:
        void HandlePendingEvents() override;
        void HandleBufferOverrun(const timestamp_t timestamp) override;
        void OnConnected() override;
        void OnDisconnected() override;
        
        inline void DigitalizeAnalogDpad(const float nvalue, const timestamp_t timestamp, const Button b1, const Button b2)
        {
            const float value1 = std::max(0.0F, nvalue);
            const float value2 = std::max(0.0F, -nvalue);
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state1, state2;
            if (button_data_[static_cast<int>(b1)].Modify(value1, timestamp, state1)) { ButtonChanged(b1, value1, state1); }
            if (button_data_[static_cast<int>(b2)].Modify(value2, timestamp, state2)) { ButtonChanged(b2, value2, state2); }
            #else
            button_data_[static_cast<int>(b1)].Modify(value1, timestamp);
            button_data_[static_cast<int>(b2)].Modify(value2, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    };


    #ifdef CROSSPUT_FEATURE_FORCE
    class LinuxForce final : BaseForce<LinuxDevice>
    {
        friend LinuxDevice;

    private:
        const int16_t effect_id_;
        ForceStatus status_ = ForceStatus::UNKNOWN;

    public:
        LinuxForce(LinuxDevice *const p_device, const ForceType type, const int16_t effect_id) :
            BaseForce(p_device, 0, type),
            effect_id_(effect_id)
        {
            assert(p_device != nullptr);
        }

        constexpr ForceStatus GetStatus() const override { return status_; }
        void SetActive(const bool active) override;
        bool WriteParams() override;

        ~LinuxForce()
        {
            if (!IsOrphaned())
            {
                p_device_->force_mapping.erase(effect_id_);
                p_device_->force_count_noorph_--;
                ioctl(p_device_->file_desc_, EVIOCRMFF, effect_id_);
            }
        }
    };
    #endif // CROSSPUT_FEATURE_FORCE


    /*
    // Implementation of my Gist https://gist.github.com/TriceHelix/de47ed38dcb4f7216b26291c47445d99
    // This is less performant and potentially more error-prone than ioctl,
    // however it seems to be the only way to access EV_SYN capabilities.
    // Value 0 reports EV capabilities, which is also the value of EV_SYN.
    void ReadInputDeviceCapabilities(const unsigned int event_x, const char *filename, unsigned char *bits, const size_t len)
    {
        std::ifstream file(std::format("/sys/class/input/event{}/device/capabilities/{}", event_x, filename));

        if (!file.is_open())
        {
            // some kind of file error
            std::memset(bits, 0, len);
            return;
        }

        size_t buflen = std::max(static_cast<size_t>(64), (len * 2) + (len / 16) + 2);
        std::unique_ptr<char[]> buffer = std::unique_ptr<char[]>(new char[buflen]);

        size_t rd = 0;
        do
        {
            if (rd >= buflen)
            {
                // resize character buffer
                const size_t buflen_old = buflen;
                buflen *= 2;
                char *const p_new = new char[buflen];
                std::memcpy(p_new, buffer.get(), buflen_old);
                buffer.reset(p_new);
            }
            rd += file.readsome(buffer.get() + rd, buflen - rd);
        }
        while (!(file.eof() || file.bad()));

        if (rd == 0)
        {
            // file is empty
            std::memset(bits, 0, len);
            return;
        }

        // read characters in reverse
        unsigned int hexoff = 0U;
        bool hit_space = false;
        for (const char *p_char = buffer.get() + rd; p_char >= buffer.get(); )
        {
            const char c = std::tolower(*(p_char--));
            
            if (c == ' ')
            {
                if (!hit_space) // ignore consecutive spaces
                {
                    // spaces separate value regions of 64 bits (16 hex characters), regardless of whether or not all of the bits were explicitly encoded into hex
                    // (example: gamepad key capabilities often look like "xxxxx 0 0 0 0" because gamepads usually do not send KEY_<...> values that are between 0 and 255,
                    // thus the 256 false bits are replaced with four zeros separated by spaces that create the 4*64=256 bit offset for the x region)
                    hexoff = std::max(0U, ((hexoff - 1U) / 16U + 1U) * 16U);
                    hit_space = true;
                }

                continue;
            }

            // try to decode hex
            unsigned char bin;
            if ('0' <= c && c <= '9') { bin = c - '0'; }
            else if ('a' <= c && c <= 'f') { bin = 10 + (c - 'a'); }
            else { continue; } // not a hex character

            // store bits
            bits[hexoff / 2U] |= (bin << (hexoff & 1U));

            hexoff++;
            hit_space = false;
        }
    }
    */


    // returns the most likely device type of the target event device
    DeviceType DeduceInputDeviceType(const int fd)
    {
        int proof_mouse = 0, proof_keyboard = 0, proof_gamepad = 0;

        // capability bitfields
        unsigned char ev_capabilities[(EV_CNT - 1) / 8 + 1] = {};
        unsigned char key_capabilities[(KEY_CNT - 1) / 8 + 1] = {};

        // query capabilities
        ioctl(fd, EVIOCGBIT(0, sizeof(ev_capabilities)), ev_capabilities);
        ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_capabilities)), key_capabilities);

        // analyze event capabilities
        {
            if (!(AnyBit(ev_capabilities, sizeof(ev_capabilities)) && GETBIT_(ev_capabilities, EV_SYN)))
            {
                // must send EV_SYN events (SYN_REPORT is used to separate packets)
                return DeviceType::UNKNOWN;
            }

            if (GETBIT_(ev_capabilities, EV_REL))
            {
                // only mice generate relative events
                proof_mouse++;
                proof_keyboard--;
                proof_gamepad--;
            }

            if (GETBIT_(ev_capabilities, EV_ABS))
            {
                // only gamepads generate absolute events
                // note: ignoring the possibility of a touchscreen because they are checked via the BTN_TOUCH capability
                proof_gamepad++;
                proof_mouse--;
                proof_keyboard--;
            }

            if (GETBIT_(ev_capabilities, EV_FF))
            {
                // most gamepads support force-feedback
                proof_gamepad++;
            }
        }

        // analyze key capabilities
        {
            // count the number of matching keycode capabilities for each device type
            int kccount_mouse = 0, kccount_keyboard = 0, kccount_gamepad = 0;
            
            for (unsigned int k = 0; k < NUM_KEY_CODES; k++)
            {
                const unsigned short code = reverse_keycode_mapping[k];
                kccount_keyboard += GETBIT_(key_capabilities, code);
            }

            for (unsigned int b = 0; b < NUM_BUTTON_CODES; b++)
            {
                const unsigned short code = reverse_button_mapping[b];
                kccount_gamepad += GETBIT_(key_capabilities, code);
            }

            for (unsigned int mb = 0; mb < DEVRECOG_MOUSE_LEN; mb++)
            {
                const unsigned short code = devrecog_mouse[mb];
                kccount_mouse += GETBIT_(key_capabilities, code);
            }

            // blacklist (strongly discouraged)
            for (unsigned int av = 0; av < DEVRECOG_AVOID_LEN; av++)
            {
                const unsigned short code = devrecog_avoid[av];
                if (GETBIT_(key_capabilities, code))
                {
                    proof_mouse--;
                    proof_keyboard--;
                    proof_gamepad--;
                }
            }

            if (kccount_mouse > 0 && (kccount_keyboard + kccount_gamepad) <= 0)
            {
                // device sends mouse-related keycodes
                proof_mouse += 2;
                proof_keyboard--;
                proof_gamepad--;
            }
            else if (kccount_keyboard > 0 && kccount_gamepad <= 0)
            {
                // device sends keyboard-related keycodes
                // (giving extra proof to keybooard because they barely have any other proof)
                proof_keyboard += 3;
                proof_mouse--;
                proof_gamepad--;
            }
            else if (kccount_gamepad > 0)
            {
                // device sends gamepad related keycodes
                proof_gamepad += 2;
                proof_mouse--;
                proof_keyboard--;
            }
        }

        DeviceType deduced_type;
        int deduced_proof;
        if (proof_mouse > proof_keyboard && proof_mouse > proof_gamepad)
        {
            // device is most likely a mouse
            deduced_type = DeviceType::MOUSE;
            deduced_proof = proof_mouse;
        }
        else if (proof_keyboard > proof_gamepad)
        {
            // device is most likely a keyboard
            deduced_type = DeviceType::KEYBOARD;
            deduced_proof = proof_keyboard;
        }
        else
        {
            // device is most likely a gamepad
            deduced_type = DeviceType::GAMEPAD;
            deduced_proof = proof_gamepad;
        }

        return deduced_proof > 1 ? deduced_type : DeviceType::UNKNOWN;
    }


    // invoke function-like object for every /dev/input/eventX file,
    // returns number of invocations
    // [handler signature: bool (const int file_descriptor, const unsigned int x, bool &close_fd)]
    size_t ForeachEventXFile(auto &&handler, const int fd_flags)
    {
        size_t num = 0;

        for (const auto &entry : std::filesystem::directory_iterator(DEV_INPUT_DIR))
        {
            if (entry.is_directory()) { continue; }

            const std::string fnstr = entry.path().filename().string();
            if (!std::regex_match(fnstr, REGEX_EVENTX_FN)) { continue; } // not an eventX file

            const int fd = open(entry.path().c_str(), fd_flags);
            if (fd < 0)
            {
                if (errno == EPERM)
                {
                    throw std::runtime_error(std::format("Access to \"{}\" denied. Is the current user in the \"input\" group?", entry.path().c_str()));
                }

                continue;
            }

            bool close_fd;
            char *end = nullptr;
            const unsigned int x = static_cast<unsigned int>(std::strtoul(fnstr.data() + 5, &end, 10)); // parse eventX
            const bool status = handler(fd, x, close_fd);
            num++;

            if (close_fd) { close(fd); }
            if (!status) { break; }
        }

        return num;
    }


    #ifdef CROSSPUT_FEATURE_FORCE
    constexpr int16_t TranslateMagnitude(const float magnitude)
    {
        constexpr float M_LIMIT = static_cast<float>(std::numeric_limits<int16_t>::max()) / 1E3F;
        return static_cast<int16_t>(std::clamp(magnitude * 1E3F, -M_LIMIT, M_LIMIT));
    }


    constexpr ff_envelope TranslateEnvelope(const ForceEnvelope &envelope, uint16_t &duration)
    {
        // when the sum of times exceeds MAX_TIME, all times are scaled back into the valid range
        const float m = 1.0F / std::max(1.0F, (std::max(0.0F, envelope.attack_time) + std::max(0.0F, envelope.sustain_time) + std::max(0.0F, envelope.release_time)) * (1.0F / ForceEnvelope::MAX_TIME));
        duration = static_cast<uint16_t>(envelope.sustain_time * m * 1E3F);

        return ff_envelope
        {
            .attack_length = static_cast<uint16_t>(envelope.attack_time * m * 1E3F),
            .attack_level = static_cast<uint16_t>(std::clamp(envelope.attack_gain, 0.0F, 1.0F) * static_cast<float>(0x7fff)),
            .fade_length = static_cast<uint16_t>(envelope.release_time * m * 1E3F),
            .fade_level = static_cast<uint16_t>(std::clamp(envelope.release_gain, 0.0F, 1.0F) * static_cast<float>(0x7fff))
        };
    }


    constexpr ff_rumble_effect TranslateRumbleParams(const RumbleForceParams &params)
    {
        return ff_rumble_effect
        {
            .strong_magnitude = static_cast<uint16_t>(params.low_frequency * static_cast<float>(0xFFFF)),
            .weak_magnitude = static_cast<uint16_t>(params.high_frequency * static_cast<float>(0xFFFF))
        };
    }


    constexpr ff_constant_effect TranslateConstantParams(const ConstantForceParams &params, uint16_t &duration)
    {
        return ff_constant_effect
        {
            .level = TranslateMagnitude(params.magnitude),
            .envelope = TranslateEnvelope(params.envelope, duration)
        };
    }


    constexpr ff_ramp_effect TranslateRampParams(const RampForceParams &params, uint16_t &duration)
    {
        return ff_ramp_effect
        {
            .start_level = TranslateMagnitude(params.magnitude_start),
            .end_level = TranslateMagnitude(params.magnitude_end),
            .envelope = TranslateEnvelope(params.envelope, duration)
        };
    }


    constexpr ff_periodic_effect TranslatePeriodicParams(const uint16_t waveform, const PeriodicForceParams &params, uint16_t &duration)
    {
        (void)TranslateEnvelope(params.envelope, duration);
        return ff_periodic_effect
        {
            .waveform = waveform,
            .period = std::max(static_cast<uint16_t>(1), static_cast<uint16_t>(std::min(1E3F / params.frequency, static_cast<float>(0xFFFF)))),
            .magnitude = TranslateMagnitude(params.magnitude),
            .offset = TranslateMagnitude(params.offset),
            .phase = static_cast<uint16_t>(std::clamp(params.phase, 0.0F, 1.0F) * static_cast<float>(0xFFFF)),
            .custom_len = 0,
            .custom_data = nullptr
        };
    }


    constexpr ff_condition_effect TranslateConditionParams(const ConditionForceParams &params)
    {
        return ff_condition_effect
        {
            .right_saturation = static_cast<uint16_t>(std::max(static_cast<int16_t>(0), static_cast<int16_t>(TranslateMagnitude(params.right_saturation)))),
            .left_saturation = static_cast<uint16_t>(std::max(static_cast<int16_t>(0), static_cast<int16_t>(TranslateMagnitude(params.left_saturation)))),
            .right_coeff = static_cast<int16_t>(std::clamp(params.right_coefficient, -1.0F, 1.0F) * static_cast<float>(0x7FFF)),
            .left_coeff = static_cast<int16_t>(std::clamp(params.left_coefficient, -1.0F, 1.0F) * static_cast<float>(0x7FFF)),
            .deadband = static_cast<uint16_t>(std::clamp(params.deadzone, 0.0F, 1.0F) * static_cast<float>(0xFFFF)),
            .center = static_cast<int16_t>(std::clamp(params.center, -1.0F, 1.0F) * static_cast<float>(0x7FFF))
        };
    }


    constexpr ff_effect TranslateForceParams(const ForceParams &params)
    {
        const int type_index = static_cast<int>(params.type);

        ff_effect effect;
        effect.type = effect_type_mapping[type_index];
        effect.direction = 0;
        effect.trigger = {};
        effect.replay = {.length = static_cast<uint16_t>(ForceEnvelope::MAX_TIME * 1E3F), .delay = 0};

        switch (effect.type)
        {
        case FF_RUMBLE: effect.u.rumble = TranslateRumbleParams(params.rumble); break;
        case FF_CONSTANT: effect.u.constant = TranslateConstantParams(params.constant, effect.replay.length); break;
        case FF_RAMP: effect.u.ramp = TranslateRampParams(params.ramp, effect.replay.length); break;
        case FF_PERIODIC: effect.u.periodic = TranslatePeriodicParams(periodic_waveform_mapping[type_index], params.periodic, effect.replay.length); break;

        default:
            // assume it's a condition effect
            assert(IsConditionForceType(params.type));
            ff_condition_effect cond_effect = TranslateConditionParams(params.condition);
            effect.u.condition[0] = cond_effect;
            effect.u.condition[1] = cond_effect;
            break;
        }

        return effect;
    }
    #endif // CROSSPUT_FEATURE_FORCE


    // GLOBAL FUNCTION IMPLEMENTATIONS

    size_t DiscoverDevices()
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        size_t devices_created = 0;
        const auto dev_analyzer = [&devices_created](const int fd, const unsigned int x, bool &close_fd) -> bool
        {
            close_fd = true;

            // check if hardware is already registered
            LinuxHardwareID hwid;
            GetHWID(hwid, fd, x);
            if (nat_device_ids.contains(hwid)) { return true; }

            LinuxDevice *p_vdev;
            switch (DeduceInputDeviceType(fd))
            {
            case DeviceType::MOUSE: p_vdev = new LinuxMouse(hwid); break;
            case DeviceType::KEYBOARD: p_vdev = new LinuxKeyboard(hwid); break;
            case DeviceType::GAMEPAD: p_vdev = new LinuxGamepad(hwid); break;
            default: return true; // type of input produced by device not recognizable
            }

            nat_device_ids.insert(hwid);
            glob_interfaces.insert({p_vdev->GetID(), p_vdev});

            #ifdef CROSSPUT_FEATURE_CALLBACK
            DeviceStatusChanged(p_vdev, DeviceStatusChange::DISCOVERED);
            #endif // CROSSPUT_FEATURE_CALLBACK

            return true;
        };

        ForeachEventXFile(dev_analyzer, O_RDONLY | O_NONBLOCK);

        return devices_created;
    }


    // LINUX DEVICE
    std::string LinuxDevice::GetDisplayName() const
    {
        // use ioctl to query name
        char name[256] = {};
        return (is_connected_ && ioctl(file_desc_, EVIOCGNAME(sizeof(name) - 1), &name) >= 0)
            ? std::string(name)
            : std::string();
    }


    void LinuxDevice::Update()
    {
        ProtectManagementAPI(std::format("crossput::IDevice::Update() - Device ID {}", id_));
        
        if (!(is_connected_ || TryConnect())) { return; }

        last_update_timestamp_ = GetTimestampNow();
        PreInputHandling();

        ssize_t stat;
        input_event ev;
        while ((stat = read(file_desc_, &ev, sizeof(input_event))) > 0)
        {
            switch (ev.type)
            {
            case EV_SYN:
                if (ev.code == SYN_DROPPED)
                {
                    // buffer overrun, drop events
                    pending_events_.clear();
                    fsync(file_desc_);
                    HandleBufferOverrun(GetEventTimestamp(ev));
                    break;
                }

                if (ev.code == SYN_REPORT)
                {
                    // process a group of events (device-specific implementation)
                    last_update_timestamp_ = std::max(last_update_timestamp_, GetEventTimestamp(ev));
                    HandlePendingEvents();
                }

                if (!is_connected_)
                {
                    // error in event handler caused disconnect, abort
                    return;
                }

                break;

            #ifdef CROSSPUT_FEATURE_FORCE
            case EV_FF_STATUS:
                HandleFFStatusEvent(ev);
                break;
            #endif // CROSSPUT_FEATURE_FORCE
                
            default:
                // device implementation handles event
                pending_events_.push_back(ev);
                break;
            }
        }

        if (stat < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // error during event reading
            Disconnect();
            return;
        }
    }


    bool LinuxDevice::TryConnect()
    {
        if (is_connected_) [[unlikely]] { return true; }

        unsigned int event_x;
        const auto analyze_device = [this, &event_x](const int fd, const unsigned int x, bool &close_fd) -> bool
        {
            close_fd = true;

            LinuxHardwareID hwid;
            GetHWID(hwid, fd, x);

            // hardware ID and device type must match
            if (this->hardware_id_ == hwid && DeduceInputDeviceType(fd) == this->GetType())
            {
                // set timestamp clock ID
                int clockid = CROSSPUT_TSCLOCKID;
                if (ioctl(fd, EVIOCSCLOCKID, &clockid) >= 0)
                {
                    close_fd = false;
                    event_x = x;
                    this->file_desc_ = fd;
                    this->is_connected_ = true;
                    return false; // stop searching for devices
                }
            }

            return true;
        };

        ForeachEventXFile(analyze_device, O_RDWR | O_NONBLOCK);

        if (is_connected_)
        {
            #ifdef CROSSPUT_FEATURE_FORCE
            // query force capabilities
            unsigned char ff_capabilities[(FF_CNT - 1) / 8 + 1] = {};
            ioctl(file_desc_, EVIOCGBIT(EV_FF, sizeof(ff_capabilities)), ff_capabilities);

            #define QUERY_FF_CAPABILITY_(enum_name, code) supported_forces_[static_cast<int>(ForceType::enum_name)] = GETBIT_(ff_capabilities, code)
            #define ENABLE_FF_CAPABILITY_(enum_name) supported_forces_[static_cast<int>(ForceType::enum_name)] = true

            // 1-1 mapping
            QUERY_FF_CAPABILITY_(RUMBLE, FF_RUMBLE);
            QUERY_FF_CAPABILITY_(CONSTANT, FF_RUMBLE);
            QUERY_FF_CAPABILITY_(RAMP, FF_RAMP);
            QUERY_FF_CAPABILITY_(SPRING, FF_SPRING);
            QUERY_FF_CAPABILITY_(FRICTION, FF_FRICTION);
            QUERY_FF_CAPABILITY_(DAMPER, FF_DAMPER);
            QUERY_FF_CAPABILITY_(INERTIA, FF_INERTIA);

            // FF_PERIODIC corresponds to all waveforms
            if (GETBIT_(ff_capabilities, FF_PERIODIC))
            {
                ENABLE_FF_CAPABILITY_(SINE);
                ENABLE_FF_CAPABILITY_(TRIANGLE);
                ENABLE_FF_CAPABILITY_(SQUARE);
                ENABLE_FF_CAPABILITY_(SAW_UP);
                ENABLE_FF_CAPABILITY_(SAW_DOWN);
            }
            #undef ENABLE_FF_CAPABILITY_
            #undef QUERY_FF_CAPABILITY_

            supports_gain_ = GETBIT_(ff_capabilities, FF_GAIN);
            supports_autocenter_ = GETBIT_(ff_capabilities, FF_AUTOCENTER);

            // apply default force-feedback parameters
            (void)SetGainImpl(1.0F);
            (void)DisableAutocenterImpl();
            #endif // CROSSPUT_FEATURE_FORCE

            OnConnected();

            #ifdef CROSSPUT_FEATURE_CALLBACK
            DeviceStatusChanged(this, DeviceStatusChange::CONNECTED);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        return is_connected_;
    }


    void LinuxDevice::Disconnect()
    {
        if (!is_connected_) [[unlikely]] { return; }

        is_connected_ = false;
        last_update_timestamp_ = 0;
        pending_events_.clear();

        #ifdef CROSSPUT_FEATURE_FORCE
        supported_forces_.reset();
        gain_ = 0.0F;
        force_count_noorph_ = 0;
        supports_gain_ = false;
        supports_autocenter_ = false;
        #endif // CROSSPUT_FEATURE_FORCE

        // forces are orphaned in CloseDevFile()
        CloseDevFile();
        OnDisconnected();

        #ifdef CROSSPUT_FEATURE_CALLBACK
        DeviceStatusChanged(this, DeviceStatusChange::DISCONNECTED);
        #endif // CROSSPUT_FEATURE_CALLBACK
    }


    void LinuxDevice::CloseDevFile()
    {
        if (file_desc_ < 0) { return; }
        
        #ifdef CROSSPUT_FEATURE_FORCE
        // orphan forces
        for (auto [effect_id, p_force] : force_mapping)
        {
            p_force->p_device_ = nullptr;
            p_force->status_ = ForceStatus::INACTIVE;
            ioctl(file_desc_, EVIOCRMFF, effect_id);
        }
        #endif // CROSSPUT_FEATURE_FORCE

        close(file_desc_);
        file_desc_ = -1;
    }


    #ifdef CROSSPUT_FEATURE_FORCE
    void LinuxDevice::HandleFFStatusEvent(const input_event &ev)
    {
        assert(ev.type == EV_FF_STATUS);

        const auto it = force_mapping.find(ev.code);
        if (it != force_mapping.end())
        {
            ForceStatus &stat = it->second->status_;
            switch (ev.value)
            {
            case FF_STATUS_STOPPED: stat = ForceStatus::INACTIVE; break;
            case FF_STATUS_PLAYING: stat = ForceStatus::ACTIVE; break;
            default: stat = ForceStatus::UNKNOWN; break;
            }
        }
    }


    constexpr uint32_t LinuxDevice::GetMotorCount() const
    {
        return (is_connected_ && supported_forces_.any()) ? 1 : 0;
    }


    constexpr float LinuxDevice::GetGain(const uint32_t motor_index) const
    {
        return (is_connected_ && motor_index == 0) ? gain_ : 0.0F;
    }


    void LinuxDevice::SetGain([[maybe_unused]] const uint32_t motor_index, float gain)
    {
        if (is_connected_)
        {
            (void)SetGainImpl(std::clamp(gain, 0.0F, 1.0F));
        }
    }


    constexpr bool LinuxDevice::SupportsForce(const uint32_t motor_index, const ForceType type) const
    {
        return is_connected_
            && motor_index == 0
            && supported_forces_[static_cast<int>(type)];
    }


    bool LinuxDevice::TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force)
    {
        const int type_index = static_cast<int>(type);
        if (SupportsForce(motor_index, type) && force_count_noorph_ < FF_MAX_EFFECTS)
        {
            // create effect
            ff_effect effect = {.type = effect_type_mapping[type_index], .id = -1};
            if (ioctl(file_desc_, EVIOCSFF, &effect) >= 0 && effect.id >= 0)
            {
                // effect allocation successful, now create interface
                LinuxForce *const p_lxforce = new LinuxForce(this, type, effect.id);
                forces_.insert({p_lxforce->GetID(), p_lxforce});
                force_mapping.insert({effect.id, p_lxforce});
                p_force = p_lxforce;
                force_count_noorph_++;
                return true;
            }
        }

        p_force = nullptr;
        return false;
    }
    #endif // CROSSPUT_FEATURE_FORCE


    // LINUX MOUSE
    void LinuxMouse::PreInputHandling()
    {
        // reset deltas
        data_.dx = 0;
        data_.dy = 0;
        data_.sdx = 0;
        data_.sdy = 0;
    }


    void LinuxMouse::HandlePendingEvents()
    {
        // total position and scroll change within currently pending events
        // (SYN_REPORT will likely separate movement and scroll)
        int64_t dx = 0, dy = 0, sdx = 0, sdy = 0, hrsdx = 0, hrsdy = 0;

        for (const input_event &ev : pending_events_)
        {
            switch (ev.type)
            {
            case EV_REL:
                // accumulate cursor and wheel movement
                switch (ev.code)
                {
                case REL_X: dx += ev.value; break;
                case REL_Y: dy += ev.value; break;
                case REL_HWHEEL: sdx += ev.value; break;
                case REL_WHEEL: sdy += ev.value; break;
                case REL_HWHEEL_HI_RES: hrsdx += ev.value; break;
                case REL_WHEEL_HI_RES: hrsdy += ev.value; break;
                }
                break;

            case EV_KEY:
                if (BTN_LEFT <= ev.code && ev.code <= BTN_TASK)
                {
                    const unsigned short mb = ev.code - BTN_LEFT;
                    const float value = ev.value != 0 ? 1.0F : 0.0F;
                    #ifdef CROSSPUT_FEATURE_CALLBACK
                    bool state;
                    if (button_data_[mb].Modify(value, GetEventTimestamp(ev), state)) { ButtonChanged(static_cast<uint32_t>(mb), value, state); }
                    #else
                    button_data_[mb].Modify(value, GetEventTimestamp(ev));
                    #endif // CROSSPUT_FEATURE_CALLBACK
                }
                break;

            #ifdef CROSSPUT_FEATURE_FORCE
            case EV_FF_STATUS:
                HandleFFStatusEvent(ev);
                break;
            #endif // CROSSPUT_FEATURE_FORCE
            }
        }

        pending_events_.clear();

        // accumulate movement
        if (dx != 0 || dy != 0)
        {
            data_.x += dx;
            data_.y += dy;
            data_.dx += dx;
            data_.dy += dy;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            MouseMoved(data_.x, data_.y, dx, dy);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        // accumulate scroll
        if (hrsdx != 0 || hrsdy != 0)
        {
            // use high-res scroll
            data_.sx += hrsdx;
            data_.sy += hrsdy;
            data_.sdx += hrsdx;
            data_.sdy += hrsdy;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            MouseScrolled(data_.sx, data_.sy, hrsdx, hrsdy);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
        else if (sdx != 0 || sdy != 0)
        {
            // use low-res scroll
            const int64_t sdx_120 = sdx * 120;
            const int64_t sdy_120 = sdy * 120;
            data_.sx += sdx_120;
            data_.sy += sdy_120;
            data_.sdx += sdx_120;
            data_.sdy += sdy_120;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            MouseScrolled(data_.sx, data_.sy, sdx_120, sdy_120);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void LinuxMouse::HandleBufferOverrun(const timestamp_t timestamp)
    {
        unsigned char globks[(KEY_CNT - 1) / 8 + 1];
        if (ioctl(file_desc_, EVIOCGKEY(sizeof(globks)), &globks) < 0)
        {
            // cannot query global key states
            Disconnect();
            return;
        }

        // update mouse button states based on global query
        for (unsigned int mb = 0; mb < LinuxMouse::NUM_BUTTONS; mb++)
        {
            const float value = GETBIT_(globks, BTN_LEFT + mb) ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (button_data_[mb].Modify(value, timestamp, state)) { ButtonChanged(static_cast<uint32_t>(mb), value, state); }
            #else
            button_data_[mb].Modify(value, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void LinuxMouse::OnDisconnected()
    {
        std::memset(button_data_, 0, sizeof(button_data_));
        data_ = {};
    }


    constexpr void LinuxMouse::GetPosition(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.x;
        y = data_.y;
    }

    constexpr void LinuxMouse::GetDelta(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.dx;
        y = data_.dy;
    }

    constexpr void LinuxMouse::GetScroll(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.sx;
        y = data_.sy;
    }

    constexpr void LinuxMouse::GetScrollDelta(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.sdx;
        y = data_.sdy;
    }

    constexpr uint32_t LinuxMouse::GetButtonCount() const
    {
        return is_connected_ ? LinuxMouse::NUM_BUTTONS : 0;
    }

    constexpr void LinuxMouse::SetButtonThreshold(const uint32_t index, float threshold)
    {
        if (index < LinuxMouse::NUM_BUTTONS)
        {
            button_data_[index].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void LinuxMouse::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < LinuxMouse::NUM_BUTTONS; i++) { button_data_[i].SetThreshold(threshold); }
    }

    constexpr float LinuxMouse::GetButtonThreshold(const uint32_t index) const
    {
        return index < LinuxMouse::NUM_BUTTONS
            ? button_data_[index].Threshold()
            : 0.0F;
    }

    constexpr float LinuxMouse::GetButtonValue(const uint32_t index) const
    {
        return (is_connected_ && index < LinuxMouse::NUM_BUTTONS)
            ? button_data_[index].Value()
            : 0.0F;
    }

    constexpr bool LinuxMouse::GetButtonState(const uint32_t index, float &time) const
    {
        if (is_connected_ && index < LinuxMouse::NUM_BUTTONS)
        {
            const TSTV &d = button_data_[index];
            time = TimestampDeltaSeconds(d.Timestamp(), last_update_timestamp_);
            return d.State();
        }
        else
        {
            time = std::numeric_limits<float>::infinity();
            return false;
        }
    }


    // LINUX KEYBOARD
    void LinuxKeyboard::HandlePendingEvents()
    {
        for (const input_event &ev : pending_events_)
        {
            if (ev.type == EV_KEY)
            {
                static_assert(!std::numeric_limits<decltype(ev.code)>::is_signed);
                if (KEYCODE_MAPPING_LEN < ev.code) [[unlikely]] { continue; }

                const Key key = keycode_mapping[ev.code];
                if (IsValidKey(key))
                {
                    const float value = ev.value != 0 ? 1.0F : 0.0F;
                    #ifdef CROSSPUT_FEATURE_CALLBACK
                    bool state;
                    if (key_data_[static_cast<int>(key)].Modify(value, GetEventTimestamp(ev), state, num_keys_pressed_)) { KeyChanged(key, value, state); }
                    #else
                    key_data_[static_cast<int>(key)].Modify(value, GetEventTimestamp(ev), num_keys_pressed_);
                    #endif // CROSSPUT_FEATURE_CALLBACK
                }
            }
            #ifdef CROSSPUT_FEATURE_FORCE
            else if (ev.type == EV_FF_STATUS)
            {
                HandleFFStatusEvent(ev);
            }
            #endif // CROSSPUT_FEATURE_FORCE
        }

        pending_events_.clear();
    }


    void LinuxKeyboard::HandleBufferOverrun(const timestamp_t timestamp)
    {
        unsigned char globks[(KEY_CNT - 1) / 8 + 1];
        if (ioctl(file_desc_, EVIOCGKEY(sizeof(globks)), &globks) < 0)
        {
            // cannot query global key states
            Disconnect();
            return;
        }

        // update key states based on global query
        for (unsigned int k = 0; k < NUM_KEY_CODES; k++)
        {
            const float value = GETBIT_(globks, reverse_keycode_mapping[k]) ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (key_data_[k].Modify(value, timestamp, state, num_keys_pressed_)) { KeyChanged(static_cast<Key>(k), value, state); }
            #else
            key_data_[k].Modify(value, timestamp, num_keys_pressed_);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void LinuxKeyboard::OnDisconnected()
    {
        std::memset(key_data_, 0, sizeof(key_data_));
        num_keys_pressed_ = 0;
    }


    constexpr uint32_t LinuxKeyboard::GetNumKeysPressed() const
    {
        return is_connected_ ? num_keys_pressed_ : 0;
    }

    constexpr void LinuxKeyboard::SetKeyThreshold(const Key key, float threshold)
    {
        if (IsValidKey(key))
        {
            key_data_[static_cast<int>(key)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void LinuxKeyboard::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_KEY_CODES; i++) { key_data_[i].SetThreshold(threshold); }
    }

    constexpr float LinuxKeyboard::GetKeyThreshold(const Key key) const
    {
        return IsValidKey(key)
            ? key_data_[static_cast<int>(key)].Threshold()
            : 0.0F;
    }

    constexpr float LinuxKeyboard::GetKeyValue(const Key key) const
    {
        return (is_connected_ && IsValidKey(key)) ? key_data_[static_cast<int>(key)].Value() : 0.0F;
    }

    constexpr bool LinuxKeyboard::GetKeyState(const Key key, float &time) const
    {
        if (is_connected_ && IsValidKey(key))
        {
            const TSTV &d = key_data_[static_cast<int>(key)];
            time = TimestampDeltaSeconds(d.Timestamp(), last_update_timestamp_);
            return d.State();
        }
        else
        {
            time = std::numeric_limits<float>::infinity();
            return false;
        }
    }


    // LINUX GAMEPAD
    void LinuxGamepad::HandlePendingEvents()
    {
        // modification to a thumbstick within a group of events
        struct ThumbstickMod
        {
            uint32_t target;
            int x, y;
            bool has_target, has_x, has_y;

            constexpr void Clear()
            {
                has_target = false;
                has_x = false;
                has_y = false;
            }

            constexpr void SetX(const uint32_t index, const float value)
            {
                if (has_target && target != index) { Clear(); } // failsafe

                if (!has_target || index == target)
                {
                    target = index;
                    x = value;
                    has_target = true;
                    has_x = true;
                }
            }

            constexpr void SetY(const uint32_t index, const float value)
            {
                if (has_target && target != index) { Clear(); } // failsafe

                if (!has_target || index == target)
                {
                    target = index;
                    y = value;
                    has_target = true;
                    has_y = true;
                }
            }
        };

        const auto handle_analog_trigger = [this](const int32_t raw_value, const timestamp_t timestamp, const int trigger_index, const Button b)
        {
            const float value = NormalizeAbsValue(this->trigger_norms_[trigger_index], raw_value);
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (this->button_data_[static_cast<int>(b)].Modify(value, timestamp, state)) { this->ButtonChanged(b, value, state); }
            #else
            this->button_data_[static_cast<int>(b)].Modify(value, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        };

        const auto handle_digital_button = [this](const int32_t raw_value, const timestamp_t timestamp, const Button b)
        {
            if (!IsValidButton(b)) { return; }

            // only handle purely digital button events, which are identified by not having any valid normalization
            // (some devices/drivers may send digital and analog events for the same physical input)
            const int b_index = static_cast<int>(b);
            const AbsValNorm *const p_norm = this->button_to_normalizer_[b_index];
            if (p_norm != nullptr) { return; }

            const float value = raw_value != 0 ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (this->button_data_[b_index].Modify(value, timestamp, state)) { ButtonChanged(b, value, state); }
            #else
            this->button_data_[b_index].Modify(value, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        };

        ThumbstickMod tsmod = {};

        for (const input_event &ev : pending_events_)
        {
            const timestamp_t timestamp = GetEventTimestamp(ev);

            switch (ev.type)
            {
            case EV_ABS:
                // analog value changed
                switch (ev.code)
                {
                // dpad
                case ABS_HAT0X: DigitalizeAnalogDpad(NormalizeAbsValue(dpad_norm_.first, ev.value), timestamp, Button::DPAD_RIGHT, Button::DPAD_LEFT); break;
                case ABS_HAT0Y: DigitalizeAnalogDpad(NormalizeAbsValue(dpad_norm_.second, ev.value), timestamp, Button::DPAD_UP, Button::DPAD_DOWN); break;

                // triggers (Y is left, X is right)
                case ABS_HAT1Y: handle_analog_trigger(ev.value, timestamp, 0, Button::L1); break;
                case ABS_HAT1X: handle_analog_trigger(ev.value, timestamp, 1, Button::R1); break;
                case ABS_HAT2Y: handle_analog_trigger(ev.value, timestamp, 2, Button::L2); break;
                case ABS_HAT2X: handle_analog_trigger(ev.value, timestamp, 3, Button::R2); break;

                // thumbsticks
                case ABS_X: tsmod.SetX(0, ev.value); break;
                case ABS_Y: tsmod.SetY(0, ev.value); break;
                case ABS_RX: tsmod.SetX(1, ev.value); break;
                case ABS_RY: tsmod.SetY(1, ev.value); break;
                }
                break;

            case EV_KEY:
                // digital value changed
                if (BTN_SOUTH <= ev.code && ev.code <= BTN_THUMBR)
                {
                    // regular
                    handle_digital_button(ev.value, timestamp, button_mapping[ev.code - BTN_SOUTH]);
                }
                else if (BTN_DPAD_UP <= ev.code && ev.code <= BTN_DPAD_RIGHT)
                {
                    // dpad
                    handle_digital_button(ev.value, timestamp, button_mapping_dpad[ev.code - BTN_DPAD_UP]);
                }
                break;

            #ifdef CROSSPUT_FEATURE_FORCE
            case EV_FF_STATUS:
                HandleFFStatusEvent(ev);
                break;
            #endif // CROSSPUT_FEATURE_FORCE
            }
        }

        // apply thumbstick modification
        if (tsmod.has_target)
        {
            [[maybe_unused]] bool do_callback = false; // only used for CROSSPUT_FEATURE_CALLBACK
            float x, y;
            const size_t tx = static_cast<size_t>(tsmod.target) * 2;
            const size_t ty = tx + 1;
            float &dest_x = thumbstick_values_[tx];
            float &dest_y = thumbstick_values_[ty];

            if (tsmod.has_x)
            {
                x = NormalizeAbsValue(thumbstick_norms_[tx], tsmod.x);
                #ifdef CROSSPUT_FEATURE_CALLBACK
                do_callback |= (x != dest_x);
                #endif // CROSSPUT_FEATURE_CALLBACK
                dest_x = x;
            }

            if (tsmod.has_y)
            {
                y = -NormalizeAbsValue(thumbstick_norms_[ty], tsmod.y); // negate Y
                #ifdef CROSSPUT_FEATURE_CALLBACK
                do_callback |= (y != dest_y);
                #endif // CROSSPUT_FEATURE_CALLBACK
                dest_y = y;
            }

            #ifdef CROSSPUT_FEATURE_CALLBACK
            if (do_callback) { ThumbstickChanged(tsmod.target, dest_x, dest_y); }
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        pending_events_.clear();
    }


    void LinuxGamepad::HandleBufferOverrun(const timestamp_t timestamp)
    {
        unsigned char globks[(KEY_CNT - 1) / 8 + 1];
        if (ioctl(file_desc_, EVIOCGKEY(sizeof(globks)), &globks) < 0)
        {
            // cannot query global key states
            Disconnect();
            return;
        }

        const auto handle_digital_button = [this, &globks, timestamp](const Button b)
        {
            const float value = GETBIT_(globks, reverse_button_mapping[static_cast<int>(b)]) ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (this->button_data_[static_cast<int>(b)].Modify(value, timestamp, state)) { ButtonChanged(b, value, state); }
            #else
            this->button_data_[static_cast<int>(b)].Modify(value, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        };

        const auto query_thumbstick = [this](const uint32_t index, const unsigned short code_x, const unsigned short code_y)
        {
            float x, y;
            if (!AbsValueFromIoctl(this->file_desc_, code_x, x)) { x = 0.0F; }
            if (!AbsValueFromIoctl(this->file_desc_, code_y, y)) { y = 0.0F; } else { y = -y; } // negate Y

            const size_t tx = static_cast<size_t>(index) * 2;
            float &dest_x = this->thumbstick_values_[tx];
            float &dest_y = this->thumbstick_values_[tx + 1];
            if (x != dest_x || y != dest_y)
            {
                dest_x = x;
                dest_y = y;

                #ifdef CROSSPUT_FEATURE_CALLBACK
                ThumbstickChanged(index, x, y);
                #endif // CROSSPUT_FEATURE_CALLBACK
            }
        };

        const auto query_trigger = [this, &handle_digital_button, timestamp](const int code, const Button b)
        {
            input_absinfo info;
            if (ioctl(this->file_desc_, EVIOCGABS(code), &info) >= 0)
            {
                // analog available
                const float value = NormalizeAbsValue(info);
                #ifdef CROSSPUT_FEATURE_CALLBACK
                bool state;
                if (this->button_data_[static_cast<int>(b)].Modify(value, timestamp, state)) { ButtonChanged(b, value, state); }
                #else
                this->button_data_[static_cast<int>(b)].Modify(value, timestamp);
                #endif // CROSSPUT_FEATURE_CALLBACK
            }
            else
            {
                // digital fallback
                handle_digital_button(b);
            }
        };

        const auto query_dpad = [this, &handle_digital_button, timestamp](const int code, const Button b1, const Button b2)
        {
            input_absinfo info;
            if (ioctl(this->file_desc_, EVIOCGABS(code), &info) >= 0)
            {
                // analog available
                this->DigitalizeAnalogDpad(NormalizeAbsValue(info), timestamp, b1, b2);
            }
            else
            {
                // digital fallback
                handle_digital_button(b1);
                handle_digital_button(b2);
            }
        };

        // dpad
        query_dpad(ABS_HAT0X, Button::DPAD_RIGHT, Button::DPAD_LEFT);
        query_dpad(ABS_HAT0Y, Button::DPAD_UP, Button::DPAD_DOWN);

        // triggers (Y is left, X is right)
        query_trigger(ABS_HAT1Y, Button::L1);
        query_trigger(ABS_HAT1X, Button::R1);
        query_trigger(ABS_HAT2Y, Button::L2);
        query_trigger(ABS_HAT2X, Button::R2);

        // thumbsticks
        query_thumbstick(0, ABS_X, ABS_Y);
        query_thumbstick(1, ABS_RX, ABS_RY);

        // regular buttons
        handle_digital_button(Button::NORTH);
        handle_digital_button(Button::SOUTH);
        handle_digital_button(Button::WEST);
        handle_digital_button(Button::EAST);
        handle_digital_button(Button::THUMBSTICK_L);
        handle_digital_button(Button::THUMBSTICK_R);
        handle_digital_button(Button::SELECT);
        handle_digital_button(Button::START);
    }


    void LinuxGamepad::OnConnected()
    {
        const auto handle_abs_val_button = [this](const int code, AbsValNorm &norm_dest, std::initializer_list<Button> &&buttons)
        {
            if (AbsValueNormFromIoctl(this->file_desc_, code, norm_dest))
            {
                for (const Button b : buttons) { this->button_to_normalizer_[static_cast<int>(b)] = &norm_dest; }
            }
        };

        // dpad
        handle_abs_val_button(ABS_HAT0X, dpad_norm_.first, { Button::DPAD_RIGHT, Button::DPAD_LEFT });
        handle_abs_val_button(ABS_HAT0Y, dpad_norm_.second, { Button::DPAD_UP, Button::DPAD_DOWN });

        // triggers (Y is left, X is right)
        handle_abs_val_button(ABS_HAT1Y, trigger_norms_[0], { Button::L1 });
        handle_abs_val_button(ABS_HAT1X, trigger_norms_[1], { Button::R1 });
        handle_abs_val_button(ABS_HAT2Y, trigger_norms_[2], { Button::L2 });
        handle_abs_val_button(ABS_HAT2X, trigger_norms_[3], { Button::R2 });

        // thumbsticks
        #define CREATE_THUMBSTICK_HANDLER(code, norm_dest) if (!AbsValueNormFromIoctl(file_desc_, code, norm_dest)) { norm_dest = {}; }
        CREATE_THUMBSTICK_HANDLER(ABS_X, thumbstick_norms_[0])
        CREATE_THUMBSTICK_HANDLER(ABS_Y, thumbstick_norms_[1])
        CREATE_THUMBSTICK_HANDLER(ABS_RX, thumbstick_norms_[2])
        CREATE_THUMBSTICK_HANDLER(ABS_RY, thumbstick_norms_[3])
        #undef CREATE_THUMBSTICK_HANDLER
    }


    void LinuxGamepad::OnDisconnected()
    {
        std::memset(button_data_, 0, sizeof(button_data_));
        std::memset(thumbstick_norms_, 0 , sizeof(thumbstick_norms_));
        std::memset(trigger_norms_, 0, sizeof(trigger_norms_));
        std::memset(thumbstick_values_, 0, sizeof(thumbstick_values_));
        std::memset(button_to_normalizer_, 0, sizeof(button_to_normalizer_));
        dpad_norm_ = {};
    }


    constexpr void LinuxGamepad::SetButtonThreshold(const Button button, float threshold)
    {
        if (IsValidButton(button))
        {
            button_data_[static_cast<int>(button)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void LinuxGamepad::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_BUTTON_CODES; i++) { button_data_[i].SetThreshold(threshold); }
    }

    constexpr float LinuxGamepad::GetButtonThreshold(const Button button) const
    {
        return IsValidButton(button)
            ? button_data_[static_cast<int>(button)].Threshold()
            : 0.0F;
    }

    constexpr float LinuxGamepad::GetButtonValue(const Button button) const
    {
        return (is_connected_ && IsValidButton(button))
            ? button_data_[static_cast<int>(button)].Value()
            : 0.0F;
    }

    constexpr bool LinuxGamepad::GetButtonState(const Button button, float &time) const
    {
        if (is_connected_ && IsValidButton(button))
        {
            const TSTV &d = button_data_[static_cast<int>(button)];
            time = TimestampDeltaSeconds(d.Timestamp(), last_update_timestamp_);
            return d.State();
        }
        else
        {
            time = std::numeric_limits<float>::infinity();
            return false;
        }
    }

    constexpr uint32_t LinuxGamepad::GetThumbstickCount() const
    {
        return is_connected_ ? LinuxGamepad::NUM_THUMBSTICKS : 0;
    }

    constexpr void LinuxGamepad::GetThumbstick(const uint32_t index, float &x, float &y) const
    {
        if (!is_connected_ || index >= LinuxGamepad::NUM_THUMBSTICKS)
        {
            x = 0.0F;
            y = 0.0F;
            return;
        }

        const size_t tx = static_cast<size_t>(index) * 2;
        x = thumbstick_values_[tx];
        y = thumbstick_values_[tx + 1];
    }


    #ifdef CROSSPUT_FEATURE_FORCE
    void LinuxForce::SetActive(const bool active)
    {
        if (IsOrphaned()) { return; }
        
        input_event ev = {.type = EV_FF, .code = static_cast<uint16_t>(effect_id_)};
        if (active)
        {
            if (status_ == ForceStatus::ACTIVE || !WriteParams()) { return; }
            ev.value = is_condition_effect_ ? std::numeric_limits<int>::max() : 1; // start (and repeat condition effects)
        }
        else
        {
            if (status_ == ForceStatus::INACTIVE) { return; }
            ev.value = 0; // stop
        }

        // allow failure
        [[maybe_unused]] const ssize_t ignore = write(p_device_->file_desc_, &ev, sizeof(input_event));
    }


    bool LinuxForce::WriteParams()
    {
        if (!IsOrphaned() && params_.type == type_)
        {
            ff_effect effect = TranslateForceParams(params_);
            effect.id = effect_id_;
            return ioctl(p_device_->file_desc_, EVIOCSFF, &effect) >= 0;
        }

        return false;
    }
    #endif // CROSSPUT_FEATURE_FORCE
}
