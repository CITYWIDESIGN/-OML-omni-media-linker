#include "util/config_loader.h"

#include <array>
#include <sstream>

namespace oml
{
    namespace
    {
        std::wstring to_wstring_compat(int value)
        {
            std::wstringstream stream;

            stream << value;

            return stream.str();
        }

        std::wstring to_wstring_compat(UINT value)
        {
            std::wstringstream stream;

            stream << value;

            return stream.str();
        }
    }

    ConfigLoader::ConfigLoader(std::wstring path)
        : path_(std::move(path))
    {
    }

    std::vector<Binding> ConfigLoader::load() const
    {
        std::vector<Binding> bindings;
        wchar_t section_names[8192]{};

        GetPrivateProfileSectionNamesW(section_names, static_cast<DWORD>(std::size(section_names)), path_.c_str());

        const wchar_t *current = section_names;
        int next_id = 1;

        while (*current != L'\0')
        {
            const std::wstring section = current;

            if (section.rfind(L"binding_", 0) == 0)
            {
                wchar_t hwnd_buffer[64]{};
                wchar_t title_buffer[512]{};
                wchar_t class_buffer[256]{};
                wchar_t process_name_buffer[260]{};
                wchar_t action_buffer[128]{};
                wchar_t modifiers_buffer[64]{};
                wchar_t virtual_key_buffer[64]{};
                wchar_t id_buffer[64]{};
                wchar_t enabled_buffer[16]{};
                wchar_t match_mode_buffer[16]{};

                GetPrivateProfileStringW(section.c_str(), L"hwnd", L"", hwnd_buffer, static_cast<DWORD>(std::size(hwnd_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"title", L"", title_buffer, static_cast<DWORD>(std::size(title_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"class", L"", class_buffer, static_cast<DWORD>(std::size(class_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"process_name", L"", process_name_buffer, static_cast<DWORD>(std::size(process_name_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"action", L"", action_buffer, static_cast<DWORD>(std::size(action_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"modifiers", L"0", modifiers_buffer, static_cast<DWORD>(std::size(modifiers_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"virtual_key", L"0", virtual_key_buffer, static_cast<DWORD>(std::size(virtual_key_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"id", L"0", id_buffer, static_cast<DWORD>(std::size(id_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"enabled", L"1", enabled_buffer, static_cast<DWORD>(std::size(enabled_buffer)), path_.c_str());
                GetPrivateProfileStringW(section.c_str(), L"match_mode", L"1", match_mode_buffer, static_cast<DWORD>(std::size(match_mode_buffer)), path_.c_str());

                HWND hwnd = nullptr;
                MediaAction action{};
                MatchMode match_mode{};

                if (!hwnd_from_string(hwnd_buffer, hwnd))
                {
                    current += section.size() + 1;
                    continue;
                }

                if (!action_from_string(action_buffer, action))
                {
                    current += section.size() + 1;
                    continue;
                }

                if (!match_mode_from_int(_wtoi(match_mode_buffer), match_mode))
                {
                    current += section.size() + 1;
                    continue;
                }

                Binding binding;

                binding.id = std::max(1, _wtoi(id_buffer));
                binding.hwnd = hwnd;
                binding.windowTitle = title_buffer;
                binding.windowClass = class_buffer;
                binding.processName = process_name_buffer;
                binding.matchMode = match_mode;
                binding.action = action;
                binding.hotkey.modifiers = static_cast<UINT>(_wtoi(modifiers_buffer));
                binding.hotkey.virtualKey = static_cast<UINT>(_wtoi(virtual_key_buffer));
                binding.enabled = _wtoi(enabled_buffer) != 0;

                if (binding.hotkey.virtualKey != 0)
                {
                    bindings.push_back(binding);
                    next_id = std::max(next_id, binding.id + 1);
                }
            }

            current += section.size() + 1;
        }

        return bindings;
    }

    bool ConfigLoader::save(const std::vector<Binding> &bindings) const
    {
        wchar_t section_names[8192]{};

        GetPrivateProfileSectionNamesW(section_names, static_cast<DWORD>(std::size(section_names)), path_.c_str());

        const wchar_t *current = section_names;

        while (*current != L'\0')
        {
            const std::wstring section = current;

            if (section.rfind(L"binding_", 0) == 0)
            {
                if (!WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, path_.c_str()))
                {
                    return false;
                }
            }

            current += section.size() + 1;
        }

        for (const auto &binding : bindings)
        {
            const std::wstring section = L"binding_" + to_wstring_compat(binding.id);

            if (!WritePrivateProfileStringW(section.c_str(), L"id", to_wstring_compat(binding.id).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"hwnd", hwnd_to_string(binding.hwnd).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"title", binding.windowTitle.c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"class", binding.windowClass.c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"process_name", binding.processName.c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"match_mode", to_wstring_compat(static_cast<int>(binding.matchMode)).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"action", action_to_string(binding.action).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"modifiers", to_wstring_compat(binding.hotkey.modifiers).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"virtual_key", to_wstring_compat(binding.hotkey.virtualKey).c_str(), path_.c_str()))
            {
                return false;
            }

            if (!WritePrivateProfileStringW(section.c_str(), L"enabled", binding.enabled ? L"1" : L"0", path_.c_str()))
            {
                return false;
            }
        }

        return true;
    }

    const std::wstring &ConfigLoader::path() const
    {
        return path_;
    }
}
