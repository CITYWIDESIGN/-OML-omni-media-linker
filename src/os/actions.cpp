#include "os/actions.h"

namespace oml
{
    namespace
    {
        LPARAM action_to_app_command(MediaAction action)
        {
            switch (action)
            {
            case MediaAction::PlayPause:
                return MAKELPARAM(0, APPCOMMAND_MEDIA_PLAY_PAUSE);
            case MediaAction::NextTrack:
                return MAKELPARAM(0, APPCOMMAND_MEDIA_NEXTTRACK);
            case MediaAction::PreviousTrack:
                return MAKELPARAM(0, APPCOMMAND_MEDIA_PREVIOUSTRACK);
            case MediaAction::Stop:
                return MAKELPARAM(0, APPCOMMAND_MEDIA_STOP);
            case MediaAction::VolumeUp:
                return MAKELPARAM(0, APPCOMMAND_VOLUME_UP);
            case MediaAction::VolumeDown:
                return MAKELPARAM(0, APPCOMMAND_VOLUME_DOWN);
            case MediaAction::Mute:
                return MAKELPARAM(0, APPCOMMAND_VOLUME_MUTE);
            }

            return 0;
        }
    }

    bool Actions::send_action(HWND hwnd, MediaAction action) const
    {
        if (!IsWindow(hwnd))
        {
            return false;
        }

        const LPARAM app_command = action_to_app_command(action);

        if (app_command == 0)
        {
            return false;
        }

        return PostMessageW(hwnd, WM_APPCOMMAND, reinterpret_cast<WPARAM>(hwnd), app_command) != 0;
    }
}
