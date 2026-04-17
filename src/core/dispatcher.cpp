#include "core/dispatcher.h"

#include <iostream>

namespace oml
{
    Dispatcher::Dispatcher(const Mapper &mapper, const Actions &actions)
        : mapper_(mapper), actions_(actions)
    {
    }

    bool Dispatcher::register_all() const
    {
        bool success = true;

        for (const auto &binding : mapper_.all())
        {
            if (!binding.enabled)
            {
                continue;
            }

            if (!RegisterHotKey(nullptr, binding.id, binding.hotkey.modifiers, binding.hotkey.virtualKey))
            {
                std::cout << "[!] 无法注册热键: "
                          << to_utf8(hotkey_to_string(binding.hotkey))
                          << " -> "
                          << to_utf8(binding.windowTitle)
                          << '\n';
                success = false;
            }
        }

        return success;
    }

    void Dispatcher::unregister_all() const
    {
        for (const auto &binding : mapper_.all())
        {
            if (!binding.enabled)
            {
                continue;
            }

            UnregisterHotKey(nullptr, binding.id);
        }
    }

    void Dispatcher::run_message_loop() const
    {
        MSG message{};

        std::cout << "\n已进入后台监听，按 Ctrl+C 结束程序。\n";

        while (GetMessageW(&message, nullptr, 0, 0) > 0)
        {
            if (message.message == WM_HOTKEY)
            {
                const int id = static_cast<int>(message.wParam);
                const Binding *binding = mapper_.find_by_id(id);

                if (binding != nullptr)
                {
                    const bool delivered = actions_.send_action(binding->hwnd, binding->action);

                    std::cout << "["
                              << (delivered ? "OK" : "FAIL")
                              << "] "
                              << to_utf8(hotkey_to_string(binding->hotkey))
                              << " -> "
                              << to_utf8(binding->windowTitle)
                              << " / "
                              << to_utf8(action_to_string(binding->action))
                              << '\n';
                }
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
