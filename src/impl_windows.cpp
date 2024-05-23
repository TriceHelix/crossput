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

#include <limits>
#include <unordered_set>
#include <GameInput.h>


struct WindowsHardwareID
{
    APP_LOCAL_DEVICE_ID id;

    constexpr WindowsHardwareID() noexcept = default;
    constexpr WindowsHardwareID(const APP_LOCAL_DEVICE_ID &native_id) noexcept : id(native_id) {}

    inline bool operator==(const WindowsHardwareID &other) const
    {
        return std::memcmp(id.value, other.id.value, APP_LOCAL_DEVICE_ID_SIZE) == 0;
    }
};


// hash specification
template <>
struct std::hash<WindowsHardwareID>
{
    size_t operator()(const WindowsHardwareID &id) const
    {
        return fnv1a_bytehash(id.id.value, APP_LOCAL_DEVICE_ID_SIZE);
    }
};


namespace crossput
{
    class WindowsDevice;
    class WindowsRumbleForce;
    class WindowsSpecialForce;
    using gdk_timestamp_t = uint64_t;


    // Windows Virtual-Key -> crossput Key
    constexpr Key keycode_mapping[] =
    {
/*0000*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::BACKSPACE,
        Key::TAB,
/*0010*/INVALID_KEY,
        INVALID_KEY,
        Key::NUMPAD5,
        Key::ENTER,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::PAUSE,
/*0020*/Key::CAPSLOCK,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::ESC,
        INVALID_KEY,
        INVALID_KEY,
/*0030*/INVALID_KEY,
        INVALID_KEY,
        Key::SPACE,
        Key::PAGEUP,
        Key::PAGEDOWN,
        Key::END,
        Key::HOME,
        Key::LEFT,
        Key::UP,
        Key::RIGHT,
/*0040*/Key::DOWN,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::INSERT,
        Key::DEL,
        INVALID_KEY,
        Key::NUMROW0,
        Key::NUMROW1,
/*0050*/Key::NUMROW2,
        Key::NUMROW3,
        Key::NUMROW4,
        Key::NUMROW5,
        Key::NUMROW6,
        Key::NUMROW7,
        Key::NUMROW8,
        Key::NUMROW9,
        INVALID_KEY,
        INVALID_KEY,
/*0060*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::A,
        Key::B,
        Key::C,
        Key::D,
        Key::E,
/*0070*/Key::F,
        Key::G,
        Key::H,
        Key::I,
        Key::J,
        Key::K,
        Key::L,
        Key::M,
        Key::N,
        Key::O,
/*0080*/Key::P,
        Key::Q,
        Key::R,
        Key::S,
        Key::T,
        Key::U,
        Key::V,
        Key::W,
        Key::X,
        Key::Y,
/*0090*/Key::Z,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::NUMPAD0,
        Key::NUMPAD1,
        Key::NUMPAD2,
        Key::NUMPAD3,
/*0100*/Key::NUMPAD4,
        Key::NUMPAD5,
        Key::NUMPAD6,
        Key::NUMPAD7,
        Key::NUMPAD8,
        Key::NUMPAD9,
        Key::NUMPAD_MULTIPLY,
        Key::NUMPAD_PLUS,
        Key::ENTER,
        Key::NUMPAD_MINUS,
/*0110*/Key::NUMPAD_DECIMAL,
        Key::NUMPAD_SLASH,
        Key::F1,
        Key::F2,
        Key::F3,
        Key::F4,
        Key::F5,
        Key::F6,
        Key::F7,
        Key::F8,
/*0120*/Key::F9,
        Key::F10,
        Key::F11,
        Key::F12,
        Key::F13,
        Key::F14,
        Key::F15,
        Key::F16,
        Key::F17,
        Key::F18,
/*0130*/Key::F19,
        Key::F20,
        Key::F21,
        Key::F22,
        Key::F23,
        Key::F24,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0140*/INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::NUMLOCK,
        Key::SCROLLLOCK,
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
/*0160*/Key::SHIFT_L,
        Key::SHIFT_R,
        Key::CTRL_L,
        Key::CTRL_R,
        Key::ALT_L,
        Key::ALT_R,
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
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
        Key::SEMICOLON,
        INVALID_KEY,
        INVALID_KEY,
        INVALID_KEY,
/*0190*/INVALID_KEY,
        Key::SLASH,
        Key::GRAVE,
        INVALID_KEY,
        INVALID_KEY,
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
        Key::BRACE_L,
/*0220*/Key::BACKSLASH,
        Key::BRACE_R,
        Key::APOSTROPHE,
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


    #ifdef CROSSPUT_FEATURE_FORCE
    constexpr GameInputForceFeedbackEffectKind force_type_mapping[NUM_FORCE_TYPES] =
    {
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackConstant,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackRamp,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackSineWave,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackTriangleWave,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackSquareWave,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackSawtoothUpWave,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackSawtoothDownWave,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackSpring,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackFriction,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackDamper,
        GameInputForceFeedbackEffectKind::GameInputForceFeedbackInertia
    };
    #endif // CROSSPUT_FEATURE_FORCE


    constexpr GameInputDeviceCapabilities NATIVE_SYNC_CAPABILITY = GameInputDeviceCapabilities::GameInputDeviceCapabilitySynchronization;
    constexpr GameInputKind Q_INPUT_KIND_MOUSE = GameInputKind::GameInputKindMouse;
    constexpr GameInputKind Q_INPUT_KIND_KEYBOARD = GameInputKind::GameInputKindKeyboard;
    constexpr GameInputKind Q_INPUT_KIND_GAMEPAD = GameInputKind::GameInputKindGamepad;
    constexpr GameInputKind Q_INPUT_KIND_ANY = Q_INPUT_KIND_MOUSE | Q_INPUT_KIND_KEYBOARD | Q_INPUT_KIND_GAMEPAD;


    // GameInput interface
    IGameInput *p_input = nullptr;
    std::unordered_set<WindowsHardwareID> nat_device_ids;


    inline bool IsNativeDeviceConnected(IGameInputDevice *const p_ndev)
    {
        return (p_ndev->GetDeviceStatus() & GameInputDeviceStatus::GameInputDeviceConnected) != GameInputDeviceStatus::GameInputDeviceNoStatus;
    }


    class WindowsDevice :
        public virtual BaseInterface,
        public virtual DeviceCallbackManager,
        public virtual DeviceForceManager<WindowsDevice>
    {
        friend WindowsRumbleForce;
        friend WindowsSpecialForce;

    protected:
        const WindowsHardwareID hardware_id_;
        IGameInputDevice *p_ndev_ = nullptr;
        gdk_timestamp_t last_reading_timestamp_ = 0;
        timestamp_t last_update_timestamp_ = 0;
        bool supports_sync_ = false;
        #ifdef CROSSPUT_FEATURE_FORCE
        // when supports_rumble_ == true, the first motor is for rumble only
        std::vector<std::bitset<NUM_FORCE_TYPES>> motor_capabilities_;
        std::vector<float> motor_gains_;
        WindowsRumbleForce *p_rumble_force = nullptr;
        bool supports_rumble_ = false;
        #endif // CROSSPUT_FEATURE_FORCE

    public:
        constexpr const WindowsHardwareID &GetHardwareID() const noexcept { return hardware_id_; }

        std::string GetDisplayName() const override final;
        void Update() override final;

        #ifdef CROSSPUT_FEATURE_FORCE
        constexpr bool SupportsForce(const uint32_t motor_index, const ForceType type) const noexcept override final
        {
            // different motors have different capabilities
            return is_connected_
                && motor_index < motor_capabilities_.size()
                && motor_capabilities_[motor_index][static_cast<int>(type)];
        }

        constexpr uint32_t GetMotorCount() const noexcept override final
        {
            return is_connected_ ? static_cast<uint32_t>(motor_capabilities_.size()) : 0;
        }

        constexpr float GetGain(const uint32_t motor_index) const noexcept override final
        {
            return (is_connected_ && motor_index < motor_gains_.size())
                ? motor_gains_[motor_index]
                : 0.0F;
        }

        void SetGain(const uint32_t motor_index,  float gain) override final;

        bool TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force) override final;
        #endif // CROSSPUT_FEATURE_FORCE

        #ifdef CROSSPUT_FEATURE_AGGREGATE
        constexpr bool IsAggregate() const noexcept override final { return false; }
        #endif // CROSSPUT_FEATURE_AGGREGATE

        virtual ~WindowsDevice()
        {
            nat_device_ids.erase(hardware_id_);
            ReleaseNativeDevicePtr();
        }

    protected:
        WindowsDevice(const APP_LOCAL_DEVICE_ID &dev_id) :
            hardware_id_(WindowsHardwareID(dev_id))
            {}

        virtual GameInputKind GetQueryInputKind() const = 0;

        // runs before any input is processed in Update(), after it is ensured the device is connected
        virtual constexpr void PreInputHandling() {}

        // device-specific handler for individual input readings provided by the base update implementation
        // when false is returned, no more readings are processed and the device update will be cancelled
        // NOTE: reading is automatically released after the method returns
        virtual bool HandleNativeReading(IGameInputReading *const p_reading) = 0;

        inline void ReleaseNativeDevicePtr()
        {
            if (p_ndev_ != nullptr)
            {
                p_ndev_->Release();
                p_ndev_ = nullptr;
            }
        }

        bool TryConnect();
        void Disconnect();

        virtual constexpr void OnConnected() {}
        virtual constexpr void OnDisconnected() {}
    };


