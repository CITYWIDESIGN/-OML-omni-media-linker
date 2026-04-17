#ifndef OMNI_MEDIA_LINKER_ACTIONS_H
#define OMNI_MEDIA_LINKER_ACTIONS_H

#include "common.h"

namespace oml
{
    class Actions
    {
    public:
        bool send_action(HWND hwnd, MediaAction action) const;
    };
}

#endif
