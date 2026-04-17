#ifndef OMNI_MEDIA_LINKER_CONFIG_LOADER_H
#define OMNI_MEDIA_LINKER_CONFIG_LOADER_H

#include <string>
#include <vector>

#include "common.h"

namespace oml
{
    class ConfigLoader
    {
    public:
        explicit ConfigLoader(std::wstring path);

        std::vector<Binding> load() const;

        bool save(const std::vector<Binding> &bindings) const;

        const std::wstring &path() const;

    private:
        std::wstring path_;
    };
}

#endif