    class WindowsMouse final :
        public virtual IMouse,
        public virtual WindowsDevice,
        public virtual TypedInterface<DeviceType::MOUSE>,
        public virtual MouseCallbackManager
    {
    public:
        static constexpr size_t NUM_BUTTONS = 5;

    private:
        struct Offset { int64_t x, y, sx, sy; };

        static constexpr GameInputMouseButtons INDEXED_BUTTONS[NUM_BUTTONS] =
        {
            GameInputMouseButtons::GameInputMouseLeftButton,
            GameInputMouseButtons::GameInputMouseRightButton,
            GameInputMouseButtons::GameInputMouseMiddleButton,
            GameInputMouseButtons::GameInputMouseButton4,
            GameInputMouseButtons::GameInputMouseButton5
        };

    
    private:
        TSTV button_data_[NUM_BUTTONS] = {};
        MouseData data_ = {};
        Offset offset_ = {};
        bool has_offset_ = false;

    public:
        WindowsMouse(const APP_LOCAL_DEVICE_ID &dev_id) : WindowsDevice(dev_id) {}

        constexpr void GetPosition(int64_t &x, int64_t &y) const noexcept override;
        constexpr void GetDelta(int64_t &x, int64_t &y) const noexcept override;
        constexpr void GetScroll(int64_t &x, int64_t &y) const noexcept override;
        constexpr void GetScrollDelta(int64_t &x, int64_t &y) const noexcept override;
        constexpr uint32_t GetButtonCount() const noexcept override;
        constexpr void SetButtonThreshold(const uint32_t index, float threshold) noexcept override;
        constexpr void SetGlobalThreshold(float threshold) noexcept override;
        constexpr float GetButtonThreshold(const uint32_t index) const noexcept override;
        constexpr float GetButtonValue(const uint32_t index) const noexcept override;
        constexpr bool GetButtonState(const uint32_t index, float &time) const noexcept override;

    protected:
        constexpr GameInputKind GetQueryInputKind() const noexcept { return Q_INPUT_KIND_MOUSE; }
        void PreInputHandling() override;
        bool HandleNativeReading(IGameInputReading *const p_reading) override;
        void OnDisconnected() override;
    };


    class WindowsKeyboard final :
        public virtual IKeyboard,
        public virtual WindowsDevice,
        public virtual TypedInterface<DeviceType::KEYBOARD>,
        public virtual KeyboardCallbackManager
    {
    private:
        TSTV key_data_[NUM_KEY_CODES] = {};
        std::unique_ptr<GameInputKeyState[]> p_giks_cache_;
        uint32_t max_num_keys_pressed_ = 0;
        uint32_t num_keys_pressed_ = 0;

    public:
        WindowsKeyboard(const APP_LOCAL_DEVICE_ID &dev_id) : WindowsDevice(dev_id) {}
        
        constexpr uint32_t GetNumKeysPressed() const override;
        constexpr void SetKeyThreshold(const Key key, float threshold) override;
        constexpr void SetGlobalThreshold(float threshold) override;
        constexpr float GetKeyThreshold(const Key key) const override;
        constexpr float GetKeyValue(const Key key) const override;
        constexpr bool GetKeyState(const Key key, float &time) const override;

    private:
        constexpr GameInputKind GetQueryInputKind() const noexcept { return Q_INPUT_KIND_KEYBOARD; }
        bool HandleNativeReading(IGameInputReading *const p_reading) override;
        void OnConnected() override;
        void OnDisconnected() override;
    };


