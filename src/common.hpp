/*================================================================================= *
*                                   +----------+                                    *
*                                   | crossput |                                    *
*                                   +----------+                                    *
*                            Copyright 2024 Trice Helix                             *
*                                                                                   *
* This file is part of crossput and is distributed under the BSD-3-Clause License.  *
* Please refer to LICENSE.txt for additional information.                           *
* =================================================================================*/

#ifndef CROSSPUT_IMPL_COMMON_H
#define CROSSPUT_IMPL_COMMON_H

#include "crossput.hpp"

#include <algorithm>
#include <bit>
#include <bitset>
#include <cassert>
#include <chrono>
#include <climits>
#include <format>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <utility>


static_assert(CHAR_BIT == 8, "crossput requires characters to have an exact width of 8 bits");


#if _MSC_VER && !__INTEL_COMPILER
#define CROSSPUT_FUNCTION_STR __FUNCSIG__
#elif __GNUC__
#define CROSSPUT_FUNCTION_STR __PRETTY_FUNCTION__
#else
#define CROSSPUT_FUNCTION_STR __func__
#endif


// FNV-1a hash of a null-terminated string
size_t fnv1a_strhash(const char *const str);

// FNV-1a hash of memory contents
size_t fnv1a_bytehash(const void *const p_data, const size_t len);


namespace crossput
{
    class BaseInterface;
    using timestamp_t = uint64_t;

    // global runtime-unique ID counter, 0 is invalid
    extern ID::value_type glob_id_counter;
    inline ID ReserveID() { return { glob_id_counter++ }; }

    // all device interfaces, including aggregates
    extern std::unordered_map<ID, BaseInterface *> glob_interfaces;


    constexpr float TimestampDeltaSeconds(const timestamp_t first, const timestamp_t second) noexcept
    {
        return first != 0
            ? static_cast<float>(second - first) * 1E-6F // microsecond ticks -> seconds
            : std::numeric_limits<float>::infinity();
    }


    constexpr bool AnalogToDigital(const float value, const float threshold, const bool current_state) noexcept
    {
        // prevent rapid changes of digital state when analog value "bounces" around threshold
        constexpr float ANTI_BOUNCE = 0.025F;
        const float m = std::min(threshold, 1.0F - threshold) * ANTI_BOUNCE;
        return value > (current_state ? (threshold - m) : (threshold + m));
    }


    // timestamp, state (encoded), threshold, value
    struct TSTV
    {
        timestamp_t _a;
        float _b;
        float _c;

        constexpr timestamp_t Timestamp() const noexcept { return _a & TIMESTAMP_BITMASK; }
        constexpr void SetTimestamp(const timestamp_t timestamp) noexcept { _a = (_a & STATE_BITMASK) | (timestamp & TIMESTAMP_BITMASK); }

        constexpr bool State() const noexcept { return static_cast<bool>(_a & STATE_BITMASK); }
        constexpr void SetState(const bool state) noexcept { _a = (static_cast<timestamp_t>(state) << (sizeof(timestamp_t) * CHAR_BIT - 1)) | (_a & TIMESTAMP_BITMASK); }

        constexpr void SetTimestampState(const timestamp_t timestamp, const bool state) noexcept
        {
            _a = (static_cast<timestamp_t>(state) << (sizeof(timestamp_t) * CHAR_BIT - 1))
                | (timestamp & TIMESTAMP_BITMASK);
        }

        constexpr float Threshold() const noexcept { return _b; }
        constexpr void SetThreshold(const float threshold) noexcept { _b = threshold; }

        constexpr float Value() const noexcept { return _c; }
        constexpr void SetValue(const float value) noexcept { _c = value; }

        
        // returns true if underlying values were actually modified
        [[nodiscard]] constexpr bool Modify(const float new_value, const timestamp_t ts, bool &new_state) noexcept
        {
            const bool old_state = State();
            new_state = AnalogToDigital(new_value, Threshold(), old_state);
            const bool value_changed = new_value != Value();
            const bool state_changed = new_state != old_state;
            const bool force_write = Timestamp() == 0;
            
            if (state_changed || force_write) { SetTimestampState(ts, new_state); }
            if (value_changed || force_write) { SetValue(new_value); }

            return value_changed || state_changed || (force_write && new_state);
        }

        // returns true if the underlying values were actually modified
        // KBD VERSION
        [[nodiscard]] constexpr bool Modify(const float new_value, const timestamp_t ts, bool &new_state, auto &counter) noexcept
        {
            const bool old_state = State();
            new_state = AnalogToDigital(new_value, Threshold(), old_state);
            const bool value_changed = new_value != Value();
            const bool state_changed = new_state != old_state;
            const bool force_write = Timestamp() == 0;
            
            if (state_changed || force_write)
            {
                SetTimestampState(ts, new_state);
                if (new_state) { counter++; }
                else if (!force_write) { counter--; }
            }
            if (value_changed || force_write) { SetValue(new_value); }

            return value_changed || state_changed || (force_write && new_state);
        }
        
