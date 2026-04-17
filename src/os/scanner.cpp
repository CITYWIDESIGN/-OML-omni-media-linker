#include "os/scanner.h"

#include <algorithm>

namespace oml
{
    namespace
    {
        std::wstring filename_from_path(const std::wstring &path)
        {
            const std::size_t pos = path.find_last_of(L"\\/");

            if (pos == std::wstring::npos)
            {
                return path;
            }

            return path.substr(pos + 1);
        }

        std::wstring get_process_name(DWORD process_id)
        {
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);

            if (process == nullptr)
            {
                return L"";
            }

            wchar_t buffer[MAX_PATH]{};
            DWORD size = MAX_PATH;
            std::wstring process_name;

            if (QueryFullProcessImageNameW(process, 0, buffer, &size) != 0)
            {
                process_name = filename_from_path(buffer);
            }

            CloseHandle(process);

            return process_name;
        }

        BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM l_param)
        {
            auto *windows = reinterpret_cast<std::vector<WindowInfo> *>(l_param);

            if (!IsWindowVisible(hwnd))
            {
                return TRUE;
            }

            if (GetWindow(hwnd, GW_OWNER) != nullptr)
            {
                return TRUE;
            }

            const int length = GetWindowTextLengthW(hwnd);

            if (length == 0)
            {
                return TRUE;
            }

            std::wstring title(length + 1, L'\0');

            GetWindowTextW(hwnd, &title[0], static_cast<int>(title.size()));

            title.resize(wcslen(title.c_str()));
            title = trim(title);

            if (title.empty())
            {
                return TRUE;
            }

            wchar_t class_name[256]{};

            GetClassNameW(hwnd, class_name, static_cast<int>(std::size(class_name)));

            DWORD process_id = 0;

            GetWindowThreadProcessId(hwnd, &process_id);

            windows->push_back(WindowInfo{
                hwnd,
                title,
                class_name,
                get_process_name(process_id),
                process_id
            });

            return TRUE;
        }
    }

    std::vector<WindowInfo> Scanner::scan_active_windows() const
    {
        std::vector<WindowInfo> windows;

        EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&windows));

        std::sort(windows.begin(), windows.end(), [](const WindowInfo &left, const WindowInfo &right)
        {
            return left.title < right.title;
        });

        return windows;
    }
}