    class WindowsGamepad final :
        public virtual IGamepad,
        public virtual WindowsDevice,
        public virtual TypedInterface<DeviceType::GAMEPAD>,
        public virtual GamepadCallbackManager
    {
    public:
        static constexpr size_t NUM_THUMBSTICKS = 2;

    private:
        TSTV button_data_[NUM_BUTTON_CODES] = {};
        std::pair<float, float> thumbstick_values_[NUM_THUMBSTICKS] = {};
        
    public:
        WindowsGamepad(const APP_LOCAL_DEVICE_ID &dev_id) : WindowsDevice(dev_id) {}

        constexpr void SetButtonThreshold(const Button button, float threshold) override;
        constexpr void SetGlobalThreshold(float threshold) override;
        constexpr float GetButtonThreshold(const Button button) const override;
        constexpr float GetButtonValue(const Button button) const override;
        constexpr bool GetButtonState(const Button button, float &time) const override;
        constexpr uint32_t GetThumbstickCount() const override;
        constexpr void GetThumbstick(const uint32_t index, float &x, float &y) const override;

    private:
        constexpr GameInputKind GetQueryInputKind() const noexcept { return Q_INPUT_KIND_GAMEPAD; }
        bool HandleNativeReading(IGameInputReading *const p_reading) override;
        void OnDisconnected() override;

        inline void HandleThumbstick(const uint32_t index, const float x, const float y)
        {
            if (x != thumbstick_values_[index].first || y != thumbstick_values_[index].second)
            {
                thumbstick_values_[index] = {x, y};

                #ifdef CROSSPUT_FEATURE_CALLBACK
                ThumbstickChanged(index, x, y);
                #endif // CROSSPUT_FEATURE_CALLBACK
            }
        }
    };


    #ifdef CROSSPUT_FEATURE_FORCE
    class WindowsRumbleForce final : public BaseForce<WindowsDevice>
    {
    private:
        bool is_active_ = false;

    public:
        WindowsRumbleForce(WindowsDevice *const p_device) :
            BaseForce(p_device, 0, ForceType::RUMBLE)
        {
            assert(p_device != nullptr);
        }

        constexpr ForceStatus GetStatus() const override { return (!IsOrphaned() && is_active_) ? ForceStatus::ACTIVE : ForceStatus::INACTIVE; }
        void SetActive(const bool active) override;
        bool WriteParams() override;

        ~WindowsRumbleForce()
        {
            if (!IsOrphaned())
            {
                // disable rumble
                GameInputRumbleParams p = {};
                p_device_->p_ndev_->SetRumbleState(&p);
            }
        }
    };


    class WindowsSpecialForce final : public BaseForce<WindowsDevice>
    {
    private:
        IGameInputForceFeedbackEffect *const p_ff_effect_;

    public:
        WindowsSpecialForce(WindowsDevice *const p_device, IGameInputForceFeedbackEffect *const p_ff_effect, const uint32_t motor_index, const ForceType type) :
            BaseForce(p_device, motor_index, type),
            p_ff_effect_(p_ff_effect)
        {
            assert(p_device != nullptr);
            assert(p_ff_effect != nullptr);
        }

        ForceStatus GetStatus() const override;
        void SetActive(const bool active) override;
        bool WriteParams() override;
        
        ~WindowsSpecialForce()
        {
            if (!IsOrphaned())
            {
                // remove effect from device
                p_ff_effect_->SetState(GameInputFeedbackEffectState::GameInputFeedbackStopped);
                p_ff_effect_->Release();
            }
        }
    };
    #endif // CROSSPUT_FEATURE_FORCE


    IDevice *CreateDevice(IGameInputDevice *const p_ndev)
    {
        assert(p_ndev != nullptr);

        WindowsDevice *p_vdev;
        const GameInputDeviceInfo *p_info = p_ndev->GetDeviceInfo();
        assert(p_info != nullptr);
        
        // detect device type
        GameInputKind dev_types = p_info->supportedInput;
        if ((dev_types & GameInputKind::GameInputKindMouse) != GameInputKind::GameInputKindUnknown)
        {
            p_vdev = new WindowsMouse(p_info->deviceId);
        }
        else if ((dev_types & GameInputKind::GameInputKindKeyboard) != GameInputKind::GameInputKindUnknown)
        {
            p_vdev = new WindowsKeyboard(p_info->deviceId);
        }
        else if ((dev_types & GameInputKind::GameInputKindController) != GameInputKind::GameInputKindUnknown)
        {
            p_vdev = new WindowsGamepad(p_info->deviceId);
        }
        else
        {
            // incompatible with crossput
            return nullptr;
        }

        glob_interfaces.insert({p_vdev->GetID(), p_vdev});
        nat_device_ids.insert(p_vdev->GetHardwareID());

        #ifdef CROSSPUT_FEATURE_CALLBACK
        DeviceStatusChanged(p_vdev, DeviceStatusChange::DISCOVERED);
        #endif // CROSSPUT_FEATURE_CALLBACK

        return p_vdev;
    }