        constexpr void Modify(const float new_value, const timestamp_t ts) noexcept
        {
            const bool old_state = State();
            const bool new_state = AnalogToDigital(new_value, Threshold(), old_state);
            const bool force_write = Timestamp() == 0;
            
            if (new_state != old_state || force_write) { SetTimestampState(ts, new_state); }
            if (new_value != Value() || force_write) { SetValue(new_value); }
        }

        // KBD VERSION
        constexpr void Modify(const float new_value, const timestamp_t ts, auto &counter) noexcept
        {
            const bool old_state = State();
            const bool new_state = AnalogToDigital(new_value, Threshold(), old_state);
            const bool force_write = Timestamp() == 0;
            
            if (new_state != old_state || force_write)
            {
                SetTimestampState(ts, new_state);
                if (new_state) { counter++; }
                else if (!force_write) { counter--; }
            }
            if (new_value != Value() || force_write) { SetValue(new_value); }
        }

    private:
        static constexpr timestamp_t STATE_BITMASK = static_cast<timestamp_t>(1) << (sizeof(timestamp_t) * CHAR_BIT - 1);
        static constexpr timestamp_t TIMESTAMP_BITMASK = ~STATE_BITMASK;
    };


    struct MouseData
    {
        int64_t x;      // position X
        int64_t y;      // position Y
        int64_t dx;     // position delta X
        int64_t dy;     // position delta Y
        int64_t sx;     // scroll X
        int64_t sy;     // scroll Y
        int64_t sdx;    // scroll delta X
        int64_t sdy;    // scroll delta Y

        constexpr bool operator==(const MouseData &other) const noexcept
        {
            return x == other.x
                && y == other.y
                && dx == other.dx
                && dy == other.dy
                && sdx == other.sdx
                && sdy == other.sdy;
        }
    };


    // implements:
    // IDevice::GetID()
    // IDevice::IsConnected()
    // and provides basic ID and status functionality
    class BaseInterface : public virtual IDevice
    {
    protected:
        const ID id_;
        bool is_connected_ = false;

    public:
        constexpr ID GetID() const noexcept override final { return id_; }
        constexpr bool IsConnected() const override final { return is_connected_; }
        virtual ~BaseInterface() = default;

    protected:
        BaseInterface() : id_(ReserveID()) {}
    };


    template <typename T>
    inline size_t GetDevicesOfType(std::vector<T *> &devices, const DeviceType type, const bool ignore_disconnected)
    {
        size_t num = 0;
        for (const auto &elm : glob_interfaces)
        {
            if (elm.second->GetType() == type && (!ignore_disconnected || elm.second->IsConnected()))
            {
                devices.push_back(dynamic_cast<T *>(elm.second));
                num++;
            }
        }
        return num;
    }


    // implements:
    // IDevice::GetType()
    template <DeviceType TYPE>
    class TypedInterface : public virtual IDevice
    {
    public:
        constexpr DeviceType GetType() const noexcept final { return TYPE; }
    };
}


#ifdef CROSSPUT_FEATURE_CALLBACK
// => CALLBACK FEATURE ENABLED

namespace crossput
{
    struct EventCallbackKey
    {
        using filter_type = uint64_t;

        ID device_id;
        filter_type filter;
        unsigned short ctypeid;
        bool has_filter;

        inline bool operator==(const EventCallbackKey &other) const noexcept
        {
            return device_id == other.device_id
                && filter == other.filter
                && ctypeid == other.ctypeid
                && has_filter == other.has_filter;
        }

        inline bool operator!=(const EventCallbackKey &other) const noexcept
        {
            return !(operator==(other));
        }
    };


    // non-filtered callback
    template <typename TCallback>
    constexpr EventCallbackKey ConstructEventCallbackKey(const ID device_id) noexcept
    {
        return EventCallbackKey
        {
            .device_id = device_id,
            .filter = 0,
            .ctypeid = TCallback::CTYPEID,
            .has_filter = false
        };
    }


    // type which can be used as a callback filter
    template <typename T>
    concept EvcbFilterType = (std::is_integral_v<T> || std::is_enum_v<T>) && sizeof(T) <= sizeof(EventCallbackKey::filter_type);


    // filtered callback
    template <typename TCallback, EvcbFilterType T>
    constexpr EventCallbackKey ConstructEventCallbackKey(const ID device_id, const T value_filter) noexcept
    {
        return EventCallbackKey
        {
            .device_id = device_id,
            .filter = static_cast<EventCallbackKey::filter_type>(value_filter),
            .ctypeid = TCallback::CTYPEID,
            .has_filter = true
        };
    }
}


