#include "gui_tool.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <regex>
#include <algorithm>
#include <memory>
#include <array>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

namespace agent {

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string GuiTool::encode_base64(const std::vector<unsigned char>& data) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string GuiTool::get_clipboard_text() {
#ifdef _WIN32
    if (!OpenClipboard(NULL)) {
        return "Error: Unable to open clipboard.";
    }
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) {
        CloseClipboard();
        return "";
    }
    wchar_t* pwszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (pwszText == NULL) {
        CloseClipboard();
        return "";
    }
    
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, pwszText, -1, NULL, 0, NULL, NULL);
    std::string text(utf8_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, pwszText, -1, &text[0], utf8_size, NULL, NULL);
    
    if (!text.empty() && text.back() == '\0') {
        text.pop_back();
    }
    
    GlobalUnlock(hData);
    CloseClipboard();
    return text;
#else
    return "Clipboard text extraction supported on Windows only.";
#endif
}

std::string GuiTool::capture_screenshot(const std::string& output_path) {
#ifdef _WIN32
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmScreen);
    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hbmOld);
    
    BITMAP bmp;
    GetObject(hbmScreen, sizeof(BITMAP), &bmp);
    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    
    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    std::vector<unsigned char> pixels(dwBmpSize);
    GetDIBits(hdcScreen, hbmScreen, 0, (UINT)bmp.bmHeight, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    
    BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;

    std::filesystem::path fp = output_path;
    if (fp.has_parent_path() && !std::filesystem::exists(fp.parent_path())) {
        std::filesystem::create_directories(fp.parent_path());
    }
    std::ofstream file(output_path, std::ios::binary);
    if(file) {
        file.write((const char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
        file.write((const char*)&bi, sizeof(BITMAPINFOHEADER));
        file.write((const char*)pixels.data(), dwBmpSize);
    }
    
    DeleteObject(hbmScreen);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return encode_base64(pixels);
#else
    return "Screenshot supported on Windows only in this demo.";
#endif
}

std::string GuiTool::execute(const nlohmann::json& args) {
    std::string action;
    nlohmann::json effective_args = args;

    if (args.is_object() && args.contains("action") && args["action"].is_string()) {
        action = args["action"].get<std::string>();
    } else if (args.is_object() && args.contains("raw_input") && args["raw_input"].is_string()) {
        // Auto-infer action when LLM sends plain-text raw_input
        std::string raw = args["raw_input"].get<std::string>();
        std::string lower_raw;
        for (char c : raw) lower_raw.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

        if (lower_raw.find("screenshot") != std::string::npos) {
            action = "screenshot";
        } else if (lower_raw.find("click") != std::string::npos) {
            action = "click";
            std::smatch m;
            std::regex re("(\\d+)\\s*[,\\s]\\s*(\\d+)");
            if (std::regex_search(raw, m, re)) {
                effective_args["x"] = std::stoi(m[1].str());
                effective_args["y"] = std::stoi(m[2].str());
            }
        } else if (lower_raw.find("type") != std::string::npos || lower_raw.find("write") != std::string::npos) {
            action = "type";
            size_t pos = raw.find_first_of("':\" ");
            if (pos != std::string::npos && pos + 1 < raw.length()) {
                effective_args["text"] = raw.substr(pos + 1);
            } else {
                effective_args["text"] = raw;
            }
        } else if (lower_raw.find("sleep") != std::string::npos || lower_raw.find("wait") != std::string::npos) {
            action = "sleep";
            std::smatch m;
            std::regex re("(\\d+)");
            if (std::regex_search(raw, m, re)) {
                effective_args["ms"] = std::stoi(m[1].str());
            } else {
                effective_args["ms"] = 2000;
            }
        } else if (lower_raw.find("enter") != std::string::npos || lower_raw.find("return") != std::string::npos) {
            action = "key";
            effective_args["key"] = "enter";
        } else if (lower_raw.find("key") != std::string::npos) {
            action = "key";
            effective_args["key"] = raw;
        } else if (lower_raw.find("clipboard") != std::string::npos || lower_raw.find("copy") != std::string::npos) {
            action = "clipboard";
        } else {
            action = "screenshot";
        }
    } else if (args.is_object() && !args.empty()) {
        // Smart fallback auto-inference when LLM omits the 'action' key in JSON object
        if (args.contains("text")) action = "type";
        else if (args.contains("ms")) action = "sleep";
        else if (args.contains("key")) action = "key";
        else if (args.contains("x") && args.contains("y")) action = "click";
        else action = "clipboard";
    } else {
        return "Error: Missing action parameter for GUI tool.";
    }

#ifdef _WIN32
    if (action == "screenshot") {
        std::string out_path = effective_args.value("path", "outputs/screen captures/screenshot.bmp");
        capture_screenshot(out_path);
        return "Screenshot captured and saved to '" + out_path + "'.";
    } else if (action == "click") {
        int x = effective_args.value("x", 100);
        int y = effective_args.value("y", 200);
        SetCursorPos(x, y);
        mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, x, y, 0, 0);
        return "Clicked at " + std::to_string(x) + ", " + std::to_string(y);
    }
    
    if (action == "type" || action == "key") {
        HWND consoleWnd = GetConsoleWindow();
        HWND foregroundWnd = GetForegroundWindow();
        DWORD foregroundPid = 0;
        GetWindowThreadProcessId(foregroundWnd, &foregroundPid);

        char processName[MAX_PATH] = {0};
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, foregroundPid);
        if (hProcess) {
            DWORD size = MAX_PATH;
            QueryFullProcessImageNameA(hProcess, 0, processName, &size);
            CloseHandle(hProcess);
        }
        std::string procNameStr = processName;
        std::transform(procNameStr.begin(), procNameStr.end(), procNameStr.begin(), ::tolower);

        if ((consoleWnd != NULL && foregroundWnd == consoleWnd) || 
            foregroundPid == GetCurrentProcessId() || 
            procNameStr.find("cmd.exe") != std::string::npos ||
            procNameStr.find("powershell.exe") != std::string::npos ||
            procNameStr.find("pwsh.exe") != std::string::npos ||
            procNameStr.find("windowsterminal.exe") != std::string::npos ||
            procNameStr.find("conhost.exe") != std::string::npos) {
            return "Error: Cannot execute GUI typing into the CLI terminal itself. Return responses directly as conversational text.";
        }
    }

    if (action == "type") {
        std::string text = effective_args.value("text", "");
        
        std::string cmd = "powershell.exe -NoProfile -Command \"Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('" + text + "')\" 2>&1";
        
        std::string result;
        std::array<char, 256> buffer;
#if defined(_WIN32)
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif
        if (pipe) {
            while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
                result += buffer.data();
            }
        }
        
        if (!result.empty() && result.find("Exception") != std::string::npos) {
            return result;
        }

        return "Typed: " + text;
    } else if (action == "key") {
        std::string k = effective_args.value("key", "enter");
        if (k == "enter" || k == "Return") {
            keybd_event(VK_RETURN, 0, 0, 0);
            keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
            return "Pressed key: Enter";
        } else if (k == "ctrl+c" || k == "copy") {
            keybd_event(VK_CONTROL, 0, 0, 0);
            keybd_event('C', 0, 0, 0);
            keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
            return "Pressed key sequence: Ctrl+C";
        } else if (k == "ctrl+a" || k == "select_all") {
            keybd_event(VK_CONTROL, 0, 0, 0);
            keybd_event('A', 0, 0, 0);
            keybd_event('A', 0, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
            return "Pressed key sequence: Ctrl+A";
        }
        return "Pressed key: " + k;
    } else if (action == "sleep") {
        int ms = effective_args.value("ms", 1000);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return "Slept for " + std::to_string(ms) + " ms.";
    } else if (action == "clipboard" || action == "get_clipboard" || action == "read_clipboard") {
        std::string clip_text = get_clipboard_text();
        return clip_text.empty() ? "Clipboard is empty." : clip_text;
    }
    return "Error: Unknown GUI action '" + action + "'.";
#else
    return "Error: GUI actions supported on Windows only in this build.";
#endif
}

} // namespace agent
