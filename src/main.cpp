#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "common.h"
#include "core/dispatcher.h"
#include "core/mapper.h"
#include "os/scanner.h"
#include "util/config_loader.h"

namespace
{
    using namespace oml;

    Dispatcher *g_dispatcher = nullptr;
    HANDLE g_single_instance_mutex = nullptr;

    enum class MenuAction
    {
        CreateBindings = 1,
        ManageBindings = 2,
        DeleteBindings = 3,
        ExitProgram = 4
    };

    enum class ManageAction
    {
        EnableBinding = 1,
        DisableBinding = 2,
        EditBinding = 3,
        StartListening = 4,
        BackToMenu = 5
    };

    void handle_console_signal(int)
    {
        if (g_dispatcher != nullptr)
        {
            g_dispatcher->unregister_all();
        }

        if (g_single_instance_mutex != nullptr)
        {
            CloseHandle(g_single_instance_mutex);
            g_single_instance_mutex = nullptr;
        }

        std::_Exit(0);
    }

    bool acquire_single_instance_guard()
    {
        g_single_instance_mutex = CreateMutexW(nullptr, FALSE, L"OmniMediaLinker.SingleInstance");

        if (g_single_instance_mutex == nullptr)
        {
            return false;
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(g_single_instance_mutex);
            g_single_instance_mutex = nullptr;
            return false;
        }

        return true;
    }

    void release_single_instance_guard()
    {
        if (g_single_instance_mutex != nullptr)
        {
            CloseHandle(g_single_instance_mutex);
            g_single_instance_mutex = nullptr;
        }
    }

    // 清空控制台，让每次回到菜单时界面更干净。
    void clear_console()
    {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (console == INVALID_HANDLE_VALUE)
        {
            return;
        }

        CONSOLE_SCREEN_BUFFER_INFO buffer_info{};

        if (!GetConsoleScreenBufferInfo(console, &buffer_info))
        {
            return;
        }

        DWORD written = 0;
        const DWORD size = static_cast<DWORD>(buffer_info.dwSize.X) * static_cast<DWORD>(buffer_info.dwSize.Y);
        const COORD home{0, 0};

        FillConsoleOutputCharacterA(console, ' ', size, home, &written);
        FillConsoleOutputAttribute(console, buffer_info.wAttributes, size, home, &written);
        SetConsoleCursorPosition(console, home);
    }

    bool read_line(std::string &value)
    {
        return static_cast<bool>(std::getline(std::cin, value));
    }

    bool parse_int(const std::string &text, int &value)
    {
        std::stringstream stream(text);

        stream >> value;

        if (!stream)
        {
            return false;
        }

        char remain = '\0';

        if (stream >> remain)
        {
            return false;
        }

        return true;
    }

    bool read_int(const std::string &prompt, int &value)
    {
        std::string line;

        std::cout << prompt;

        if (!read_line(line))
        {
            return false;
        }

        return parse_int(line, value);
    }

    // 过长文本会截断，避免把后面的列顶歪。
    std::string truncate_text(const std::string &value, std::size_t width)
    {
        if (value.size() <= width)
        {
            return value;
        }

        if (width <= 3)
        {
            return value.substr(0, width);
        }

        return value.substr(0, width - 3) + "...";
    }

    // 使用定宽列来保证控制台列表稳定对齐。
    std::string pad_cell(const std::string &value, std::size_t width)
    {
        std::ostringstream stream;

        stream << std::left << std::setw(static_cast<int>(width)) << truncate_text(value, width);

        return stream.str();
    }

    // 中文字符在控制台里通常占两列，这里按显示宽度来计算对齐。
    std::size_t display_width(const std::wstring &value)
    {
        std::size_t width = 0;

        for (wchar_t ch : value)
        {
            if (ch == L'\0')
            {
                continue;
            }

            width += ch <= 0x7f ? 1 : 2;
        }

        return width;
    }