template <>
struct std::hash<crossput::EventCallbackKey>
{
    inline size_t operator()(const crossput::EventCallbackKey &key) const
    {
        const size_t h1 = std::hash<crossput::ID>().operator()(key.device_id);
        const size_t h2 = std::hash<crossput::EventCallbackKey::filter_type>().operator()(key.filter);
        const size_t h3 = std::hash<unsigned short>().operator()(key.ctypeid);
        const size_t h4 = std::hash<bool>().operator()(key.has_filter);
        return ((h1 * 23 + h2) * 17 + h3) * 23 + h4;
    }
};


namespace crossput
{
    class EventCallbackWrapper;
    class DeviceCallbackManagerImpl;
    class MouseCallbackManagerImpl;
    class KeyboardCallbackManagerImpl;
    class GamepadCallbackManagerImpl;

    using DeviceCallbackManager = DeviceCallbackManagerImpl;
    using MouseCallbackManager = MouseCallbackManagerImpl;
    using KeyboardCallbackManager = KeyboardCallbackManagerImpl;
    using GamepadCallbackManager = GamepadCallbackManagerImpl;


    extern bool glob_disable_management_api;
    extern std::unordered_map<ID, EventCallbackWrapper *> glob_callbacks;
    extern std::unordered_multimap<EventCallbackKey, ID> glob_callback_mapping;


    inline void ProtectManagementAPI(const char *const details_str)
    {
        if (glob_disable_management_api)
        {
            throw std::runtime_error(std::format("Illegal access to crossput management API from within a callback. Details: {}", details_str));
        }
    }

    inline void ProtectManagementAPI(const std::string details_str) { ProtectManagementAPI(details_str.c_str()); }


    class EventCallbackWrapper
    {
    public:
        template <typename TCallback, typename... TData>
        inline void Invoke(const typename TCallback::DevT *const p_device, const TData... data)
        {
            // block management functions during callback invocation to avoid dangerous errors caused by API user
            // (e.g. iterator invalidation, infinite loops, etc.)
            glob_disable_management_api = true;
            try
            {
                reinterpret_cast<const typename TCallback::FuncT *>(GetCallback())->operator()(p_device, data...);
            }
            catch (...)
            {
                // disable block in case user handles exception safely
                glob_disable_management_api = false;
                throw;
            }

            glob_disable_management_api = false;
        }

        virtual ~EventCallbackWrapper() = default;

    protected:
        virtual const void *GetCallback() const = 0;
    };


    template <typename TCallback>
    class EventCallbackWrapperImpl final : public EventCallbackWrapper
    {
    private:
        const typename TCallback::FuncT callback_;

    public:
        EventCallbackWrapperImpl(const typename TCallback::FuncT &&callback) : callback_(std::move(callback)) {}
        ~EventCallbackWrapperImpl() = default;

    private:
        const void *GetCallback() const override { return &callback_; }
    };


    template <typename TCallback>
    inline ID InsertCallback(const typename TCallback::FuncT &&callback, const ID device_id)
    {
        const ID id = ReserveID();
        glob_callbacks.insert({id, new EventCallbackWrapperImpl<TCallback>(std::move(callback))});
        glob_callback_mapping.insert({ConstructEventCallbackKey<TCallback>(device_id), id});
        return id;
    }


    template <typename TCallback, EvcbFilterType TValueFilter>
    inline ID InsertCallback(const typename TCallback::FuncT &&callback, const ID device_id, const TValueFilter value_filter)
    {
        const ID id = ReserveID();
        glob_callbacks.insert({id, new EventCallbackWrapperImpl<TCallback>(std::move(callback))});
        glob_callback_mapping.insert({ConstructEventCallbackKey<TCallback, TValueFilter>(device_id, value_filter), id});
        return id;
    }


    inline void EraseCallback(const ID id)
    {
        const auto it = glob_callbacks.find(id);
        if (it != glob_callbacks.end())
        {
            delete it->second;
            glob_callbacks.erase(it);
        }
    }


    template <typename TCallback, typename... TData>
    void ExecuteCallbacks_Impl(const EventCallbackKey &&key, const typename TCallback::DevT *const p_device, const TData... data)
    {
        // find all callbacks linked to the specific device and value filter
        auto [it, end] = glob_callback_mapping.equal_range(key);
        if (it == glob_callback_mapping.end()) { return; }

        do
        {
            const auto current = it++;
            if (const auto cbit = glob_callbacks.find(current->second); cbit != glob_callbacks.end())
            {
                // valid mapping, callback can be invoked
                cbit->second->template Invoke<TCallback, TData...>(p_device, data...);
            }
            else
            {
                // clean up dead mapping
                glob_callback_mapping.erase(current);
            }
        }
        while (it != end);
    }


