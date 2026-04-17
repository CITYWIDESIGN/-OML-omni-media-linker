#include "core/mapper.h"

#include <algorithm>

namespace oml
{
    int Mapper::add_binding(HWND hwnd, const std::wstring &window_title, const std::wstring &window_class, const std::wstring &process_name, MatchMode match_mode, MediaAction action, Hotkey hotkey)
    {
        Binding binding;

        binding.id = next_id_++;
        binding.hwnd = hwnd;
        binding.windowTitle = window_title;
        binding.windowClass = window_class;
        binding.processName = process_name;
        binding.matchMode = match_mode;
        binding.action = action;
        binding.hotkey = hotkey;

        bindings_.push_back(binding);

        return binding.id;
    }

    void Mapper::replace_all(std::vector<Binding> bindings)
    {
        bindings_ = std::move(bindings);
        next_id_ = 1;

        for (const auto &binding : bindings_)
        {
            next_id_ = std::max(next_id_, binding.id + 1);
        }
    }

    const std::vector<Binding> &Mapper::all() const
    {
        return bindings_;
    }

    const Binding *Mapper::find_by_id(int id) const
    {
        const auto it = std::find_if(bindings_.begin(), bindings_.end(), [id](const Binding &binding)
        {
            return binding.id == id;
        });

        if (it == bindings_.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    const Binding *Mapper::find_by_hotkey(UINT modifiers, UINT virtual_key) const
    {
        const auto it = std::find_if(bindings_.begin(), bindings_.end(), [modifiers, virtual_key](const Binding &binding)
        {
            return binding.hotkey.modifiers == modifiers && binding.hotkey.virtualKey == virtual_key;
        });

        if (it == bindings_.end())
        {
            return nullptr;
        }

        return &(*it);
    }

    bool Mapper::hotkey_exists(UINT modifiers, UINT virtual_key) const
    {
        return find_by_hotkey(modifiers, virtual_key) != nullptr;
    }

    void Mapper::clear()
    {
        bindings_.clear();
        next_id_ = 1;
    }
}
