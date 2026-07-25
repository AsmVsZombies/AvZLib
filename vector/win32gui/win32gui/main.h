#ifndef __WIN32_GUI_H__
#define __WIN32_GUI_H__

#ifdef UNICODE
#define UNICODE_DEF 1
#else
#define UNICODE_DEF 0
#endif // UNICODE

#if (!UNICODE_DEF)
#define UNICODE
#endif

#include <avz.h>
#include <commctrl.h>
#include <filesystem>
#include <windowsx.h>

inline int _GetDpiForWindow(HWND hwnd)
{
    typedef UINT(WINAPI * GetDpiForWindowPtr)(HWND);
    static GetDpiForWindowPtr pGetDpiForWindow = nullptr;
    if (!pGetDpiForWindow) {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        pGetDpiForWindow = (GetDpiForWindowPtr)GetProcAddress(hUser32, "GetDpiForWindow");
    }
    if (pGetDpiForWindow) {
        UINT dpi = pGetDpiForWindow(hwnd);
        if (dpi != 0)
            return (int)dpi;
    }
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi ? dpi : 96;
}

inline int _Scale(int value, int dpi)
{
    return MulDiv(value, dpi, 96);
}

class __AWindowClass : public AOrderedAfterInjectHook<-100>,
                       public AOrderedBeforeExitHook<100> {
public:
    const wchar_t* GetName() noexcept
    {
        return _wndClassName;
    }
    HINSTANCE GetInstance() noexcept
    {
        return _hInst;
    }
    static auto&& GetOpMap()
    {
        return _opMap;
    }

    __AWindowClass() = default;

protected:

#ifdef ENABLE_COMCTL32_V6

    virtual void _AfterInject() override
    {
        _hInst = AGetDllInstance();

        auto manifestPath = (std::filesystem::path(__FILE__).parent_path() / "win32gui.manifest").wstring();
        ACTCTXW actCtx = {};
        actCtx.cbSize = sizeof(actCtx);
        actCtx.lpSource = manifestPath.c_str();
        _hActCtx = CreateActCtxW(&actCtx);
        if (_hActCtx != INVALID_HANDLE_VALUE) {
            ActivateActCtx(_hActCtx, &_cookie);
        }

        _hComCtl32 = LoadLibraryW(L"comctl32.dll");
        if (_hComCtl32 != nullptr) {
            using InitCommonControlsExFunc = BOOL(WINAPI*)(const INITCOMMONCONTROLSEX*);
            auto initCommonControlsEx = reinterpret_cast<InitCommonControlsExFunc>(
                GetProcAddress(_hComCtl32, "InitCommonControlsEx"));
            if (initCommonControlsEx != nullptr) {
                INITCOMMONCONTROLSEX info = {sizeof(info), ICC_STANDARD_CLASSES};
                initCommonControlsEx(&info);
            }
        }

        WNDCLASSW wc = {};
        wc.lpfnWndProc = _HandleMsg;
        wc.hInstance = _hInst;
        wc.lpszClassName = _wndClassName;
        RegisterClassW(&wc);
    }

    virtual void _BeforeExit() override
    {
        if (_hActCtx != INVALID_HANDLE_VALUE) {
            DeactivateActCtx(0, _cookie);
            ReleaseActCtx(_hActCtx);
            _hActCtx = INVALID_HANDLE_VALUE;
        }
        UnregisterClassW(_wndClassName, _hInst);
        if (_hComCtl32 != nullptr) {
            FreeLibrary(_hComCtl32);
            _hComCtl32 = nullptr;
        }
    }

    HANDLE _hActCtx = INVALID_HANDLE_VALUE;
    HMODULE _hComCtl32 = nullptr;
    ULONG_PTR _cookie = 0;

#else
    virtual void _AfterInject() override
    {
        _hInst = AGetDllInstance();
        WNDCLASSW wc = {};
        wc.lpfnWndProc = _HandleMsg;
        wc.hInstance = GetInstance();
        wc.lpszClassName = GetName();
        RegisterClassW(&wc);
    }

    virtual void _BeforeExit() override
    {
        UnregisterClassW(_wndClassName, GetInstance());
    }

#endif
    __AWindowClass(const __AWindowClass&) = delete;
    __AWindowClass& operator=(const __AWindowClass&) = delete;

    static WINAPI LRESULT _HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        auto childHwnd = (HWND)lParam;
        switch (msg) {
        case WM_COMMAND:
            if (_opMap.contains(childHwnd)) {
                _opMap[childHwnd]();
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
        }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static constexpr const wchar_t* _wndClassName = L"AWindow";
    HINSTANCE _hInst;
    inline static std::unordered_map<HWND, std::function<void()>> _opMap;
};

inline __AWindowClass __aWindowClass;

class ABasicWindow : public AOrderedBeforeExitHook<-1> {
    __ADeleteCopyAndMove(ABasicWindow);

public:
    ABasicWindow() = default;
    virtual void _BeforeExit() override
    {
        if (_hwnd != nullptr) {
            DestroyWindow(_hwnd);
            _hwnd = nullptr;
        }
        if (_font != nullptr) {
            DeleteObject(_font);
        }
    }

    HWND GetHwnd()
    {
        return _hwnd;
    }

    ABasicWindow* GetParent()
    {
        return _parent;
    }

    void SetText(const std::string& text)
    {
        SetWindowTextW(_hwnd, AStrToWstr(text).c_str());
    }

    std::string GetText()
    {
        std::wstring str;
        str.resize(GetWindowTextLengthW(_hwnd) + 1);
        GetWindowTextW(_hwnd, str.data(), str.size());
        return AWStrToStr(str);
    }

    bool SetFont(const std::string& name, int fontSize = 20, bool isBold = false)
    {
        if (_font != nullptr) {
            DeleteObject(_font);
        }
        int scaledSize = _Scale(fontSize, _dpi);
        _font = CreateFontW(scaledSize, 0, 0, 0, isBold ? FW_BOLD : FW_MEDIUM, FALSE, FALSE, false, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, AStrToWstr(name).c_str());
        if (_font == nullptr) {
            return false;
        }
        SendMessage(_hwnd, WM_SETFONT, (WPARAM)_font, TRUE);
        for (auto&& child : _childs) {
            SendMessage(child->GetHwnd(), WM_SETFONT, (WPARAM)_font, TRUE);
        }
        return true;
    }

    int GetWidth() const noexcept
    {
        return _logicWidth;
    }

    virtual int GetHeight()
    {
        return _logicHeight;
    }

    // 返回实际程序的宽度，即 经过dpi适应后的宽度
    int GetPhysicalWidth() const noexcept
    {
        return _width;
    }

    // 返回实际程序的高度，即 经过dpi适应后的高度
    int GetPhysicalHeight() const noexcept
    {
        return _height;
    }

    bool GetEnable() const noexcept
    {
        return _isEnable;
    }

    void SetEnable(bool enable)
    {
        _isEnable = enable;
        EnableWindow(_hwnd, _isEnable);
    }

    void Show()
    {
        ShowWindow(_hwnd, SW_SHOW);
        _isHide = false;
    }

    void Hide()
    {
        ShowWindow(_hwnd, SW_HIDE);
        _isHide = true;
    }

    bool IsHide() const noexcept
    {
        return _isHide;
    }

    int GetDpi() const noexcept
    {
        return _dpi;
    }

    int Scale(int value) const noexcept
    {
        return _Scale(value, _dpi);
    }

protected:
    void _CreateWindow(ABasicWindow* parent, const std::wstring& className, DWORD style,
        const std::string& text, int x, int y, int width = 0, int height = 0)
    {
        // 原始值
        _logicX = x;
        _logicY = y;
        _logicWidth = width;
        _logicHeight = height;

        if (parent != nullptr) {
            _dpi = parent->_dpi;
            _font = parent->_font;
            _isEnable = parent->_isEnable;
            _isHide = parent->_isHide;
        }

        int finalX = x;
        int finalY = y;
        int finalW = width;
        int finalH = height;

        if (parent != nullptr) {
            // 子控件按 DPI 缩放
            finalX = _Scale(x, _dpi);
            finalY = _Scale(y, _dpi);
            finalW = _Scale(width, _dpi);
            finalH = _Scale(height, _dpi);

            // 未指定尺寸时填充剩余客户区
            if (finalW <= 0 || finalH <= 0) {
                RECT rect;
                GetClientRect(parent->GetHwnd(), &rect);
                finalW = std::max<int>(10, rect.right - rect.left - finalX);
                finalH = std::max<int>(10, rect.bottom - rect.top - finalY);
                _logicWidth = _Scale(finalW, 96);  // 得到逻辑宽度
                _logicHeight = _Scale(finalH, 96); // 得到逻辑高度
            }
        }

        if (parent != nullptr) {
            _hwnd = CreateWindowW(className.c_str(), AStrToWstr(text).c_str(), style,
                finalX, finalY, finalW, finalH, parent->GetHwnd(), 0, AGetDllInstance(), 0);
            SendMessage(_hwnd, WM_SETFONT, (WPARAM)(parent->_font), 0);
            _parent = parent;
        } else {
            _hwnd = CreateWindowW(className.c_str(), AStrToWstr(text).c_str(), style,
                finalX, finalY, finalW, finalH, nullptr, 0, AGetDllInstance(), 0);
            if (_hwnd) {
                _dpi = _GetDpiForWindow(_hwnd);
                RECT rect;
                GetWindowRect(_hwnd, &rect);
                _width = rect.right - rect.left;
                _height = rect.bottom - rect.top;
            }
        }

        _width = finalW;
        _height = finalH;
        SetEnable(_isEnable);
        ShowWindow(_hwnd, _isHide ? SW_HIDE : SW_SHOW);
    }

    HWND _hwnd = nullptr;
    ABasicWindow* _parent = nullptr;
    HFONT _font = nullptr;
    int _width = 0;       // 真实宽度
    int _height = 0;      // 真实高度
    int _logicX = 0;      // 逻辑 X
    int _logicY = 0;      // 逻辑 Y
    int _logicWidth = 0;  // 逻辑宽度
    int _logicHeight = 0; // 逻辑高度
    int _dpi = 96;
    std::vector<std::shared_ptr<ABasicWindow>> _childs;
    bool _isEnable = true;
    bool _isHide = false;
};

// 这是为了 AConnect 服务的类
// 继承此类才能使用 AConnect 和控件
class AConnectControl : public ABasicWindow {
public:
    template <typename Op>
    void Connect(Op&& op)
    {
        if (__aWindowClass.GetOpMap().contains(this->GetHwnd())) {
            AGetInternalLogger()->Error("同一个控件无法同时绑定多个操作， 如需绑定新操作，请调用 Stop 将之前的绑定删除");
            return;
        }
        __aWindowClass.GetOpMap()[this->GetHwnd()] = std::forward<Op>(op);
    }

    void Stop()
    {
        __aWindowClass.GetOpMap().erase(this->GetHwnd());
    }
};

class APushButton : public AConnectControl {
public:
    APushButton(ABasicWindow& parent, const std::string& text, int x, int y, int width = 80, int height = 30)
    {
        _CreateWindow(&parent, L"Button", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, text, x, y, width, height);
    }
};

class ACheckBox : public AConnectControl {
public:
    ACheckBox(ABasicWindow& parent, const std::string& text, int x, int y, int width = 80, int height = 30)
    {
        _CreateWindow(&parent, L"Button", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, text, x, y, width, height);
    }

    bool GetCheck()
    {
        return Button_GetCheck(_hwnd);
    }

    void SetCheck(bool check)
    {
        Button_SetCheck(_hwnd, check);
    }
};

class AComboBox : public ABasicWindow {
public:
    AComboBox(ABasicWindow& parent, int x, int y, int width = 80, int height = 300)
    {
        _CreateWindow(&parent, L"ComboBox", CBS_DROPDOWN | WS_CHILD | WS_VISIBLE | WS_VSCROLL, "", x, y, width, height);
    }

    void AddString(const std::string& str)
    {
        ComboBox_AddString(_hwnd, AStrToWstr(str).c_str());
        ComboBox_SetCurSel(_hwnd, 0);
    }

    template <typename... Others>
    void AddString(const std::string& first, Others&&... others)
    {
        AddString(first);
        AddString(std::forward<Others>(others)...);
    }

    void DeleteString(const std::string& str, int startIdx = 0)
    {
        int idx = ComboBox_FindString(_hwnd, startIdx, AStrToWstr(str).c_str());
        ComboBox_DeleteString(_hwnd, idx);
    }

    void DeleteIdx(int idx)
    {
        ComboBox_DeleteString(_hwnd, idx);
    }

    int GetCount()
    {
        return ComboBox_GetCount(_hwnd);
    }

    std::string GetString(int idx = -1)
    {
        if (idx == -1) {
            idx = ComboBox_GetCurSel(_hwnd);
        }
        std::wstring text;
        text.resize(ComboBox_GetLBTextLen(_hwnd, idx) + 1);
        ComboBox_GetLBText(_hwnd, idx, text.data());
        return AWStrToStr(text);
    }

    virtual int GetHeight()
    {
        return ComboBox_GetItemHeight(_hwnd);
    }

    // 得到展开状态下 ComboBox 的长度
    int GetTotalHeight() const noexcept
    {
        return _height;
    }
};

class AEdit : public ABasicWindow {
public:
    AEdit(ABasicWindow& parent, int x, int y, int width = 80, int height = 30, DWORD style = 0)
    {
        _CreateWindow(&parent, L"Edit", WS_VISIBLE | WS_CHILD | WS_BORDER | style, "", x, y, width, height);
    }
};

class ALabel : public ABasicWindow {
public:
    ALabel(ABasicWindow& parent, const std::string& text, int x, int y, int width = 80, int height = 30)
    {
        _CreateWindow(&parent, L"Static", WS_VISIBLE | WS_CHILD | SS_LEFT, text, x, y, width, height);
    }
};

class AWindow : public ABasicWindow {
public:
    AWindow(ABasicWindow& parent, int x, int y, int width = 0, int height = 0)
    {
        _CreateWindow(&parent, __aWindowClass.GetName(), WS_CHILD | WS_VISIBLE,
            "", x, y, width, height);
    }

    AWindow(const AWindow&) = delete;
    AWindow& operator=(const AWindow&) = delete;

    AWindow* AddWindow(int x, int y, int width = 0, int height = 0)
    {
        auto ptr = std::make_shared<AWindow>(*this, x, y, width, height);
        _childs.emplace_back(ptr);
        return ptr.get();
    }

    APushButton* AddPushButton(const std::string& text, int x, int y, int width = 100, int height = 30)
    {
        auto ptr = std::make_shared<APushButton>(*this, text, x, y, width, height);
        _childs.emplace_back(ptr);
        return ptr.get();
    }

    ACheckBox* AddCheckBox(const std::string& text, int x, int y, int width = 100, int height = 30)
    {
        auto ptr = std::make_shared<ACheckBox>(*this, text, x, y, width, height);
        _childs.emplace_back(ptr);
        return ptr.get();
    }

    AComboBox* AddComboBox(int x, int y, int width = 100, int height = 90)
    {
        auto ptr = std::make_shared<AComboBox>(*this, x, y, width, height);
        _childs.emplace_back(ptr);
        return ptr.get();
    }

    // 最后一个参数为 style
    // 设置为 ES_NUMBER 就可以限制为只能输入数字
    // 设置为 ES_MULTILINE 就可以多行输入
    // 多个格式可以同时设定，使用位运算符 | 即可
    // 具体查看 https://learn.microsoft.com/zh-cn/windows/win32/controls/edit-control-styles
    AEdit* AddEdit(const std::string& text, int x, int y, int width = 100, int height = 30, DWORD style = 0)
    {
        auto ptr = std::make_shared<AEdit>(*this, x, y, width, height, style);
        _childs.emplace_back(ptr);
        ptr->SetText(text);
        return ptr.get();
    }

    ALabel* AddLabel(const std::string& text, int x, int y, int width = 100, int height = 30)
    {
        auto ptr = std::make_shared<ALabel>(*this, text.c_str(), x, y, width, height);
        _childs.emplace_back(ptr);
        return ptr.get();
    }

protected:
    AWindow() = default;
};

class AMainWindow : public AWindow,
                    public AOrderedAfterInjectHook<-1> {
public:
    AMainWindow(const std::string& title, int width = 600, int height = 400, bool isActiveInSTMode = true, int x = -1, int y = -1)
    {
        _title = title;
        _x = x;
        _y = y;
        _width = width;
        _height = height;
        _isActiveInSTMode = isActiveInSTMode;
    }

protected:
    virtual void _AfterInject() override
    {
        if (_x < 0) {
            _x = CW_USEDEFAULT;
        }
        if (_y < 0) {
            _y = CW_USEDEFAULT;
        }

        // 先用原始尺寸创建窗口
        _CreateWindow(nullptr, __aWindowClass.GetName(), WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
            _title.c_str(), _x, _y, _width, _height);

        // 根据实际 DPI 调整窗口大小
        if (_hwnd && _dpi != 96) {
            int scaledW = _Scale(_width, _dpi);
            int scaledH = _Scale(_height, _dpi);
            SetWindowPos(_hwnd, NULL, 0, 0, scaledW, scaledH, SWP_NOMOVE | SWP_NOZORDER);
            _width = scaledW;
            _height = scaledH;
            _logicWidth = _width;
            _logicHeight = _height;
        }

        SetFont("Microsoft YaHei");

        auto msgLoop = [this] {
            if (!_isActiveInSTMode && !__aGameControllor.isAdvancedPaused) {
                return;
            }
            MSG msg;
            if (!PeekMessageW(&msg, this->_hwnd, 0, 0, PM_NOREMOVE)) {
                return;
            }
            if (!GetMessageW(&msg, this->_hwnd, 0, 0)) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        };
        _tickRunner.Start(std::move(msgLoop), ATickRunner::AFTER_INJECT);
    }

    ATickRunner _tickRunner;
    std::string _title;
    int _x;
    int _y;
    bool _isActiveInSTMode;
};

class ACtrlConnHandle {
public:
    ACtrlConnHandle() = default;
    ACtrlConnHandle(AConnectControl* ptr)
        : _ptr(ptr)
    {
    }

    void Stop()
    {
        _ptr->Stop();
    }

protected:
    AConnectControl* _ptr = nullptr;
};

template <typename Op>
ACtrlConnHandle AConnect(AConnectControl* ptr, Op&& op)
{
    if (ptr != nullptr) {
        ptr->Connect(std::forward<Op>(op));
    }
    return ACtrlConnHandle(ptr);
}

#if (!UNICODE_DEF)
#undef UNICODE
#endif

#endif // __WIN32_GUI_H__