    // execute global and device-specific callbacks using a valuefilter
    template <typename TCallback, EvcbFilterType TValueFilter, typename... TData>
    inline void ExecuteCallbacksWithFilter(const typename TCallback::DevT *const p_device, const TValueFilter value_filter, const TData... data)
    {
        // prioritize callbacks that are attached to the device
        ExecuteCallbacks_Impl<TCallback, TData...>(ConstructEventCallbackKey<TCallback, TValueFilter>(p_device->GetID(), value_filter), p_device, data...);
        ExecuteCallbacks_Impl<TCallback, TData...>(ConstructEventCallbackKey<TCallback, TValueFilter>({0}, value_filter), p_device, data...);
    }


    // execute global and device-specific callbacks
    template <typename TCallback, typename... TData>
    inline void ExecuteCallbacks(const typename TCallback::DevT *const p_device, const TData... data)
    {
        // prioritize callbacks that are attached to the device
        ExecuteCallbacks_Impl<TCallback, TData...>(ConstructEventCallbackKey<TCallback>(p_device->GetID()), p_device, data...);
        ExecuteCallbacks_Impl<TCallback, TData...>(ConstructEventCallbackKey<TCallback>({0}), p_device, data...);
    }


    inline void DeviceStatusChanged(const IDevice *const p_device, const DeviceStatusChange status)
    {
        // prioritize callbacks with a filter
        ExecuteCallbacksWithFilter<impl::_StatusCallback>(p_device, status, status);
        ExecuteCallbacks<impl::_StatusCallback>(p_device, status);
    }


    // implements:
    // IDevice::RegisterStatusCallback(...)
    // and provides basic callback management
    class DeviceCallbackManagerImpl : public virtual IDevice
    {
    protected:
        std::vector<ID> attached_callbacks_;

    public:
        ID RegisterStatusCallback(const StatusCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_StatusCallback>(std::move(callback));
        }

        ID RegisterStatusCallback(const StatusCallback &&callback, DeviceStatusChange type) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_StatusCallback>(std::move(callback), type);
        }

        virtual ~DeviceCallbackManagerImpl()
        {
            for (const ID cbid : attached_callbacks_)
            {
                EraseCallback(cbid);
            }
        }

    protected:
        DeviceCallbackManagerImpl()
        {
            attached_callbacks_.reserve(16);
        }

        template <typename TCallback>
        inline ID AttachCallback(const typename TCallback::FuncT &&callback)
        {
            const ID cbid = InsertCallback<TCallback>(std::move(callback), GetID());
            attached_callbacks_.push_back(cbid);
            return cbid;
        }

