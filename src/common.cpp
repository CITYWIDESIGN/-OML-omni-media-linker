#include "common.h"

#include <algorithm>
#include <cwctype>
#include <sstream>

namespace oml
{
    std::wstring trim(const std::wstring &value)
    {
        const auto left = std::find_if_not(value.begin(), value.end(), iswspace);
        const auto right = std::find_if_not(value.rbegin(), value.rend(), iswspace).base();

        if (left >= right)
        {
            return L"";
        }

        return std::wstring(left, right);
    }

    std::wstring action_to_string(MediaAction action)
    {
        switch (action)
        {
        case MediaAction::PlayPause:
            return L"play_pause";
        case MediaAction::NextTrack:
            return L"next_track";
        case MediaAction::PreviousTrack:
            return L"previous_track";
        case MediaAction::Stop:
            return L"stop";
        case MediaAction::VolumeUp:
            return L"volume_up";
        case MediaAction::VolumeDown:
            return L"volume_down";
        case MediaAction::Mute:
            return L"mute";
        }

        return L"unknown";
    }

    bool action_from_string(const std::wstring &value, MediaAction &action)
    {
        const std::wstring normalized = trim(value);

        if (normalized == L"play_pause")
        {
            action = MediaAction::PlayPause;
            return true;
        }

        if (normalized == L"next_track")
        {
            action = MediaAction::NextTrack;
            return true;
        }

        if (normalized == L"previous_track")
        {
            action = MediaAction::PreviousTrack;
            return true;
        }

        if (normalized == L"stop")
        {
            action = MediaAction::Stop;
            return true;
        }

        if (normalized == L"volume_up")
        {
            action = MediaAction::VolumeUp;
            return true;
        }

        if (normalized == L"volume_down")
        {
            action = MediaAction::VolumeDown;
            return true;
        }

        if (normalized == L"mute")
        {
            action = MediaAction::Mute;
            return true;
        }

        return false;
    }

    std::wstring match_mode_to_string(MatchMode match_mode)
    {
        switch (match_mode)
        {
        case MatchMode::TitleExact:
            return L"标题精确";
        case MatchMode::TitleContains:
            return L"标题包含";
        case MatchMode::ProcessName:
            return L"进程名";
        case MatchMode::ProcessNameAndClass:
            return L"进程名+类名";
        }

        return L"未知";
    }

    bool match_mode_from_int(int value, MatchMode &match_mode)
    {
        switch (value)
        {
        case 1:
            match_mode = MatchMode::TitleExact;
            return true;
        case 2:
            match_mode = MatchMode::TitleContains;
            return true;
        case 3:
            match_mode = MatchMode::ProcessName;
            return true;
        case 4:
            match_mode = MatchMode::ProcessNameAndClass;
            return true;
        default:
            return false;
        }
    }

    std::wstring hotkey_to_string(const Hotkey &hotkey)
    {
        std::wstring result;

        if ((hotkey.modifiers & MOD_CONTROL) != 0)
        {
            result += L"Ctrl+";
        }

        if ((hotkey.modifiers & MOD_ALT) != 0)
        {
            result += L"Alt+";
        }

        if ((hotkey.modifiers & MOD_SHIFT) != 0)
        {
            result += L"Shift+";
        }

        if ((hotkey.modifiers & MOD_WIN) != 0)
        {
            result += L"Win+";
        }

        const UINT scan_code = MapVirtualKeyW(hotkey.virtualKey, MAPVK_VK_TO_VSC) << 16;
        wchar_t key_name[128]{};

        GetKeyNameTextW(static_cast<LONG>(scan_code), key_name, static_cast<int>(std::size(key_name)));

        if (wcslen(key_name) == 0)
        {
            std::wstringstream stream;

            stream << L"VK_" << hotkey.virtualKey;
            result += stream.str();
        }
        else
        {
            result += key_name;
        }

        return result;
    }

    std::wstring hwnd_to_string(HWND hwnd)
    {
        std::wstringstream stream;

        stream << reinterpret_cast<std::uintptr_t>(hwnd);

        return stream.str();
    }

    bool hwnd_from_string(const std::wstring &value, HWND &hwnd)
    {
        try
        {
            const auto raw_value = static_cast<std::uintptr_t>(wcstoull(value.c_str(), nullptr, 10));

            hwnd = reinterpret_cast<HWND>(raw_value);

            return hwnd != nullptr;
        }
        catch (...)
        {
            hwnd = nullptr;
            return false;
        }
    }

    std::string to_utf8(const std::wstring &value)
    {
        if (value.empty())
        {
            return "";
        }

        const int size = WideCharToMultiByte(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        std::string result(size, '\0');

        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.c_str(),
            static_cast<int>(value.size()),
            &result[0],
            size,
            nullptr,
            nullptr);

        return result;
    }
}