    // 按显示宽度截断，避免中文标题把后面的列挤乱。
    std::wstring truncate_wide_text(const std::wstring &value, std::size_t width)
    {
        if (display_width(value) <= width)
        {
            return value;
        }

        if (width <= 3)
        {
            return value.substr(0, width);
        }

        std::wstring result;
        std::size_t current_width = 0;
        const std::size_t limit = width - 3;

        for (wchar_t ch : value)
        {
            const std::size_t ch_width = ch <= 0x7f ? 1 : 2;

            if (current_width + ch_width > limit)
            {
                break;
            }

            result.push_back(ch);
            current_width += ch_width;
        }

        result += L"...";

        return result;
    }

    // 根据显示宽度补空格，保证中英文混排时列仍然整齐。
    std::string pad_wide_cell(const std::wstring &value, std::size_t width)
    {
        const std::wstring truncated = truncate_wide_text(value, width);
        const std::size_t current_width = display_width(truncated);
        std::string result = to_utf8(truncated);

        if (current_width < width)
        {
            result += std::string(width - current_width, ' ');
        }

        return result;
    }

    std::wstring int_to_wstring(int value)
    {
        std::wstringstream stream;

        stream << value;

        return stream.str();
    }

    void print_banner()
    {
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "        Omni Media Linker Console\n";
        std::cout << "========================================\n";
        std::cout << "给指定播放器绑定专属热键\n";
    }

    void print_main_menu()
    {
        std::cout << "\n主菜单\n";
        std::cout << "----------------------------------------\n";
        std::cout << "  1) 创建绑定\n";
        std::cout << "  2) 管理绑定并启动监听\n";
        std::cout << "  3) 删除绑定\n";
        std::cout << "  4) 退出程序\n";
        std::cout << "----------------------------------------\n";
    }

    bool ask_menu_action(MenuAction &action)
    {
        int selected = 0;

        print_main_menu();

        if (!read_int("请输入功能编号: ", selected))
        {
            return false;
        }

        switch (selected)
        {
        case 1:
            action = MenuAction::CreateBindings;
            return true;
        case 2:
            action = MenuAction::ManageBindings;
            return true;
        case 3:
            action = MenuAction::DeleteBindings;
            return true;
        case 4:
            action = MenuAction::ExitProgram;
            return true;
        default:
            return false;
        }
    }

    void print_windows(const std::vector<WindowInfo> &windows)
    {
        const std::size_t index_width = 6;
        const std::size_t title_width = 30;
        const std::size_t class_width = 18;
        const std::size_t process_width = 18;
        const std::size_t hwnd_width = 14;

        std::cout << "\n窗口列表\n";
        std::cout << pad_wide_cell(L"编号", index_width)
                  << pad_wide_cell(L"窗口标题", title_width)
                  << pad_wide_cell(L"类名", class_width)
                  << pad_wide_cell(L"进程名", process_width)
                  << pad_wide_cell(L"HWND", hwnd_width)
                  << '\n';
        std::cout << "----------------------------------------------------------------------------------------\n";

        for (std::size_t index = 0; index < windows.size(); ++index)
        {
            const auto &window = windows[index];

            std::cout << pad_wide_cell(int_to_wstring(static_cast<int>(index + 1)), index_width)
                      << pad_wide_cell(window.title, title_width)
                      << pad_wide_cell(window.className, class_width)
                      << pad_wide_cell(window.processName, process_width)
                      << pad_wide_cell(hwnd_to_string(window.hwnd), hwnd_width)
                      << '\n';
        }
    }

    void print_actions()
    {
        std::cout << "\n可选动作:\n";
        std::cout << "[1] 播放 / 暂停\n";
        std::cout << "[2] 下一曲\n";
        std::cout << "[3] 上一曲\n";
        std::cout << "[4] 停止\n";
        std::cout << "[5] 音量加\n";
        std::cout << "[6] 音量减\n";
        std::cout << "[7] 静音\n";
    }