        template <typename TCallback, EvcbFilterType TValueFilter>
        inline ID AttachCallback(const typename TCallback::FuncT &&callback, const TValueFilter value_filter)
        {
            const ID cbid = InsertCallback<TCallback>(std::move(callback), GetID(), value_filter);
            attached_callbacks_.push_back(cbid);
            return cbid;
        }
    };


    // implements:
    // IMouse::RegisterMoveCallback(...)
    // IMouse::RegisterScrollCallback(...)
    // IMouse::RegisterButtonCallback(...)
    class MouseCallbackManagerImpl : public virtual DeviceCallbackManagerImpl, public virtual IMouse
    {
    public:
        ID RegisterMoveCallback(const MouseMoveCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_MouseMoveCallback>(std::move(callback));
        }

        ID RegisterScrollCallback(const MouseScrollCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_MouseScrollCallback>(std::move(callback));
        }

        ID RegisterButtonCallback(const MouseButtonCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_MouseButtonCallback>(std::move(callback));
        }

        ID RegisterButtonCallback(const MouseButtonCallback &&callback, const uint32_t index) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_MouseButtonCallback>(std::move(callback), index);
        }

        virtual ~MouseCallbackManagerImpl() = default;

    protected:
        inline void MouseMoved(const int64_t x, const int64_t y, const int64_t dx, const int64_t dy)
        {
            ExecuteCallbacks<impl::_MouseMoveCallback>(this, x, y, dx, dy);
        }

        inline void MouseScrolled(const int64_t x, const int64_t y, const int64_t dx, const int64_t dy)
        {
            ExecuteCallbacks<impl::_MouseScrollCallback>(this, x, y, dx, dy);
        }

        inline void ButtonChanged(const uint32_t index, const float value, const bool state)
        {
            // prioritize callbacks with a filter
            ExecuteCallbacksWithFilter<impl::_MouseButtonCallback>(this, index, index, value, state);
            ExecuteCallbacks<impl::_MouseButtonCallback>(this, index, value, state);
        }
    };


    // implements:
    // IKeyboard::RegisterKeyCallback(...)
    class KeyboardCallbackManagerImpl : public virtual DeviceCallbackManagerImpl, public virtual IKeyboard
    {
    public:
        ID RegisterKeyCallback(const KeyboardKeyCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_KeyboardKeyCallback>(std::move(callback));
        }

        ID RegisterKeyCallback(const KeyboardKeyCallback &&callback, const Key key) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_KeyboardKeyCallback>(std::move(callback), key);
        }

        virtual ~KeyboardCallbackManagerImpl() = default;

    protected:
        inline void KeyChanged(const Key key, const float value, const bool state)
        {
            // prioritize callbacks with a filter
            ExecuteCallbacksWithFilter<impl::_KeyboardKeyCallback>(this, key, key, value, state);
            ExecuteCallbacks<impl::_KeyboardKeyCallback>(this, key, value, state);
        }
    };


    // implements:
    // IGamepad::RegisterButtonCallback(...)
    // IGamepad::RegisterThumbstickCallback(...)
    class GamepadCallbackManagerImpl : public virtual DeviceCallbackManagerImpl, public virtual IGamepad
    {
    public:
        ID RegisterButtonCallback(const GamepadButtonCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_GamepadButtonCallback>(std::move(callback));
        }

        ID RegisterButtonCallback(const GamepadButtonCallback &&callback, const Button button) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_GamepadButtonCallback>(std::move(callback), button);
        }

        ID RegisterThumbstickCallback(const GamepadThumbstickCallback &&callback) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_GamepadThumbstickCallback>(std::move(callback));
        }

        ID RegisterThumbstickCallback(const GamepadThumbstickCallback &&callback, const uint32_t index) override final
        {
            ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
            return AttachCallback<impl::_GamepadThumbstickCallback>(std::move(callback), index);
        }

        virtual ~GamepadCallbackManagerImpl() = default;

    protected:
        inline void ButtonChanged(const Button button, const float value, const bool state)
        {
            // prioritize callbacks with a filter
            ExecuteCallbacksWithFilter<impl::_GamepadButtonCallback>(this, button, button, value, state);
            ExecuteCallbacks<impl::_GamepadButtonCallback>(this, button, value, state);
        }

        inline void ThumbstickChanged(const uint32_t index, const float x, const float y)
        {
            // prioritize callbacks with a filter
            ExecuteCallbacksWithFilter<impl::_GamepadThumbstickCallback>(this, index, index, x, y);
            ExecuteCallbacks<impl::_GamepadThumbstickCallback>(this, index, x, y);
        }
    };
}

#else // CROSSPUT_FEATURE_CALLBACK
// => CALLBACK FEATURE DISABLED

namespace crossput
{
    class DeviceCallbackManager {};
    class MouseCallbackManager {};
    class KeyboardCallbackManager {};
    class GamepadCallbackManager {};

    constexpr void ProtectManagementAPI(const char *const details_str) noexcept {}
    constexpr void ProtectManagementAPI(const std::string details_str) noexcept {}
}

#endif // CROSSPUT_FEATURE_CALLBACK

#ifdef CROSSPUT_FEATURE_FORCE
// => FORCE FEATURE ENABLED

namespace crossput
{
    template <typename TDevice>
    class DeviceForceManagerImpl;
    class DeviceForceManagerEmptyImpl;

    template <typename TDevice>
    using DeviceForceManager = DeviceForceManagerImpl<TDevice>;
    using DeviceForceManagerEmpty = DeviceForceManagerEmptyImpl;


    constexpr bool IsConditionForceType(const ForceType type) noexcept
    {
        // enum values must be sequential for this implementation
        static_assert((static_cast<int>(ForceType::SPRING)) + 1 == static_cast<int>(ForceType::FRICTION));
        static_assert((static_cast<int>(ForceType::FRICTION)) + 1 == static_cast<int>(ForceType::DAMPER));
        static_assert((static_cast<int>(ForceType::DAMPER)) + 1 == static_cast<int>(ForceType::INERTIA));
        return (static_cast<int>(ForceType::SPRING) <= static_cast<int>(type)) && (static_cast<int>(type) <= static_cast<int>(ForceType::INERTIA));
    }


    // implements:
    // IForce::GetID()
    // IForce::GetType()
    // IForce::GetDevice()
    // IForce::IsOrphaned()
    // IForce::GetMotorIndex()
    // IForce::Params()
    template <typename TDevice>
    class BaseForce : public virtual IForce
    {
        friend DeviceForceManagerImpl<TDevice>;

    protected:
        const ID id_;
        const uint32_t motor_index_;
        const ForceType type_;
        const bool is_condition_effect_;
        TDevice *p_device_;
        ForceParams params_ = {};

    public:
        BaseForce(TDevice *const p_device, const uint32_t motor_index, const ForceType type) :
            id_(glob_id_counter++),
            motor_index_(motor_index),
            type_(type),
            is_condition_effect_(IsConditionForceType(type)),
            p_device_(p_device)
        {
            params_.type = type;
        }