    // iterate over input chain until the specified timestamp is reached, using given filters, and handle each input reading (from oldest to newest) using the provided handler function
    // returns number of inputs read, including the first input that is past the minimum timestamp (meaning if an error occurs, 0 is returned)
    size_t ReadInputChain(const GameInputKind input_kind, IGameInputDevice *const p_device, const gdk_timestamp_t min_timestamp, auto &&handler)
    {
        assert(input_kind != GameInputKind::GameInputKindUnknown);

        size_t read = 0;
        IGameInputReading *p_reading;
        std::vector<IGameInputReading *> readings;
        readings.reserve(16);

        if (SUCCEEDED(p_input->GetCurrentReading(input_kind, p_device, &p_reading)))
        {
            do
            {
                read++;

                if (p_reading->GetTimestamp() < min_timestamp)
                {
                    // all relevant readings have been processed
                    p_reading->Release();
                    break;
                }
                
                readings.push_back(p_reading);
            }
            while (SUCCEEDED(p_input->GetPreviousReading(p_reading, input_kind, p_device, &p_reading)));
        }

        if (readings.size() > 0)
        {
            // handle readings in reverse (old to new)
            for (auto it = readings.crbegin(); it != readings.crend(); it++)
            {
                // assume handler returns boolean-like value that indicates failure when evaluated to false
                if (!handler(*it))
                {
                    read = 0;
                    break;
                }
            }

            // release readings, decrementing the underlying reference count
            for (const auto rd : readings)
            {
                rd->Release();
            }
        }

        return read;
    }
    

    #ifdef CROSSPUT_FEATURE_FORCE
    constexpr GameInputForceFeedbackEnvelope TranslateEnvelope(const ForceEnvelope &envelope) noexcept
    {
        // when the sum of times exceeds MAX_TIME, all times are scaled back into the valid range
        const float m = 1.0F / std::max(1.0F, (std::max(0.0F, envelope.attack_time) + std::max(0.0F, envelope.sustain_time) + std::max(0.0F, envelope.release_time)) * (1.0F / ForceEnvelope::MAX_TIME));

        return GameInputForceFeedbackEnvelope
        {
            // seconds -> microseconds
            .attackDuration = static_cast<uint64_t>(envelope.attack_time * m * 1E6F),
            .sustainDuration = static_cast<uint64_t>(envelope.sustain_time * m * 1E6F),
            .releaseDuration = static_cast<uint64_t>(envelope.release_time * m * 1E6F),

            .attackGain = std::clamp(envelope.attack_gain, 0.0F, 1.0F),
            .sustainGain = std::clamp(envelope.sustain_gain, 0.0F, 1.0F),
            .releaseGain = std::clamp(envelope.release_gain, 0.0F, 1.0F),

            // crossput only supports manual playback (no repetitions)
            .playCount = 1,
            .repeatDelay = 0
        };
    }


    constexpr GameInputForceFeedbackMagnitude TranslateMagnitude(const float m) noexcept
    {
        return { m, m, m, m, m, m, m };
    }


    constexpr GameInputRumbleParams TranslateRumbleParams(const RumbleForceParams &params, const float gain) noexcept
    {
        return GameInputRumbleParams
        {
            .lowFrequency = std::clamp(params.low_frequency, 0.0F, 1.0F) * gain,
            .highFrequency = std::clamp(params.high_frequency, 0.0F, 1.0F) * gain,
            .leftTrigger = 0.0F,
            .rightTrigger = 0.0F
        };
    }


    constexpr GameInputForceFeedbackConstantParams TranslateConstantParams(const ConstantForceParams &params) noexcept
    {
        return GameInputForceFeedbackConstantParams
        {
            .envelope = TranslateEnvelope(params.envelope),
            .magnitude = TranslateMagnitude(params.magnitude)
        };
    }


    constexpr GameInputForceFeedbackRampParams TranslateRampParams(const RampForceParams &params) noexcept
    {
        return GameInputForceFeedbackRampParams
        {
            .envelope = TranslateEnvelope(params.envelope),
            .startMagnitude = TranslateMagnitude(params.magnitude_start),
            .endMagnitude = TranslateMagnitude(params.magnitude_end)
        };
    }


    constexpr GameInputForceFeedbackPeriodicParams TranslatePeriodicParams(const PeriodicForceParams &params) noexcept
    {
        return GameInputForceFeedbackPeriodicParams
        {
            .envelope = TranslateEnvelope(params.envelope),
            .magnitude = TranslateMagnitude(params.magnitude),
            .frequency = params.frequency,
            .phase = std::clamp(params.phase, 0.0F, 1.0F),
            .bias = params.offset
        };
    }


    constexpr GameInputForceFeedbackConditionParams TranslateConditionParams(const ConditionForceParams &params) noexcept
    {
        return GameInputForceFeedbackConditionParams
        {
            .magnitude = TranslateMagnitude(params.magnitude),
            .positiveCoefficient = std::clamp(params.right_coefficient, -1.0F, 1.0F),
            .negativeCoefficient = std::clamp(params.left_coefficient, -1.0F, 1.0F),
            .maxPositiveMagnitude = params.right_saturation,
            .maxNegativeMagnitude = params.left_saturation,
            .deadZone = std::clamp(params.deadzone, 0.0F, 1.0F),
            .bias = std::clamp(params.center, -1.0F, 1.0F)
        };
    }


    inline GameInputForceFeedbackParams TranslateForceFeedbackParams(const ForceParams &params)
    {
        GameInputForceFeedbackParams result;

        switch (params.type)
        {
        case ForceType::CONSTANT:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackConstant;
            result.data.constant = TranslateConstantParams(params.constant);
            break;

        case ForceType::RAMP:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackRamp;
            result.data.ramp = TranslateRampParams(params.ramp);
            break;

        case ForceType::SINE:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackSineWave;
            result.data.sineWave = TranslatePeriodicParams(params.periodic);
            break;

        case ForceType::TRIANGLE:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackTriangleWave;
            result.data.triangleWave = TranslatePeriodicParams(params.periodic);
            break;

        case ForceType::SQUARE:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackSquareWave;
            result.data.squareWave = TranslatePeriodicParams(params.periodic);
            break;

        case ForceType::SAW_UP:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackSawtoothUpWave;
            result.data.sawtoothUpWave = TranslatePeriodicParams(params.periodic);
            break;

        case ForceType::SAW_DOWN:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackSawtoothDownWave;
            result.data.sawtoothDownWave = TranslatePeriodicParams(params.periodic);
            break;

        case ForceType::SPRING:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackSpring;
            result.data.spring = TranslateConditionParams(params.condition);
            break;

        case ForceType::FRICTION:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackFriction;
            result.data.friction = TranslateConditionParams(params.condition);
            break;

        case ForceType::DAMPER:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackDamper;
            result.data.damper = TranslateConditionParams(params.condition);
            break;

        case ForceType::INERTIA:
            result.kind = GameInputForceFeedbackEffectKind::GameInputForceFeedbackInertia;
            result.data.inertia = TranslateConditionParams(params.condition);
            break;
        }

        return result;
    }
    #endif // CROSSPUT_FEATURE_FORCE