    bool ask_action(MediaAction &action)
    {
        print_actions();

        int selected = 0;

        if (!read_int("\n输入动作编号: ", selected))
        {
            return false;
        }

        switch (selected)
        {
        case 1:
            action = MediaAction::PlayPause;
            return true;
        case 2:
            action = MediaAction::NextTrack;
            return true;
        case 3:
            action = MediaAction::PreviousTrack;
            return true;
        case 4:
            action = MediaAction::Stop;
            return true;
        case 5:
            action = MediaAction::VolumeUp;
            return true;
        case 6:
            action = MediaAction::VolumeDown;
            return true;
        case 7:
            action = MediaAction::Mute;
            return true;
        default:
            return false;
        }
    }

    void print_match_modes()
    {
        std::cout << "\n匹配模式\n";
        std::cout << "[1] 标题精确匹配\n";
        std::cout << "[2] 标题包含匹配\n";
        std::cout << "[3] 进程名匹配\n";
        std::cout << "[4] 进程名 + 类名匹配\n";
    }

    bool ask_match_mode(MatchMode &match_mode)
    {
        int selected = 0;

        print_match_modes();

        if (!read_int("\n输入匹配模式编号: ", selected))
        {
            return false;
        }

        return match_mode_from_int(selected, match_mode);
    }

    bool is_modifier_key(int vk)
    {
        return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
               vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
               vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
               vk == VK_LWIN || vk == VK_RWIN;
    }

    UINT capture_modifiers()
    {
        UINT modifiers = 0;

        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
        {
            modifiers |= MOD_CONTROL;
        }

        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
        {
            modifiers |= MOD_ALT;
        }

        if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
        {
            modifiers |= MOD_SHIFT;
        }

        if ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0)
        {
            modifiers |= MOD_WIN;
        }

