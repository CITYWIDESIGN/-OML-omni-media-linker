#ifndef OMNI_MEDIA_LINKER_COMMON_H
#define OMNI_MEDIA_LINKER_COMMON_H

#include <cstdint>

#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#ifndef APPCOMMAND_MEDIA_NEXTTRACK
#define APPCOMMAND_MEDIA_NEXTTRACK 11
#endif

#ifndef APPCOMMAND_MEDIA_PREVIOUSTRACK
#define APPCOMMAND_MEDIA_PREVIOUSTRACK 12
#endif

#ifndef APPCOMMAND_MEDIA_STOP
#define APPCOMMAND_MEDIA_STOP 13
#endif

#ifndef APPCOMMAND_MEDIA_PLAY_PAUSE
#define APPCOMMAND_MEDIA_PLAY_PAUSE 14
#endif

#ifndef APPCOMMAND_VOLUME_MUTE
#define APPCOMMAND_VOLUME_MUTE 8
#endif

#ifndef APPCOMMAND_VOLUME_DOWN
#define APPCOMMAND_VOLUME_DOWN 9
#endif

#ifndef APPCOMMAND_VOLUME_UP
#define APPCOMMAND_VOLUME_UP 10
#endif

namespace oml
{
    struct WindowInfo
    {
        HWND hwnd{};
        std::wstring title;
        std::wstring className;
        std::wstring processName;
        DWORD processId{};
    };

    enum class MediaAction
    {
        PlayPause,
        NextTrack,
        PreviousTrack,
        Stop,
        VolumeUp,
        VolumeDown,
        Mute
    };

    enum class MatchMode
    {
        TitleExact = 1,
        TitleContains = 2,
        ProcessName = 3,
        ProcessNameAndClass = 4
    };

    struct Hotkey
    {
        UINT modifiers{};
        UINT virtualKey{};
    };

    struct Binding
    {
        int id{};
        HWND hwnd{};
        std::wstring windowTitle;
        std::wstring windowClass;
        std::wstring processName;
        MatchMode matchMode{MatchMode::TitleExact};
        MediaAction action{MediaAction::PlayPause};
        Hotkey hotkey{};
        bool enabled{true};
    };

    std::wstring trim(const std::wstring &value);

    std::wstring action_to_string(MediaAction action);

    bool action_from_string(const std::wstring &value, MediaAction &action);

    std::wstring match_mode_to_string(MatchMode match_mode);

    bool match_mode_from_int(int value, MatchMode &match_mode);

    std::wstring hotkey_to_string(const Hotkey &hotkey);

    std::wstring hwnd_to_string(HWND hwnd);

    bool hwnd_from_string(const std::wstring &value, HWND &hwnd);

    std::string to_utf8(const std::wstring &value);
}

#endif
