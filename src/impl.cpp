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

#include <cassert>
#include <cmath>
#include <concepts>
#include <cstring>
#include <numeric>
#include <unordered_set>


size_t fnv1a_strhash(const char *str)
{
    if constexpr (sizeof(size_t) > 4)
    {
        // 64-bit
        size_t hash = 0xcbf29ce484222325U;
        char c;
        size_t i = 0;
        while ((c = str[i]) != '\0')
        {
            hash = hash ^ c;
            hash *= 0x100000001B3U;
            i++;
        }

        return hash;
    }
    else
    {
        // 32-bit
        size_t hash = 0x811C9DC5U;
        char c;
        size_t i = 0;
        while ((c = str[i]) != '\0')
        {
            hash = hash ^ c;
            hash *= 0x1000193U;
        }

        return hash;
    }
}


size_t fnv1a_bytehash(const void *const p_data, const size_t len)
{
    const char *const p_data_chars = reinterpret_cast<const char *>(p_data);

    if constexpr (sizeof(size_t) > 4)
    {
        // 64-bit
        size_t hash = 0xCBF29CE484222325U;
        for (size_t i = 0; i < len; i++)
        {
            hash = hash ^ p_data_chars[i];
            hash *= 0x100000001B3U;
        }

        return hash;
    }
    else
    {
        // 32-bit
        size_t hash = 0x811C9DC5U;
        for (size_t i = 0; i < len; i++)
        {
            hash = hash ^ p_data_chars[i];
            hash *= 0x1000193U;
        }

        return hash;
    }
}


namespace crossput
{
    ID::value_type glob_id_counter = 1;
    std::unordered_map<ID, BaseInterface *> glob_interfaces;

    #ifdef CROSSPUT_FEATURE_CALLBACK
    bool glob_disable_management_api = false;
    std::unordered_map<ID, EventCallbackWrapper *> glob_callbacks;
    std::unordered_multimap<EventCallbackKey, ID> glob_callback_mapping;
    #endif // CROSSPUT_FEATURE_CALLBACK

    #ifdef CROSSPUT_FEATURE_AGGREGATE
    std::unordered_multimap<ID, ID> glob_dev_to_aggr;
    #endif // CROSSPUT_FEATURE_AGGREGATE


    // GLOBAL DEVICE MANAGEMENT

