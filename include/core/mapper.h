#ifndef OMNI_MEDIA_LINKER_MAPPER_H
#define OMNI_MEDIA_LINKER_MAPPER_H

#include <vector>

#include "common.h"

namespace oml
{
    class Mapper
    {
    public:
        int add_binding(HWND hwnd, const std::wstring &window_title, const std::wstring &window_class, const std::wstring &process_name, MatchMode match_mode, MediaAction action, Hotkey hotkey);

        void replace_all(std::vector<Binding> bindings);

        const std::vector<Binding> &all() const;

        const Binding *find_by_id(int id) const;

        const Binding *find_by_hotkey(UINT modifiers, UINT virtual_key) const;

        bool hotkey_exists(UINT modifiers, UINT virtual_key) const;

        void clear();

    private:
        std::vector<Binding> bindings_;
        int next_id_{1};
    };
}

#endif