        constexpr ID GetID() const noexcept override final { return id_; }
        constexpr ForceType GetType() const noexcept override final { return type_; }
        constexpr IDevice *GetDevice() const noexcept override final { return p_device_; }
        constexpr bool IsOrphaned() const noexcept override final { return p_device_ == nullptr; }
        constexpr uint32_t GetMotorIndex() const override final { return motor_index_; }
        constexpr ForceParams &Params() noexcept override final { return params_; }

        virtual ~BaseForce() = default;
    };


    // implements:
    // IDevice::TryGetForce(...)
    // IDevice::DestroyForce(...)
    // IDevice::DestroyAllForces()
    // and provides basic force management
    template <typename TDevice>
    class DeviceForceManagerImpl : public virtual IDevice
    {
    protected:
        std::unordered_map<ID, BaseForce<TDevice> *> forces_;

    public:
        bool TryGetForce(const ID id, IForce *&p_force) const override final
        {
            const auto it = forces_.find(id);
            const bool result = it != forces_.end();
            if (result) { p_force = it->second; }
            return result;
        }

        void DestroyForce(const ID id) override final
        {
            const auto it = forces_.find(id);
            if (it != forces_.end())
            {
                delete it->second;
                forces_.erase(it);
            }
        }

        void DestroyAllForces() override final
        {
            for (const auto f : forces_)
            {
                delete f.second;
            }

            forces_.clear();
        }

        virtual ~DeviceForceManagerImpl()
        {
            for (const auto f : forces_)
            {
                delete f.second;
            }
        }

    protected:
        DeviceForceManagerImpl()
        {
            forces_.reserve(16);
        }

        inline void OrphanAllForces()
        {
            for (const auto f : forces_)
            {
                f.second->p_device_ = nullptr;
            }
        }
    };


    // implements all force management methods to be completely non-functional
    // (useful for classes that never have any force capabilities)
    class DeviceForceManagerEmptyImpl : public virtual IDevice
    {
    public:
        constexpr uint32_t GetMotorCount() const override final { return 0; }
        constexpr float GetGain([[maybe_unused]] const uint32_t motor_index) const override final { return 0.0F; }
        constexpr void SetGain([[maybe_unused]] const uint32_t motor_index, [[maybe_unused]] const float gain) override final {}
        constexpr bool SupportsForce([[maybe_unused]] const uint32_t motor_index, [[maybe_unused]] const ForceType type) const override final { return false; }
        constexpr bool TryGetForce([[maybe_unused]] const ID id, [[maybe_unused]] IForce *&p_force) const override final { return false; }
        constexpr void DestroyForce([[maybe_unused]] const ID id) override final {}

        constexpr bool TryCreateForce(
            [[maybe_unused]] const uint32_t motor_index,
            [[maybe_unused]] const ForceType type,
            [[maybe_unused]] IForce *&p_force) override final
        {
            p_force = nullptr;
            return false;
        }

    protected:
        DeviceForceManagerEmptyImpl() = default;
    };
}

#else // CROSSPUT_FEATURE_FORCE
// => FORCE FEATURE DISABLED

namespace crossput
{
    template <typename TDevice>
    class DeviceForceManager {};
    class DeviceForceManagerEmpty {};
}

#endif // CROSSPUT_FEATURE_FORCE

#ifdef CROSSPUT_FEATURE_AGGREGATE
namespace crossput
{
    // mapping between devices and aggregates they are a member of
    extern std::unordered_multimap<ID, ID> glob_dev_to_aggr;


    inline void LinkAggregate(const ID aggregate, const ID other)
    {
        // link aggregate with other device
        glob_dev_to_aggr.insert({other, aggregate});
    }


    inline void UnlinkAggregate(const ID aggregate, const ID other)
    {
        // unlink aggregate from other device
        auto [it, end] = glob_dev_to_aggr.equal_range(other);
        if (it != glob_dev_to_aggr.end())
        {
            do { if (it->second == aggregate) { glob_dev_to_aggr.erase(it); break; } }
            while (++it != end);
        }
    }


    // destroy hierarchy of interfaces by always prioritizing those which are not members of aggregates
    inline void DestroyHierarchy(std::vector<ID> &&targets)
    {
        std::vector<ID> remaining;
        remaining.reserve(targets.size());

        do
        {
            for (const ID id : targets)
            {
                if (!glob_dev_to_aggr.contains(id))
                {
                    const auto kv_linked = glob_interfaces.find(id);
                    if (kv_linked == glob_interfaces.end()) [[unlikely]] { continue; } // failsafe

                    #ifdef CROSSPUT_FEATURE_CALLBACK
                    DeviceStatusChanged(kv_linked->second, DeviceStatusChange::DESTROYED);
                    #endif // CROSSPUT_FEATURE_CALLBACK

                    delete kv_linked->second;
                    glob_interfaces.erase(kv_linked);
                }
                else
                {
                    remaining.push_back(id);
                }
            }

            if (remaining.size() == targets.size())
            {
                // likely user error
                throw std::runtime_error("Unknown error occurred in crossput. Please ensure no circular chain of aggregated input devices exists or any kind of undefined behaviour was caused.");
            }

            targets.clear();
            std::swap(targets, remaining);
        }
        while (targets.size() > 0);
    }


