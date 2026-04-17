#ifndef OMNI_MEDIA_LINKER_SCANNER_H
#define OMNI_MEDIA_LINKER_SCANNER_H

#include <vector>

#include "common.h"

namespace oml
{
    class Scanner
    {
    public:
        std::vector<WindowInfo> scan_active_windows() const;
    };
}

#endif