    // GLOBAL FUNCTION IMPLEMENTATIONS

    size_t DiscoverDevices()
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        if (p_input == nullptr)
        {
            // first invocation -> hook into GDK
            if (!SUCCEEDED(GameInputCreate(&p_input)))
            {
                throw std::runtime_error("Error during crossput initialization: Failed to create GDK GameInput singleton.");
            }

            nat_device_ids.reserve(16);
        }

        constexpr auto devenum_callback = [](
            [[maybe_unused]] _In_ GameInputCallbackToken callback_token,
            [[maybe_unused]] _In_ void * p_context,
            [[maybe_unused]] _In_ IGameInputDevice * p_device,
            [[maybe_unused]] _In_ gdk_timestamp_t timestamp,
            [[maybe_unused]] _In_ GameInputDeviceStatus current_status,
            [[maybe_unused]] _In_ GameInputDeviceStatus previous_status)
        {
            // check if interface for device exists
            if (!nat_device_ids.contains(WindowsHardwareID(p_device->GetDeviceInfo()->deviceId)))
            {
                if (CreateDevice(p_device) != nullptr)
                {
                    // "devices_created" is passed as context
                    *reinterpret_cast<size_t *>(p_context) += 1;
                }
            }
        };

        // enumerate all connected devices and invoke the callback lambda for each one
        size_t devices_created = 0;
        GameInputCallbackToken token;
        if (SUCCEEDED(p_input->RegisterDeviceCallback(
            nullptr,
            Q_INPUT_KIND_ANY,
            GameInputDeviceStatus::GameInputDeviceConnected,
            GameInputEnumerationKind::GameInputBlockingEnumeration,
            &devices_created,
            // add __stdcall qualifier to lambda
            static_cast<void (CALLBACK *)(
                _In_ GameInputCallbackToken,
                _In_ void *,
                _In_ IGameInputDevice *,
                _In_ gdk_timestamp_t,
                _In_ GameInputDeviceStatus,
                _In_ GameInputDeviceStatus)>
                (devenum_callback),
            &token)))
        {
            // unregister callback immediately
            p_input->UnregisterCallback(token, 60000);
        }