        return modifiers;
    }

    bool capture_hotkey(Hotkey &hotkey)
    {
        std::cout << "\n请切到想绑定的组合键并按下，Esc 取消。\n";
        std::cout << "建议使用 Alt / Ctrl / Shift 搭配字母或数字键。\n";

        // 先等待上一步输入时按下的按键全部松开，避免把 Enter 误录进去。
        while (true)
        {
            bool any_pressed = false;

            for (int vk = 1; vk <= 254; ++vk)
            {
                if ((GetAsyncKeyState(vk) & 0x8000) != 0)
                {
                    any_pressed = true;
                    break;
                }
            }

            if (!any_pressed)
            {
                break;
            }

            Sleep(20);
        }

        while (true)
        {
            if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            {
                return false;
            }

            const UINT modifiers = capture_modifiers();

            for (int vk = 1; vk <= 254; ++vk)
            {
                if (is_modifier_key(vk))
                {
                    continue;
                }

                if ((GetAsyncKeyState(vk) & 0x8000) != 0)
                {
                    hotkey.modifiers = modifiers;
                    hotkey.virtualKey = static_cast<UINT>(vk);

                    while ((GetAsyncKeyState(vk) & 0x8000) != 0)
                    {
                        Sleep(20);
                    }

                    return true;
                }
            }

            Sleep(20);
        }
    }

    void print_bindings(const Mapper &mapper)
    {
        const std::size_t id_width = 6;
        const std::size_t hotkey_width = 18;
        const std::size_t title_width = 24;
        const std::size_t action_width = 18;
        const std::size_t match_width = 12;
        const std::size_t status_width = 10;

        std::cout << "\n绑定列表\n";
        std::cout << pad_wide_cell(L"ID", id_width)
                  << pad_wide_cell(L"热键", hotkey_width)
                  << pad_wide_cell(L"目标窗口", title_width)
                  << pad_wide_cell(L"动作", action_width)
                  << pad_wide_cell(L"匹配", match_width)
                  << pad_wide_cell(L"状态", status_width)
                  << '\n';
        std::cout << "---------------------------------------------------------------------------------------------\n";

        for (const auto &binding : mapper.all())
        {
            std::cout << pad_wide_cell(int_to_wstring(binding.id), id_width)
                      << pad_wide_cell(hotkey_to_string(binding.hotkey), hotkey_width)
                      << pad_wide_cell(binding.windowTitle, title_width)
                      << pad_wide_cell(action_to_string(binding.action), action_width)
                      << pad_wide_cell(match_mode_to_string(binding.matchMode), match_width)
                      << pad_wide_cell(binding.enabled ? L"启用" : L"禁用", status_width)
                      << '\n';
        }
    }

    bool ask_yes_no(const std::string &prompt)
    {
        std::string line;

        std::cout << prompt << " (y/n): ";

        if (!read_line(line) || line.empty())
        {
            return false;
        }

        return line[0] == 'y' || line[0] == 'Y';
    }

    bool save_bindings(const ConfigLoader &config_loader, const Mapper &mapper)
    {
        if (!config_loader.save(mapper.all()))
        {
            std::cout << "配置保存失败。\n";
            return false;
        }

        return true;
    }

    std::wstring executable_directory()
    {
        wchar_t path_buffer[MAX_PATH]{};

        GetModuleFileNameW(nullptr, path_buffer, MAX_PATH);

        std::wstring path = path_buffer;
        const std::size_t pos = path.find_last_of(L"\\/");

        if (pos == std::wstring::npos)
        {
            return L".";
        }

        return path.substr(0, pos);
    }

    std::wstring join_path(const std::wstring &left, const std::wstring &right)
    {
        if (left.empty())
        {
            return right;
        }

        if (left.back() == L'\\' || left.back() == L'/')
        {
            return left + right;
        }

        return left + L"\\" + right;
    }

    void load_saved_bindings(const ConfigLoader &config_loader, Mapper &mapper)
    {
        const auto saved_bindings = config_loader.load();

        mapper.replace_all(saved_bindings);
    }

    void resolve_binding_targets(Scanner &scanner, Mapper &mapper)
    {
        if (mapper.all().empty())
        {
            return;
        }

        const auto windows = scanner.scan_active_windows();
        std::vector<Binding> resolved;

        for (const auto &binding : mapper.all())
        {
            Binding updated = binding;
            bool matched = false;

            for (const auto &window : windows)
            {
                const bool title_exact = window.title == binding.windowTitle;
                const bool title_contains = !binding.windowTitle.empty() && window.title.find(binding.windowTitle) != std::wstring::npos;
                const bool process_exact = !binding.processName.empty() && window.processName == binding.processName;
                const bool class_exact = window.className == binding.windowClass;

                if ((binding.matchMode == MatchMode::TitleExact && title_exact && class_exact) ||
                    (binding.matchMode == MatchMode::TitleContains && title_contains) ||
                    (binding.matchMode == MatchMode::ProcessName && process_exact) ||
                    (binding.matchMode == MatchMode::ProcessNameAndClass && process_exact && class_exact))
                {
                    updated.hwnd = window.hwnd;
                    updated.windowTitle = window.title;
                    updated.windowClass = window.className;
                    updated.processName = window.processName;
                    matched = true;
                    break;
                }
            }

            if (matched)
            {
                resolved.push_back(updated);
            }
        }

        mapper.replace_all(resolved);
    }

    void wait_enter()
    {
        std::string line;

        std::cout << "\n按回车返回主菜单...";
        read_line(line);
    }

    void wait_enter(const std::string &prompt)
    {
        std::string line;

        std::cout << prompt;
        read_line(line);
    }

    bool set_binding_enabled(Mapper &mapper, int target_id, bool enabled)
    {
        std::vector<Binding> updated = mapper.all();
        bool found = false;

        for (auto &binding : updated)
        {
            if (binding.id == target_id)
            {
                binding.enabled = enabled;
                found = true;
                break;
            }
        }

        if (!found)
        {
            return false;
        }

        mapper.replace_all(updated);

        return true;
    }

    bool update_binding(Mapper &mapper, const Binding &updated_binding)
    {
        std::vector<Binding> updated = mapper.all();

        for (auto &binding : updated)
        {
            if (binding.id == updated_binding.id)
            {
                binding = updated_binding;
                mapper.replace_all(updated);
                return true;
            }
        }

        return false;
    }

    void normalize_binding_ids(Mapper &mapper)
    {
        std::vector<Binding> updated = mapper.all();

        for (std::size_t index = 0; index < updated.size(); ++index)
        {
            updated[index].id = static_cast<int>(index + 1);
        }

        mapper.replace_all(updated);
    }

    void print_edit_menu(const Binding &binding);

    bool edit_binding_module(Scanner &scanner, Mapper &mapper, const ConfigLoader &config_loader)
    {
        int target_id = 0;

        if (!read_int("输入要编辑的绑定 ID: ", target_id))
        {
            std::cout << "输入无效。\n";
            wait_enter("\n按回车继续...");
            return false;
        }

        const Binding *binding_ptr = mapper.find_by_id(target_id);

        if (binding_ptr == nullptr)
        {
            std::cout << "没有找到对应的绑定 ID。\n";
            wait_enter("\n按回车继续...");
            return false;
        }

        Binding updated = *binding_ptr;

        while (true)
        {
            clear_console();
            print_banner();
            std::cout << "当前编辑目标: " << to_utf8(updated.windowTitle) << "\n";
            std::cout << "当前进程: " << to_utf8(updated.processName) << "\n";
            std::cout << "当前热键: " << to_utf8(hotkey_to_string(updated.hotkey)) << "\n";
            std::cout << "当前动作: " << to_utf8(action_to_string(updated.action)) << "\n";
            std::cout << "匹配方式: " << to_utf8(match_mode_to_string(updated.matchMode)) << "\n";

            int selected = 0;

            print_edit_menu(updated);

            if (!read_int("请输入编辑项编号: ", selected))
            {
                std::cout << "输入无效。\n";
                wait_enter("\n按回车继续...");
                continue;
            }

            if (selected == 1)
            {
                MediaAction action{};

                if (!ask_action(action))
                {
                    std::cout << "动作编号无效。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                updated.action = action;
            }
            else if (selected == 2)
            {
                Hotkey hotkey{};

                if (!capture_hotkey(hotkey))
                {
                    std::cout << "已取消热键录入。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                const Binding *exists = mapper.find_by_hotkey(hotkey.modifiers, hotkey.virtualKey);

                if (exists != nullptr && exists->id != updated.id)
                {
                    std::cout << "这个热键已经被其他绑定占用。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                updated.hotkey = hotkey;
            }
            else if (selected == 3)
            {
                const auto windows = scanner.scan_active_windows();

                if (windows.empty())
                {
                    std::cout << "没有扫描到可绑定的活动窗口。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                clear_console();
                print_banner();
                print_windows(windows);

                int window_index = 0;

                if (!read_int("\n输入新的窗口编号，输入 0 返回: ", window_index))
                {
                    std::cout << "输入无效。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                if (window_index == 0)
                {
                    continue;
                }

                if (window_index < 1 || window_index > static_cast<int>(windows.size()))
                {
                    std::cout << "窗口编号无效。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                const auto &window = windows[window_index - 1];

                updated.hwnd = window.hwnd;
                updated.windowTitle = window.title;
                updated.windowClass = window.className;
                updated.processName = window.processName;
            }
            else if (selected == 4)
            {
                MatchMode match_mode{};

                if (!ask_match_mode(match_mode))
                {
                    std::cout << "匹配模式编号无效。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                updated.matchMode = match_mode;
            }
            else if (selected == 5)
            {
                return false;
            }
            else
            {
                std::cout << "编辑项编号无效。\n";
                wait_enter("\n按回车继续...");
                continue;
            }

            update_binding(mapper, updated);
            save_bindings(config_loader, mapper);
            std::cout << "绑定已更新。\n";
            wait_enter("\n按回车继续...");
            return true;
        }
    }

    bool has_enabled_bindings(const Mapper &mapper)
    {
        for (const auto &binding : mapper.all())
        {
            if (binding.enabled)
            {
                return true;
            }
        }

        return false;
    }

    void print_manage_menu()
    {
        std::cout << "\n管理操作\n";
        std::cout << "----------------------------------------\n";
        std::cout << "  1) 启用绑定\n";
        std::cout << "  2) 禁用绑定\n";
        std::cout << "  3) 编辑绑定\n";
        std::cout << "  4) 启动监听\n";
        std::cout << "  5) 返回主菜单\n";
        std::cout << "----------------------------------------\n";
    }

    bool ask_manage_action(ManageAction &action)
    {
        int selected = 0;

        print_manage_menu();

        if (!read_int("请输入操作编号: ", selected))
        {
            return false;
        }

        switch (selected)
        {
        case 1:
            action = ManageAction::EnableBinding;
            return true;
        case 2:
            action = ManageAction::DisableBinding;
            return true;
        case 3:
            action = ManageAction::EditBinding;
            return true;
        case 4:
            action = ManageAction::StartListening;
            return true;
        case 5:
            action = ManageAction::BackToMenu;
            return true;
        default:
            return false;
        }
    }

    void print_edit_menu(const Binding &binding)
    {
        std::cout << "\n编辑绑定 #" << binding.id << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "  1) 修改动作\n";
        std::cout << "  2) 修改热键\n";
        std::cout << "  3) 修改目标窗口\n";
        std::cout << "  4) 设置匹配模式\n";
        std::cout << "  5) 返回上一级\n";
        std::cout << "----------------------------------------\n";
    }

    void create_bindings_module(Scanner &scanner, Mapper &mapper, const ConfigLoader &config_loader)
    {
        clear_console();
        print_banner();

        while (true)
        {
            const auto windows = scanner.scan_active_windows();

            if (windows.empty())
            {
                std::cout << "没有扫描到可绑定的活动窗口。\n";
                return;
            }

            print_windows(windows);

            int window_index = 0;

            std::cout << "\n创建新绑定\n";

            if (!read_int("输入窗口编号，输入 0 返回主菜单: ", window_index))
            {
                std::cout << "窗口编号无效，已返回主菜单。\n";
                return;
            }

            if (window_index == 0)
            {
                return;
            }

            if (window_index < 1 || window_index > static_cast<int>(windows.size()))
            {
                std::cout << "窗口编号无效，已返回主菜单。\n";
                return;
            }

            clear_console();
            print_banner();
            std::cout << "\n已选择窗口: " << to_utf8(windows[window_index - 1].title) << "\n";

            MediaAction action{};

            if (!ask_action(action))
            {
                std::cout << "动作编号无效，已返回主菜单。\n";
                return;
            }

            MatchMode match_mode{};

            if (!ask_match_mode(match_mode))
            {
                std::cout << "匹配模式编号无效，已返回主菜单。\n";
                return;
            }

            Hotkey hotkey{};

            if (!capture_hotkey(hotkey))
            {
                std::cout << "已取消热键录入，已返回主菜单。\n";
                return;
            }

            if (mapper.hotkey_exists(hotkey.modifiers, hotkey.virtualKey))
            {
                std::cout << "这个热键已经被占用: " << to_utf8(hotkey_to_string(hotkey)) << '\n';
                continue;
            }

            const auto &window = windows[window_index - 1];

            mapper.add_binding(window.hwnd, window.title, window.className, window.processName, match_mode, action, hotkey);

            std::cout << "已绑定: "
                      << to_utf8(hotkey_to_string(hotkey))
                      << " -> "
                      << to_utf8(window.title)
                      << " / "
                      << to_utf8(action_to_string(action))
                      << '\n';

            if (!save_bindings(config_loader, mapper))
            {
                return;
            }

            print_bindings(mapper);
            wait_enter();
            return;
        }
    }

    void delete_bindings_module(Mapper &mapper, const ConfigLoader &config_loader)
    {
        clear_console();
        print_banner();

        if (mapper.all().empty())
        {
            std::cout << "当前没有任何绑定可以删除。\n";
            wait_enter();
            return;
        }

        print_bindings(mapper);

        int target_id = 0;

        if (!read_int("输入要删除的绑定 ID: ", target_id))
        {
            std::cout << "输入无效，已返回主菜单。\n";
            wait_enter();
            return;
        }

        std::vector<Binding> remaining;
        bool removed = false;

        for (const auto &binding : mapper.all())
        {
            if (binding.id == target_id)
            {
                removed = true;
                continue;
            }

            remaining.push_back(binding);
        }

        if (!removed)
        {
            std::cout << "没有找到对应的绑定 ID。\n";
            wait_enter();
            return;
        }

        mapper.replace_all(remaining);
        normalize_binding_ids(mapper);
        save_bindings(config_loader, mapper);

        std::cout << "绑定已删除。\n";

        if (!mapper.all().empty())
        {
            print_bindings(mapper);
        }

        wait_enter();
    }

    void manage_bindings_module(Scanner &scanner, Mapper &mapper, Actions &actions, const ConfigLoader &config_loader)
    {
        clear_console();
        print_banner();

        if (mapper.all().empty())
        {
            std::cout << "当前没有绑定，请先创建绑定。\n";
            wait_enter();
            return;
        }

        resolve_binding_targets(scanner, mapper);
        save_bindings(config_loader, mapper);

        while (true)
        {
            clear_console();
            print_banner();
            std::cout << "绑定管理\n";
            print_bindings(mapper);

            ManageAction action{};

            if (!ask_manage_action(action))
            {
                std::cout << "操作编号无效。\n";
                wait_enter("\n按回车继续...");
                continue;
            }

            if (action == ManageAction::EnableBinding || action == ManageAction::DisableBinding)
            {
                int target_id = 0;

                if (!read_int("输入要操作的绑定 ID: ", target_id))
                {
                    std::cout << "输入无效。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                if (!set_binding_enabled(mapper, target_id, action == ManageAction::EnableBinding))
                {
                    std::cout << "没有找到对应的绑定 ID。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                save_bindings(config_loader, mapper);
                std::cout << (action == ManageAction::EnableBinding ? "绑定已启用。\n" : "绑定已禁用。\n");
                wait_enter("\n按回车继续...");
                continue;
            }

            if (action == ManageAction::EditBinding)
            {
                edit_binding_module(scanner, mapper, config_loader);
                continue;
            }

            if (action == ManageAction::StartListening)
            {
                if (!has_enabled_bindings(mapper))
                {
                    std::cout << "当前没有启用中的绑定，无法启动监听。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                Dispatcher dispatcher(mapper, actions);

                g_dispatcher = &dispatcher;

                std::signal(SIGINT, handle_console_signal);

                if (!dispatcher.register_all())
                {
                    dispatcher.unregister_all();
                    std::cout << "热键注册失败，无法启动监听。\n";
                    wait_enter("\n按回车继续...");
                    continue;
                }

                dispatcher.run_message_loop();
                return;
            }

            if (action == ManageAction::BackToMenu)
            {
                return;
            }
        }
    }
}

int main()
{
    using namespace oml;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (!acquire_single_instance_guard())
    {
        std::cout << "omni-media-linker 已经在运行，请不要重复启动。\n";
        return 1;
    }

    Scanner scanner;
    Mapper mapper;
    Actions actions;
    ConfigLoader config_loader(join_path(executable_directory(), L"config.ini"));

    load_saved_bindings(config_loader, mapper);

    while (true)
    {
        clear_console();
        print_banner();
        std::cout << "配置文件: " << to_utf8(config_loader.path()) << "\n";

        if (!mapper.all().empty())
        {
            std::cout << "已加载历史绑定。\n";
            print_bindings(mapper);
        }

        MenuAction action{};

        if (!ask_menu_action(action))
        {
            std::cout << "菜单编号无效，请重新输入。\n\n";
            continue;
        }

        if (action == MenuAction::CreateBindings)
        {
            create_bindings_module(scanner, mapper, config_loader);
            std::cout << "\n";
            continue;
        }

        if (action == MenuAction::ManageBindings)
        {
            manage_bindings_module(scanner, mapper, actions, config_loader);
            std::cout << "\n";
            continue;
        }

        if (action == MenuAction::DeleteBindings)
        {
            delete_bindings_module(mapper, config_loader);
            std::cout << "\n";
            continue;
        }

        if (action == MenuAction::ExitProgram)
        {
            std::cout << "程序已退出。\n";
            release_single_instance_guard();
            return 0;
        }
    }

    release_single_instance_guard();
    return 0;
}