    void UpdateAllDevices()
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        #ifdef CROSSPUT_FEATURE_AGGREGATE
        for (auto &elm : glob_interfaces)
        {
            // only update devices which are not members of any aggregate to reduce overall Update() calls
            // (aggregates update their members anyway)
            if (!glob_dev_to_aggr.contains(elm.second->GetID()))
            {
                elm.second->Update();
            }
        }
        #else
        for (auto &elm : glob_interfaces)
        {
            elm.second->Update();
        }
        #endif // CROSSPUT_FEATURE_AGGREGATE
    }


    void DestroyAllDevices()
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        if (glob_interfaces.size() == 0) { return; }

        #ifdef CROSSPUT_FEATURE_AGGREGATE
        std::vector<ID> targets;
        targets.reserve(glob_interfaces.size());
        for (auto &elm : glob_interfaces) { targets.push_back(elm.second->GetID()); }

        DestroyHierarchy(std::move(targets));

        glob_interfaces.clear();
        glob_dev_to_aggr.clear();
        #else
        for (auto &elm : glob_interfaces)
        {
            #ifdef CROSSPUT_FEATURE_CALLBACK
            DeviceStatusChanged(elm.second, DeviceStatusChange::DESTROYED);
            #endif // CROSSPUT_FEATURE_CALLBACK

            delete elm.second;
        }

        glob_interfaces.clear();
        #endif // CROSSPUT_FEATURE_AGGREGATE
    }


    size_t GetDeviceCount(const bool ignore_disconnected)
    {
        if (!ignore_disconnected)
        {
            return glob_interfaces.size();
        }

        size_t num = 0;
        for (auto &elm : glob_interfaces)
        {
            if (elm.second->IsConnected()) { num++; }
        }
        return num;
    }


    size_t GetDevices(std::vector<IDevice *> &devices, const bool ignore_disconnected)
    {
        if (!ignore_disconnected)
        {
            // shortcut
            devices.reserve(devices.size() + glob_interfaces.size());
            for (auto &elm : glob_interfaces) { devices.push_back(dynamic_cast<IDevice *>(elm.second)); }
            return glob_interfaces.size();
        }

        size_t num = 0;
        for (auto &elm : glob_interfaces)
        {
            if (elm.second->IsConnected())
            {
                devices.push_back(dynamic_cast<IDevice *>(elm.second));
                num++;
            }
        }

        return num;
    }


    size_t GetMice(std::vector<IMouse *> &mice, const bool ignore_disconnected)
    {
        return GetDevicesOfType<IMouse>(mice, DeviceType::MOUSE, ignore_disconnected);
    }


    size_t GetKeyboards(std::vector<IKeyboard *> &keyboards, const bool ignore_disconnected)
    {
        return GetDevicesOfType<IKeyboard>(keyboards, DeviceType::KEYBOARD, ignore_disconnected);
    }


    size_t GetGamepads(std::vector<IGamepad *> &gamepads, const bool ignore_disconnected)
    {
        return GetDevicesOfType<IGamepad>(gamepads, DeviceType::GAMEPAD, ignore_disconnected);
    }


    IDevice *GetDevice(const ID id)
    {
        if (id.value != 0)
        {
            const auto kv = glob_interfaces.find(id);
            if (kv != glob_interfaces.end()) { return kv->second; }
        }
        
        return nullptr;
    }


    void DestroyDevice(const ID id)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        if (id.value == 0) { return; }

        // try to find main target
        const auto kv_main = glob_interfaces.find(id);
        if (kv_main == glob_interfaces.end()) { return; }

        #ifdef CROSSPUT_FEATURE_AGGREGATE
        if (!glob_dev_to_aggr.contains(kv_main->second->GetID())) // shortcut
        {
            #ifdef CROSSPUT_FEATURE_CALLBACK
            DeviceStatusChanged(kv_main->second, DeviceStatusChange::DESTROYED);
            #endif // CROSSPUT_FEATURE_CALLBACK

            delete kv_main->second;
            glob_interfaces.erase(kv_main);
            return;
        }

        std::vector<ID> targets;
        std::vector<ID> stack;
        targets.reserve(16);
        stack.reserve(16);
        targets.push_back(id);
        stack.push_back(id);

        // recursively gather all interface IDs which must be destroyed
        // (aggregates are destroyed as soon as any of their targets are)
        ID current;
        do
        {
            current = stack.back();
            stack.pop_back();

            auto [it, end] = glob_dev_to_aggr.equal_range(current);
            if (it != glob_dev_to_aggr.end())
            {
                do
                {
                    targets.push_back(it->second);
                    stack.push_back(it->second);
                }
                while (++it != end);
            }
        }
        while (stack.size() > 0);

        DestroyHierarchy(std::move(targets));
        #else
        #ifdef CROSSPUT_FEATURE_CALLBACK
        DeviceStatusChanged(kv_main->second, DeviceStatusChange::DESTROYED);
        #endif // CROSSPUT_FEATURE_CALLBACK

        delete kv_main->second;
        glob_interfaces.erase(kv_main);
        #endif // CROSSPUT_FEATURE_AGGREGATE
    }


    #ifdef CROSSPUT_FEATURE_CALLBACK
    void UnregisterCallback(const ID id)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        EraseCallback(id);
    }

    void UnregisterAllCallbacks()
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        glob_callbacks.clear();
    }

    ID RegisterGlobalStatusCallback(const StatusCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_StatusCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalStatusCallback(const StatusCallback &&callback, const DeviceStatusChange type)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_StatusCallback>(std::move(callback), {0}, type);
    }

    ID RegisterGlobalMouseMoveCallback(const MouseMoveCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_MouseMoveCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalMouseScrollCallback(const MouseScrollCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_MouseScrollCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalMouseButtonCallback(const MouseButtonCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_MouseButtonCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalMouseButtonCallback(const MouseButtonCallback &&callback, const uint32_t index)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_MouseButtonCallback>(std::move(callback), {0}, index);
    }

    ID RegisterGlobalKeyboardKeyCallback(const KeyboardKeyCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_KeyboardKeyCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalKeyboardKeyCallback(const KeyboardKeyCallback &&callback, const Key key)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_KeyboardKeyCallback>(std::move(callback), {0}, key);
    }

    ID RegisterGlobalGamepadButtonCallback(const GamepadButtonCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_GamepadButtonCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalGamepadButtonCallback(const GamepadButtonCallback &&callback, const Button button)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_GamepadButtonCallback>(std::move(callback), {0}, button);
    }

    ID RegisterGlobalGamepadThumbstickCallback(const GamepadThumbstickCallback &&callback)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_GamepadThumbstickCallback>(std::move(callback), {0});
    }

    ID RegisterGlobalGamepadThumbstickCallback(const GamepadThumbstickCallback &&callback, const uint32_t index)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);
        return InsertCallback<impl::_GamepadThumbstickCallback>(std::move(callback), {0}, index);
    }
    #endif // CROSSPUT_FEATURE_CALLBACK


    #ifdef CROSSPUT_FEATURE_AGGREGATE
    IDevice *Aggregate(const std::vector<ID> &ids, const DeviceType hint_type)
    {
        ProtectManagementAPI(CROSSPUT_FUNCTION_STR);

        const size_t member_count = ids.size();
        if (member_count == 0) { return nullptr; }

        if (member_count == 1)
        {
            // single target -> do not create aggregate
            IDevice *const p_target = GetDevice(ids[0]);
            return (p_target != nullptr && (hint_type == DeviceType::UNKNOWN || p_target->GetType() == hint_type))
                ? p_target
                : nullptr; // target does not exist or hint type mismatch
        }

        // check if aggregate already exists
        std::unordered_set<ID> aggr_candidates;
        std::unordered_set<ID> aggr_cache;
        aggr_candidates.reserve(16);
        aggr_cache.reserve(16);
        for (size_t i = 0; i < member_count; i++)
        {
            auto [it, end] = glob_dev_to_aggr.equal_range(ids[i]);
            if (it == glob_dev_to_aggr.end())
            {
                // unique aggregation
                aggr_candidates.clear();
                break;
            }

            if (i == 0)
            {
                // gather initial candidates
                do { aggr_candidates.insert(it->second); }
                while (++it != end);
            }
            else
            {
                // eliminate candidates
                aggr_cache.clear();
                do { if (aggr_candidates.contains(it->second)) { aggr_cache.insert(it->second); } }
                while (++it != end);

                if (aggr_cache.size() == 0) { break; } // unique aggregation
                std::swap(aggr_candidates, aggr_cache);
            }
        }

        if (aggr_candidates.size() == 1)
        {
            IDevice *const p_existing = GetDevice(*(aggr_candidates.cbegin()));
            if (p_existing != nullptr) { return p_existing; } // found existing aggregate
        }

        // get member pointers
        auto p_members = std::make_unique_for_overwrite<IDevice *[]>(member_count);
        for (size_t i = 0; i < member_count; i++)
        {
            IDevice *const p_device = GetDevice(ids[i]);
            if (p_device == nullptr) { return nullptr; }
            p_members[i] = p_device;
        }

        // ensure all targets are the same type
        const DeviceType t = (hint_type == DeviceType::UNKNOWN) ? p_members[0]->GetType() : hint_type;
        for (size_t i = 0; i < member_count; i++)
        {
            if (t != p_members[i]->GetType())
            {
                // not all target types match
                return nullptr;
            }
        }

        BaseInterface *p_aggregate = nullptr;
        switch (t)
        {
        case DeviceType::MOUSE:
            {
                auto p_mice = std::make_unique<IMouse *[]>(member_count);
                for (size_t i = 0; i < member_count; i++) { p_mice[i] = dynamic_cast<IMouse *>(p_members[i]); }
                p_aggregate = new AggregateMouse(std::move(p_mice), member_count);
            }
            break;

        case DeviceType::KEYBOARD:
            {
                auto p_keyboards = std::make_unique<IKeyboard *[]>(member_count);
                for (size_t i = 0; i < member_count; i++) { p_keyboards[i] = dynamic_cast<IKeyboard *>(p_members[i]); }
                p_aggregate = new AggregateKeyboard(std::move(p_keyboards), member_count);
            }
            break;

        case DeviceType::GAMEPAD:
            {
                auto p_gamepads = std::make_unique<IGamepad *[]>(member_count);
                for (size_t i = 0; i < member_count; i++) { p_gamepads[i] = dynamic_cast<IGamepad *>(p_members[i]); }
                p_aggregate = new AggregateGamepad(std::move(p_gamepads), member_count);
            }
            break;

        default:
            // wtf?
            throw std::runtime_error(std::format("Fatal error occurred during device aggregation - Unexpected crossput::DeviceType value {}", static_cast<int>(t)));
        }

        // register
        const ID id = p_aggregate->GetID();
        glob_interfaces.insert({id, p_aggregate});

        return p_aggregate;
    }


    // AGGREGATE MOUSE

    void AggregateMouse::Update()
    {
        AggregateImpl<IMouse>::Update();
        if (!is_connected_) { return; }

        // get maximum number of addressable buttons
        uint32_t bc = 0;
        for (size_t i = 0; i < member_count_; i++)
        {
            bc = std::max(bc, members_[i]->GetButtonCount());
        }

        if (bc != button_count_) [[unlikely]] // button count rarely changes
        {
            button_count_ = bc;
            button_data_.clear();
            button_data_.resize(bc);
        }

        std::unique_ptr<float[]> new_values = std::make_unique<float[]>(bc);

        int64_t dx = 0, dy = 0, sdx = 0, sdy = 0;
        for (size_t i = 0; i < member_count_; i++)
        {
            IMouse *p_member = members_[i];
            MouseAccu &accu = prev_data_[i];

            int64_t x, y, sx, sy;
            p_member->GetPosition(x, y);
            p_member->GetScroll(sx, sy);

            if (accu.is_available) [[likely]]
            {
                // accumulate delta of all members
                dx += x - accu.x;
                dy += y - accu.y;
                sdx += sx - accu.sx;
                sdy += sy - accu.sy;
            }
            else
            {
                accu.is_available = true;
            }

            accu.x = x;
            accu.y = y;
            accu.sx = sx;
            accu.sy = sy;

            // find max analog button values
            const uint32_t maxb = std::min(bc, p_member->GetButtonCount());
            for (uint32_t b = 0; b < maxb; b++)
            {
                new_values[b] = std::max(p_member->GetButtonValue(b), button_data_[b].Value());
            }
        }

        // update mouse data
        data_.x += dx;
        data_.y += dy;
        data_.dx = dx;
        data_.dy = dy;
        data_.sx += sdx;
        data_.sy += sdy;
        data_.sdx = sdx;
        data_.sdy = sdy;

        #ifdef CROSSPUT_FEATURE_CALLBACK
        if (dx != 0 || dy != 0) { MouseMoved(data_.x, data_.y, dx, dy); }
        if (sdx != 0 || sdy != 0) { MouseScrolled(data_.sx, data_.sy, sdx, sdy); }
        #endif // CROSSPUT_FEATURE_CALLBACK

        // update button data
        for (uint32_t b = 0; b < bc; b++)
        {
            const float value = new_values[b];
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (button_data_[b].Modify(value, last_update_timestamp_, state)) { ButtonChanged(b, value, state); }
            #else
            button_data_[b].Modify(value, last_update_timestamp_);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void AggregateMouse::OnDisconnected()
    {
        std::memset(prev_data_.get(), 0, sizeof(MouseAccu) * member_count_);
        button_data_.clear();
        button_count_ = 0;
    }


    void AggregateMouse::GetPosition(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.x;
        y = data_.y;
    }

    void AggregateMouse::GetDelta(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.dx;
        y = data_.dy;
    }

    void AggregateMouse::GetScroll(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.sx;
        y = data_.sy;
    }

    void AggregateMouse::GetScrollDelta(int64_t &x, int64_t &y) const
    {
        if (!is_connected_) { return; }
        x = data_.sdx;
        y = data_.sdy;
    }

    uint32_t AggregateMouse::GetButtonCount() const
    {
        return is_connected_ ? button_count_ : 0;
    }

    void AggregateMouse::SetButtonThreshold(const uint32_t index, float threshold)
    {
        if (index < button_count_)
        {
            button_data_[index].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    void AggregateMouse::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < button_count_; i++) { button_data_[i].SetThreshold(threshold); }
    }

    float AggregateMouse::GetButtonThreshold(const uint32_t index) const
    {
        return index < button_count_
            ? button_data_[index].Threshold()
            : 0.0F;
    }

    float AggregateMouse::GetButtonValue(const uint32_t index) const
    {
        return (is_connected_ && index < button_count_)
            ? button_data_[index].Value()
            : 0.0F;
    }

    bool AggregateMouse::GetButtonState(const uint32_t index, float &time) const
    {
        if (is_connected_ && index < button_count_)
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


    // AGGREGATE KEYBOARD

    void AggregateKeyboard::Update()
    {
        AggregateImpl<IKeyboard>::Update();
        if (!is_connected_) { return; }

        float new_values[NUM_KEY_CODES] = {};

        for (size_t i = 0; i < member_count_; i++)
        {
            const IKeyboard *const p_member = members_[i];

            // find maximum analog values of each key
            for (unsigned int k = 0; k < NUM_KEY_CODES; k++)
            {
                new_values[k] = std::max(new_values[k], p_member->GetKeyValue(static_cast<Key>(k)));
            }
        }

        // update key data
        for (unsigned int k = 0; k < NUM_KEY_CODES; k++)
        {
            const float value = new_values[k];
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (key_data_[k].Modify(value, last_update_timestamp_, state, num_keys_pressed_)) { KeyChanged(static_cast<Key>(k), value, state); }
            #else
            key_data_[k].Modify(value, last_update_timestamp_, num_keys_pressed_);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void AggregateKeyboard::OnDisconnected()
    {
        std::memset(key_data_, 0, sizeof(key_data_));
        num_keys_pressed_ = 0;
    }


    uint32_t AggregateKeyboard::GetNumKeysPressed() const
    {
        return is_connected_ ? num_keys_pressed_ : 0;
    }

    void AggregateKeyboard::SetKeyThreshold(const Key key, float threshold)
    {
        if (IsValidKey(key))
        {
            key_data_[static_cast<int>(key)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    void AggregateKeyboard::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_KEY_CODES; i++) { key_data_[i].SetThreshold(threshold); }
    }

    float AggregateKeyboard::GetKeyThreshold(const Key key) const
    {
        return IsValidKey(key)
            ? key_data_[static_cast<int>(key)].Threshold()
            : 0.0F;
    }

    float AggregateKeyboard::GetKeyValue(const Key key) const
    {
        return (is_connected_ && IsValidKey(key))
            ? key_data_[static_cast<int>(key)].Value()
            : 0.0F;
    }

    bool AggregateKeyboard::GetKeyState(const Key key, float &time) const
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


    // AGGREGATE GAMEPAD

    void AggregateGamepad::Update()
    {
        AggregateImpl<IGamepad>::Update();
        if (!is_connected_) { return; }

        uint32_t tc = 0;
        for (size_t i = 0; i < member_count_; i++)
        {
            tc += members_[i]->GetThumbstickCount();
        }

        // reset when number of thumbsticks changes
        const bool th_reset = tc != thumbstick_count_;
        if (th_reset) [[unlikely]]
        {
            thumbstick_count_ = tc;
            thumbstick_values_.clear();
            thumbstick_values_.resize(tc);
        }

        float new_values[NUM_BUTTON_CODES] = {};

        uint32_t th_index = 0;
        for (size_t i = 0; i < member_count_; i++)
        {
            const IGamepad *const p_member = members_[i];

            // update thumbstick data
            const uint32_t th_count = p_member->GetThumbstickCount();
            for (uint32_t t = 0; t < th_count; t++)
            {
                float x, y;
                p_member->GetThumbstick(t, x, y);
                auto &tval = thumbstick_values_[th_index];

                #ifdef CROSSPUT_FEATURE_CALLBACK
                if (x != tval.first || y != tval.second || th_reset) { ThumbstickChanged(th_index, x, y); }
                #endif // CROSSPUT_FEATURE_CALLBACK

                tval = {x, y};
                th_index++;
            }

            // find maximum analog values of each button
            for (unsigned int b = 0; b < NUM_BUTTON_CODES; b++)
            {
                new_values[b] = std::max(new_values[b], p_member->GetButtonValue(static_cast<Button>(b)));
            }
        }

        // update button data
        for (unsigned int b = 0; b < NUM_BUTTON_CODES; b++)
        {
            const float value = new_values[b];
            #ifdef CROSSPUT_FEATURE_CALLBACK
            bool state;
            if (button_data_[b].Modify(value, last_update_timestamp_, state)) { ButtonChanged(static_cast<Button>(b), value, state); }
            #else
            button_data_[b].Modify(value, last_update_timestamp_);
            #endif // CROSSPUT_FEATURE_CALLBACK
        }
    }


    void AggregateGamepad::OnDisconnected()
    {
        std::memset(button_data_, 0, sizeof(button_data_));
        thumbstick_values_.clear();
        thumbstick_count_ = 0;
    }


    void AggregateGamepad::SetButtonThreshold(const Button button, float threshold)
    {
        if (IsValidButton(button))
        {
            button_data_[static_cast<int>(button)].SetThreshold(std::clamp(threshold, 0.0F, 1.0F));
        }
    }

    void AggregateGamepad::SetGlobalThreshold(float threshold)
    {
        threshold = std::clamp(threshold, 0.0F, 1.0F);
        for (unsigned int i = 0; i < NUM_BUTTON_CODES; i++) { button_data_[i].SetThreshold(threshold); }
    }

    float AggregateGamepad::GetButtonThreshold(const Button button) const
    {
        return IsValidButton(button)
            ? button_data_[static_cast<int>(button)].Threshold()
            : 0.0F;
    }

    float AggregateGamepad::GetButtonValue(const Button button) const
    {
        return (is_connected_ && IsValidButton(button))
            ? button_data_[static_cast<int>(button)].Value()
            : 0.0F;
    }

    bool AggregateGamepad::GetButtonState(const Button button, float &time) const
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

    uint32_t AggregateGamepad::GetThumbstickCount() const
    {
        return is_connected_ ? thumbstick_count_ : 0;
    }

    void AggregateGamepad::GetThumbstick(const uint32_t index, float &x, float &y) const
    {
        std::tie(x, y) = (is_connected_ && index < thumbstick_count_)
            ? thumbstick_values_[index]
            : std::make_pair(0.0F, 0.0F);
    }
   #endif // CROSSPUT_FEATURE_AGGREGATE
}