        return devices_created;
    }


    // WINDOWS DEVICE
    std::string WindowsDevice::GetDisplayName() const
    {
        if (is_connected_)
        {
            const GameInputDeviceInfo *const p_info = p_ndev_->GetDeviceInfo();
            if (p_info != nullptr && p_info->displayName != nullptr)
            {
                return std::string(p_info->displayName->data, p_info->displayName->sizeInBytes);
            }
        }

        return std::string();
    }


    void WindowsDevice::Update()
    {
        ProtectManagementAPI(std::format("crossput::IDevice::Update() - Device ID {}", id_));
        
        if (!(is_connected_ || TryConnect())) { return; }

        if (!IsNativeDeviceConnected(p_ndev_))
        {
            // device disconnected via natural means
            Disconnect();
            return;
        }

        // decrease input latency
        if (supports_sync_)
        {
            p_ndev_->SendInputSynchronizationHint();
        }

        PreInputHandling();
        
        last_update_timestamp_ = p_input->GetCurrentTimestamp();
        if (ReadInputChain(GetQueryInputKind(), p_ndev_, last_reading_timestamp_ + 1, std::bind(&WindowsDevice::HandleNativeReading, this, std::placeholders::_1)) < 1)
        {
            // an error occurred, release GDK interface as an attempt to fix issue and disconnect manually
            ReleaseNativeDevicePtr();
            Disconnect();
            return;
        }
    }


    bool WindowsDevice::TryConnect()
    {
        if (is_connected_) [[unlikely]] { return true; }

        // query connection status (re-bind to GDK interface if needed)
        bool result = p_ndev_ != nullptr
            ? IsNativeDeviceConnected(p_ndev_)
            : (SUCCEEDED(p_input->FindDeviceFromId(&hardware_id_.id, &p_ndev_)) && IsNativeDeviceConnected(p_ndev_));

        if (result)
        {
            is_connected_ = true;

            // query sync capability
            const GameInputDeviceInfo &info = *(p_ndev_->GetDeviceInfo());
            supports_sync_ = (info.capabilities & NATIVE_SYNC_CAPABILITY) != GameInputDeviceCapabilities::GameInputDeviceCapabilityNone;
            if (supports_sync_) { p_ndev_->SetInputSynchronizationState(true); }

            #ifdef CROSSPUT_FEATURE_FORCE
            // query rumble capability
            supports_rumble_ = info.supportedRumbleMotors != GameInputRumbleMotors::GameInputRumbleNone;
            const uint32_t num_virtual_motors = (supports_rumble_ ? 1 : 0) + info.forceFeedbackMotorCount;
            motor_capabilities_.reserve(num_virtual_motors);
            motor_gains_.assign(num_virtual_motors, 1.0F);

            if (supports_rumble_)
            {
                // pretend to have an extra motor just for rumbling
                std::bitset<NUM_FORCE_TYPES> sf;
                sf[static_cast<int>(ForceType::RUMBLE)] = true;
                motor_capabilities_.push_back(sf);
            }

            // query force-feedback capabilities of each real motor
            // and set the initial gain
            for (uint32_t i = 0; i < info.forceFeedbackMotorCount; i++)
            {
                p_ndev_->SetForceFeedbackMotorGain(i, 1.0F);

                std::bitset<NUM_FORCE_TYPES> sf;
                const GameInputForceFeedbackMotorInfo &motor_info = info.forceFeedbackMotorInfo[i];

                #define QUERY_FF_TYPE_(enum_name, field_name) sf[static_cast<int>(ForceType::enum_name)] = motor_info.is ## field_name ## EffectSupported
                QUERY_FF_TYPE_(CONSTANT, Constant);
                QUERY_FF_TYPE_(RAMP, Ramp);
                QUERY_FF_TYPE_(SINE, SineWave);
                QUERY_FF_TYPE_(TRIANGLE, TriangleWave);
                QUERY_FF_TYPE_(SQUARE, SquareWave);
                QUERY_FF_TYPE_(SAW_UP, SawtoothUpWave);
                QUERY_FF_TYPE_(SAW_DOWN, SawtoothDownWave);
                QUERY_FF_TYPE_(SPRING, Spring);
                QUERY_FF_TYPE_(FRICTION, Friction);
                QUERY_FF_TYPE_(DAMPER, Damper);
                QUERY_FF_TYPE_(INERTIA, Inertia);
                #undef QUERY_FF_TYPE_

                motor_capabilities_.push_back(sf);
            }
            #endif // CROSSPUT_FEATURE_FORCE

            OnConnected();

            #ifdef CROSSPUT_FEATURE_CALLBACK
            DeviceStatusChanged(this, DeviceStatusChange::CONNECTED);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        return result;
    }


    void WindowsDevice::Disconnect()
    {
        if (!is_connected_) [[unlikely]] { return; }

        is_connected_ = false;
        last_reading_timestamp_ = 0;
        last_update_timestamp_ = 0;
        supports_sync_ = false;
        #ifdef CROSSPUT_FEATURE_FORCE
        motor_capabilities_.clear();
        motor_gains_.clear();
        supports_rumble_ = false;
        p_rumble_force = nullptr;
        #endif // CROSSPUT_FEATURE_FORCE

        OnDisconnected();

        #ifdef CROSSPUT_FEATURE_FORCE
        OrphanAllForces();
        #endif // CROSSPUT_FEATURE_FORCE

        #ifdef CROSSPUT_FEATURE_CALLBACK
        DeviceStatusChanged(this, DeviceStatusChange::DISCONNECTED);
        #endif // CROSSPUT_FEATURE_CALLBACK
    }


    #ifdef CROSSPUT_FEATURE_FORCE
    void WindowsDevice::SetGain(const uint32_t motor_index, float gain)
    {
        if (!is_connected_ || motor_index >= motor_gains_.size()) { return; }
        
        gain = std::clamp(gain, 0.0F, 1.0F);
        motor_gains_[motor_index] = gain;

        // update gain on underlying hardware
        if (supports_rumble_)
        {
            if (motor_index == 0 && p_rumble_force != nullptr) { p_rumble_force->WriteParams(); }
            if (motor_index > 0) { p_ndev_->SetForceFeedbackMotorGain(motor_index - 1, gain); } // skip fake motor
        }
        else
        {
            p_ndev_->SetForceFeedbackMotorGain(motor_index, gain);
        }
    }


    bool WindowsDevice::TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force)
    {
        bool result = false;
        BaseForce<WindowsDevice> *p_force_interface = nullptr;

        if (SupportsForce(motor_index, type))
        {
            if (type == ForceType::RUMBLE)
            {
                // only one rumble instance can exist at any time and it always uses virtual motor 0
                result = p_rumble_force == nullptr;
                if (result)
                {
                    p_rumble_force = new WindowsRumbleForce(this);
                    p_force_interface = p_rumble_force;
                }
            }
            else
            {
                // special force-feedback (skip virtual rumble motor if present)
                assert(motor_index > 0);
                const uint32_t native_motor_index = supports_rumble_ ? (motor_index - 1) : motor_index;

                GameInputForceFeedbackParams initial_params =
                {
                    .kind = force_type_mapping[static_cast<int>(type)],
                    .data = {}
                };

                IGameInputForceFeedbackEffect *p_ff_effect;
                if (SUCCEEDED(p_ndev_->CreateForceFeedbackEffect(native_motor_index, &initial_params, &p_ff_effect)))
                {
                    p_force_interface = new WindowsSpecialForce(this, p_ff_effect, motor_index, type);
                    result = true;
                }
            }
            
        }

        if (p_force_interface != nullptr) { forces_.insert({p_force_interface->GetID(), p_force_interface}); }
        p_force = p_force_interface;
        return result;
    }
    #endif // CROSSPUT_FEATURE_FORCE


    // MOUSE
    void WindowsMouse::PreInputHandling()
    {
        // clear deltas
        data_.dx = 0;
        data_.dy = 0;
        data_.sdx = 0;
        data_.sdy = 0;
    }


    bool WindowsMouse::HandleNativeReading(IGameInputReading *const p_reading)
    {
        GameInputMouseState ms;
        if (!p_reading->GetMouseState(&ms))
        {
            return false;
        }

        const gdk_timestamp_t gdk_timestamp = p_reading->GetTimestamp();
        const timestamp_t timestamp = static_cast<timestamp_t>(gdk_timestamp);
        last_reading_timestamp_ = gdk_timestamp;
        last_update_timestamp_ = std::max(timestamp, last_update_timestamp_);
        
        int64_t dx, dy, sdx, sdy;
        if (has_offset_) [[likely]]
        {
            // compute deltas
            dx = ms.positionX - offset_.x;
            dy = ms.positionY - offset_.y;
            sdx = ms.wheelX - offset_.sx;
            sdy = ms.wheelY - offset_.sy;

            // accumulate deltas
            data_.dx += dx;
            data_.dy += dy;
            data_.sdx += sdx;
            data_.sdy += sdy;
        }
        else
        {
            dx = 0;
            dy = 0;
            sdx = 0;
            sdy = 0;
            has_offset_ = true;
        }

        offset_ = { ms.positionX, ms.positionY, ms.wheelX, ms.wheelY };

        // add deltas to total
        data_.x += dx;
        data_.y += dy;
        data_.sx += sdx;
        data_.sy += sdy;
        
        #ifdef CROSSPUT_FEATURE_CALLBACK
        if (dx != 0 || dy != 0) { MouseMoved(data_.x, data_.y, dx, dy); }
        if (sdx != 0 || sdy != 0) { MouseScrolled(data_.sx, data_.sy, sdx, sdy); }
        #endif // CROSSPUT_FEATURE_CALLBACK

        // update button states and timestamps
        for (unsigned int b = 0; b < WindowsMouse::NUM_BUTTONS; b++)
        {
            const float value = ((ms.buttons & INDEXED_BUTTONS[b]) != GameInputMouseButtons::GameInputMouseNone) ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (button_data_[b].Modify(value, timestamp, state)) { ButtonChanged(b, value, state); }
            #else
            button_data_[b].Modify(value, timestamp);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        return true;
    }


    void WindowsMouse::OnDisconnected()
    {
        std::memset(button_data_, 0, sizeof(button_data_));
        data_ = {};
        offset_ = {};
        has_offset_ = false;
    }


    constexpr void WindowsMouse::GetPosition(int64_t &x, int64_t &y) const noexcept
    {
        if (!is_connected_) { return; }
        x = data_.x;
        y = data_.y;
    }

    constexpr void WindowsMouse::GetDelta(int64_t &x, int64_t &y) const noexcept
    {
        if (!is_connected_) { return; }
        x = data_.dx;
        y = data_.dy;
    }

    constexpr void WindowsMouse::GetScroll(int64_t &x, int64_t &y) const noexcept
    {
        if (!is_connected_) { return; }
        x = data_.sx;
        y = data_.sy;
    }

    constexpr void WindowsMouse::GetScrollDelta(int64_t &x, int64_t &y) const noexcept
    {
        if (!is_connected_) { return; }
        x = data_.sdx;
        y = data_.sdy;
    }

    constexpr uint32_t WindowsMouse::GetButtonCount() const noexcept
    {
        return is_connected_ ? WindowsMouse::NUM_BUTTONS : 0;
    }

    constexpr void WindowsMouse::SetButtonThreshold(const uint32_t index, float threshold) noexcept
    {
        if (index < WindowsMouse::NUM_BUTTONS)
        {
            button_data_[index].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void WindowsMouse::SetGlobalThreshold(float threshold) noexcept
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < WindowsMouse::NUM_BUTTONS; i++) { button_data_[i].SetThreshold(threshold); }
    }

    constexpr float WindowsMouse::GetButtonThreshold(const uint32_t index) const noexcept
    {
        return index < WindowsMouse::NUM_BUTTONS
            ? button_data_[index].Threshold()
            : 0.0F;
    }

    constexpr float WindowsMouse::GetButtonValue(const uint32_t index) const noexcept
    {
        return (is_connected_ && index < WindowsMouse::NUM_BUTTONS)
            ? button_data_[index].Value()
            : 0.0F;
    }

    constexpr bool WindowsMouse::GetButtonState(const uint32_t index, float &time) const noexcept
    {
        if (is_connected_ && index < WindowsMouse::NUM_BUTTONS)
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


    // KEYBOARD
    bool WindowsKeyboard::HandleNativeReading(IGameInputReading *const p_reading)
    {
        const gdk_timestamp_t gdk_timestamp = p_reading->GetTimestamp();
        const timestamp_t timestamp = static_cast<timestamp_t>(gdk_timestamp);
        last_reading_timestamp_ = gdk_timestamp;
        last_update_timestamp_ = std::max(timestamp, last_update_timestamp_);

        const uint32_t nkp = p_reading->GetKeyCount();
        const uint32_t valid_nkp = p_reading->GetKeyState(nkp, this->p_giks_cache_.get());

        // compute the state of every key
        bool new_states[NUM_KEY_CODES] = {};
        for (uint32_t kp = 0; kp < valid_nkp; kp++)
        {
            const Key key = keycode_mapping[this->p_giks_cache_[kp].virtualKey];
            if (IsValidKey(key)) { new_states[static_cast<int>(key)] = true; }
        }

        // update key data
        for (unsigned int k = 0; k < NUM_KEY_CODES; k++)
        {
            const float value = new_states[k] ? 1.0F : 0.0F;
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (key_data_[k].Modify(value, timestamp, state, num_keys_pressed_)) { KeyChanged(static_cast<Key>(k), value, state); }
            #else
            key_data_[k].Modify(value, timestamp, num_keys_pressed_);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }

        return true;
    }


    void WindowsKeyboard::OnConnected()
    {
        max_num_keys_pressed_ = p_ndev_->GetDeviceInfo()->keyboardInfo->maxSimultaneousKeys;
        p_giks_cache_.reset(new GameInputKeyState[max_num_keys_pressed_]);
    }


    void WindowsKeyboard::OnDisconnected()
    {
        p_giks_cache_.reset();
        std::memset(key_data_, 0 ,sizeof(key_data_));
        max_num_keys_pressed_ = 0;
        num_keys_pressed_ = 0;
    }


    constexpr uint32_t WindowsKeyboard::GetNumKeysPressed() const
    {
        return is_connected_ ? num_keys_pressed_ : 0;
    }

    constexpr void WindowsKeyboard::SetKeyThreshold(const Key key, float threshold)
    {
        if (IsValidKey(key))
        {
            key_data_[static_cast<int>(key)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void WindowsKeyboard::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_KEY_CODES; i++) { key_data_[i].SetThreshold(threshold); }
    }

    constexpr float WindowsKeyboard::GetKeyThreshold(const Key key) const
    {
        return IsValidKey(key)
            ? key_data_[static_cast<int>(key)].Threshold()
            : 0.0F;
    }

    constexpr float WindowsKeyboard::GetKeyValue(const Key key) const
    {
        return (is_connected_ && IsValidKey(key))
            ? key_data_[static_cast<int>(key)].Value()
            : 0.0F;
    }

    constexpr bool WindowsKeyboard::GetKeyState(const Key key, float &time) const
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


    // GAMEPAD
    bool WindowsGamepad::HandleNativeReading(IGameInputReading *const p_reading)
    {
        GameInputGamepadState gs;
        if (!p_reading->GetGamepadState(&gs))
        {
            return false;
        }

        const gdk_timestamp_t gdk_timestamp = p_reading->GetTimestamp();
        const timestamp_t timestamp = static_cast<timestamp_t>(gdk_timestamp);
        last_reading_timestamp_ = gdk_timestamp;
        last_update_timestamp_ = std::max(timestamp, last_update_timestamp_);

        #ifdef CROSSPUT_FEATURE_CALLBACK
        // => event-based handlers

        #define HANDLE_GAMEPAD_BUTTON_(from, to) \
        { \
            bool state; \
            const float value = (gs.buttons & GameInputGamepadButtons:: ## from) ? 1.0F : 0.0F; \
            if (button_data_[static_cast<size_t>(to)].Modify(value, timestamp, state)) { ButtonChanged(static_cast<Button>(to), value, state); } \
        }

        #define HANDLE_GAMEPAD_TRIGGER_(value, to) \
        { \
            bool state; \
            if (button_data_[static_cast<size_t>(to)].Modify(value, timestamp, state)) { ButtonChanged(static_cast<Button>(to), value, state); } \
        }

        #else
        // => regular handlers

        // digital button enum handler
        #define HANDLE_GAMEPAD_BUTTON_(from, to) \
        { \
            const float value = (gs.buttons & GameInputGamepadButtons:: ## from) ? 1.0F : 0.0F; \
            button_data_[static_cast<size_t>(to)].Modify(value, timestamp); \
        }
        
        // analog button field handler
        #define HANDLE_GAMEPAD_TRIGGER_(value, to) \
        { button_data_[static_cast<size_t>(to)].Modify(value, timestamp); }

        #endif // CROSSPUT_FEATURE_CALLBACK

        // update buttons
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadY, Button::NORTH)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadA, Button::SOUTH)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadX, Button::WEST)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadB, Button::EAST)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadDPadUp, Button::DPAD_UP)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadDPadDown, Button::DPAD_DOWN)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadDPadLeft, Button::DPAD_LEFT)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadDPadRight, Button::DPAD_RIGHT)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadLeftShoulder, Button::L1)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadRightShoulder, Button::R1)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadLeftThumbstick, Button::THUMBSTICK_L)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadRightThumbstick, Button::THUMBSTICK_R)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadView, Button::SELECT)
        HANDLE_GAMEPAD_BUTTON_(GameInputGamepadMenu, Button::START)
        HANDLE_GAMEPAD_TRIGGER_(gs.leftTrigger, Button::L2)
        HANDLE_GAMEPAD_TRIGGER_(gs.rightTrigger, Button::R2)

        #undef HANDLE_GAMEPAD_TRIGGER_
        #undef HANDLE_GAMEPAD_BUTTON_

        // update thumbsticks
        HandleThumbstick(0, gs.leftThumbstickX, gs.leftThumbstickY);
        HandleThumbstick(1, gs.rightThumbstickX, gs.rightThumbstickY);

        return true;
    }


    void WindowsGamepad::OnDisconnected()
    {
        std::memset(button_data_, 0, sizeof(button_data_));
        std::memset(thumbstick_values_, 0, sizeof(thumbstick_values_));
    }


    constexpr void WindowsGamepad::SetButtonThreshold(const Button button, float threshold)
    {
        if (IsValidButton(button))
        {
            button_data_[static_cast<int>(button)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    constexpr void WindowsGamepad::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_BUTTON_CODES; i++) { button_data_[i].SetThreshold(threshold); }
    }

    constexpr float WindowsGamepad::GetButtonThreshold(const Button button) const
    {
        return IsValidButton(button)
            ? button_data_[static_cast<int>(button)].Threshold()
            : 0.0F;
    }

    constexpr float WindowsGamepad::GetButtonValue(const Button button) const
    {
        return (is_connected_ && IsValidButton(button))
            ? button_data_[static_cast<int>(button)].Value()
            : 0.0F;
    }

    constexpr bool WindowsGamepad::GetButtonState(const Button button, float &time) const
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

    constexpr uint32_t WindowsGamepad::GetThumbstickCount() const
    {
        return is_connected_ ? WindowsGamepad::NUM_THUMBSTICKS : 0;
    }

    constexpr void WindowsGamepad::GetThumbstick(const uint32_t index, float &x, float &y) const
    {
        std::tie(x, y) = (is_connected_ && index < WindowsGamepad::NUM_THUMBSTICKS)
            ? thumbstick_values_[index]
            : std::make_pair(0.0F, 0.0F);
    }


    #ifdef CROSSPUT_FEATURE_FORCE
    // RUMBLE FORCE
    void WindowsRumbleForce::SetActive(const bool active)
    {
        if (IsOrphaned() || active == is_active_) { return; }
        
        GameInputRumbleParams p = active
            ? TranslateRumbleParams(params_.rumble, p_device_->motor_gains_[0])
            : GameInputRumbleParams{}; // use "all zeros" to deactivate rumble
        
        p_device_->p_ndev_->SetRumbleState(&p);
        is_active_ = active;
    }


    bool WindowsRumbleForce::WriteParams()
    {
        const bool result = !IsOrphaned() && params_.type == ForceType::RUMBLE && is_active_;
        if (result)
        {
            GameInputRumbleParams p = TranslateRumbleParams(params_.rumble, p_device_->motor_gains_[0]);
            p_device_->p_ndev_->SetRumbleState(&p);
        }
        return result;
    }


    // SPECIAL FORCE
    ForceStatus WindowsSpecialForce::GetStatus() const
    {
        return (!IsOrphaned() && (p_ff_effect_->GetState() == GameInputFeedbackEffectState::GameInputFeedbackRunning))
            ? ForceStatus::ACTIVE
            : ForceStatus::INACTIVE;
    }


    void WindowsSpecialForce::SetActive(const bool active)
    {
        if (IsOrphaned()) { return; }

        if (active)
        {
            if (p_ff_effect_->GetState() == GameInputFeedbackEffectState::GameInputFeedbackRunning) { return; }

            // implicitly apply changes to parameters before activating force
            GameInputForceFeedbackParams p = TranslateForceFeedbackParams(params_);
            if (!p_ff_effect_->SetParams(&p)) { return; }
        }

        p_ff_effect_->SetState(active ? GameInputFeedbackEffectState::GameInputFeedbackRunning : GameInputFeedbackEffectState::GameInputFeedbackStopped);
    }


    bool WindowsSpecialForce::WriteParams()
    {
        if (!IsOrphaned() && type_ == params_.type)
        {
            GameInputForceFeedbackParams p = TranslateForceFeedbackParams(params_);
            return p_ff_effect_->SetParams(&p);
        }
        return false;        
    }
    #endif // CROSSPUT_FEATURE_FORCE
}