    inline timestamp_t GenericTimestampNow() noexcept
    {
        return static_cast<timestamp_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    }


    // implements:
    // IDevice::IsAggregate()
    // IDevice::GetDisplayName()
    // and stores aggregate members
    template <typename TDevice>
    requires std::derived_from<TDevice, IDevice>
    class AggregateImpl :
        public virtual BaseInterface,
        public virtual DeviceCallbackManager
    {
    protected:
        const std::unique_ptr<TDevice *[]> members_;
        const size_t member_count_;
        timestamp_t last_update_timestamp_ = 0;
        #ifdef CROSSPUT_FEATURE_FORCE
        std::unordered_map<ID, TDevice *> force_to_member_;
        std::vector<std::pair<TDevice *, uint32_t>> motor_to_member_;
        #endif // CROSSPUT_FEATURE_FORCE

    public:
        AggregateImpl(std::unique_ptr<TDevice *[]> &&members, const size_t member_count) :
            members_(std::move(members)),
            member_count_(member_count)
        {
            assert(member_count > 0);

            // create association between aggregate and members
            for (size_t i = 0; i < member_count; i++)
            {
                LinkAggregate(id_, members[i]->GetID());
            }
        }

        constexpr bool IsAggregate() const noexcept override final { return true; }

        std::string GetDisplayName() const override
        {
            if (!is_connected_) { return std::string(); }

            std::ostringstream name;

            // format: Aggregate{<name_0>;<name_1>;...,<name_x>}
            name << "Aggregate{";
            for (size_t i = 0; i < member_count_; i++) { name << members_[i]->GetDisplayName() << ';'; }
            name << "}";

            return name.str();
        }

        virtual void Update() override
        {
            ProtectManagementAPI(std::format("crossput::IDevice::Update() - Device ID {}", id_));

            bool connected = true;
            for (size_t i = 0; i < member_count_; i++)
            {
                members_[i]->Update();
                if (!members_[i]->IsConnected())
                {
                    connected = false;
                    break;
                }
            }

            if (connected != is_connected_)
            {
                is_connected_ = connected;
                if (!connected) // -> disconnected
                {
                    last_update_timestamp_ = 0;
                    #ifdef CROSSPUT_FEATURE_FORCE
                    motor_to_member_.clear();
                    #endif // CROSSPUT_FEATURE_FORCE

                    OnDisconnected();
                }
            }

            if (connected)
            {
                #ifdef CROSSPUT_FEATURE_FORCE
                // update motor mapping
                motor_to_member_.clear();
                for (size_t i = 0; i < member_count_; i++)
                {
                    const uint32_t c = members_[i]->GetMotorCount();
                    for (uint32_t j = 0; j < c; j++) { motor_to_member_.push_back({members_[i], j}); }
                }
                #endif // CROSSPUT_FEATURE_FORCE

                last_update_timestamp_ = GenericTimestampNow();
            }
        }

        #ifdef CROSSPUT_FEATURE_FORCE
        constexpr uint32_t GetMotorCount() const override final { return is_connected_ ? static_cast<uint32_t>(motor_to_member_.size()) : 0; }

        float GetGain(const uint32_t motor_index) const override final
        {
            if (is_connected_ && motor_index < motor_to_member_.size())
            {
                const auto &[p_member, real_motor] = motor_to_member_[motor_index];
                return p_member->GetGain(real_motor);
            }

            return 0.0F;
        }

        void SetGain(const uint32_t motor_index, float gain) override final
        {
            if (is_connected_ && motor_index < motor_to_member_.size())
            {
                const auto &[p_member, real_motor] = motor_to_member_[motor_index];
                p_member->SetGain(real_motor, gain);
            }
        }

        bool SupportsForce(const uint32_t motor_index, const ForceType type) const override final
        {
            if (is_connected_ && motor_index < motor_to_member_.size())
            {
                const auto &[p_member, real_motor] = motor_to_member_[motor_index];
                return p_member->SupportsForce(real_motor, type);
            }

            return false;
        }

        bool TryCreateForce(const uint32_t motor_index, const ForceType type, IForce *&p_force) override final
        {
            if (is_connected_ && motor_index < motor_to_member_.size())
            {
                const auto &[p_member, real_motor] = motor_to_member_[motor_index];
                if (p_member->TryCreateForce(real_motor, type, p_force))
                {
                    force_to_member_.insert({p_force->GetID(), p_member});
                    return true;
                }
            }

            return false;
        }

        bool TryGetForce(const ID id, IForce *&p_force) const override final
        {
            const auto it = force_to_member_.find(id);
            return (it != force_to_member_.end()) && it->second->TryGetForce(id, p_force);
        }

        void DestroyForce(const ID id) override final
        {
            const auto it = force_to_member_.find(id);
            if (it != force_to_member_.end())
            {
                it->second->DestroyForce(id);
                force_to_member_.erase(it);
            }
        }

        void DestroyAllForces()
        {
            for (auto &[id, p_member] : force_to_member_) { p_member->DestroyForce(id); }
            force_to_member_.clear();
        }
        #endif // CROSSPUT_FEATURE_FORCE

        virtual ~AggregateImpl()
        {
            // remove association between aggregate and members
            for (size_t i = 0; i < member_count_; i++)
            {
                UnlinkAggregate(id_, members_[i]->GetID());
            }
        }

    protected:
        virtual void OnDisconnected() = 0;
    };


    class AggregateMouse final :
        public virtual IMouse,
        public virtual AggregateImpl<IMouse>,
        public virtual TypedInterface<DeviceType::MOUSE>,
        public virtual MouseCallbackManager
    {
    private:
        struct MouseAccu
        {
            int64_t x;
            int64_t y;
            int64_t sx;
            int64_t sy;
            bool is_available;
        };

    private:
        MouseData data_ = {};
        std::unique_ptr<MouseAccu[]> prev_data_;
        std::vector<TSTV> button_data_;
        uint32_t button_count_ = 0;

    public:
        AggregateMouse(std::unique_ptr<IMouse *[]> &&members, const size_t member_count) :
            AggregateImpl<IMouse>(std::move(members), member_count)
        {
            prev_data_ = std::make_unique<MouseAccu[]>(member_count);
        }

        void Update() override;

        void GetPosition(int64_t &x, int64_t &y) const override;
        void GetDelta(int64_t &x, int64_t &y) const override;
        void GetScroll(int64_t &x, int64_t &y) const override;
        void GetScrollDelta(int64_t &x, int64_t &y) const override;
        uint32_t GetButtonCount() const override;
        void SetButtonThreshold(const uint32_t index, float threshold) override;
        void SetGlobalThreshold(float threshold) override;
        float GetButtonThreshold(const uint32_t index) const override;
        float GetButtonValue(const uint32_t index) const override;
        bool GetButtonState(const uint32_t index, float &time) const override;

    private:
        void OnDisconnected() override;
    };


    class AggregateKeyboard final :
        public virtual IKeyboard,
        public virtual AggregateImpl<IKeyboard>,
        public virtual TypedInterface<DeviceType::KEYBOARD>,
        public virtual KeyboardCallbackManager
    {
    private:
        TSTV key_data_[NUM_KEY_CODES] = {};
        uint32_t num_keys_pressed_ = 0;

    public:
        AggregateKeyboard(std::unique_ptr<IKeyboard *[]> &&members, const size_t member_count) :
            AggregateImpl<IKeyboard>(std::move(members), member_count)
            {}

        void Update() override;

        uint32_t GetNumKeysPressed() const override;
        void SetKeyThreshold(const Key key, float threshold) override;
        void SetGlobalThreshold(float threshold) override;
        float GetKeyThreshold(const Key key) const override;
        float GetKeyValue(const Key key) const override;
        bool GetKeyState(const Key key, float &time) const override;

    private:
        void OnDisconnected() override;
    };


    class AggregateGamepad final :
        public virtual IGamepad,
        public virtual AggregateImpl<IGamepad>,
        public virtual TypedInterface<DeviceType::GAMEPAD>,
        public virtual GamepadCallbackManager
    {
    private:
        TSTV button_data_[NUM_BUTTON_CODES] = {};
        std::vector<std::pair<float, float>> thumbstick_values_;
        uint32_t thumbstick_count_ = 0;

    public:
        AggregateGamepad(std::unique_ptr<IGamepad *[]> &&members, const size_t member_count) :
            AggregateImpl<IGamepad>(std::move(members), member_count)
            {}

        void Update() override;

        void SetButtonThreshold(const Button button, float threshold) override;
        void SetGlobalThreshold(float threshold) override;
        float GetButtonThreshold(const Button button) const override;
        float GetButtonValue(const Button button) const override;
        bool GetButtonState(const Button button, float &time) const override;
        uint32_t GetThumbstickCount() const override;
        void GetThumbstick(const uint32_t index, float &x, float &y) const override;

    private:
        void OnDisconnected() override;
    };
}
#endif // CROSSPUT_FEATURE_AGGREGATE

#endif // CROSSPUT_IMPL_COMMON_H
