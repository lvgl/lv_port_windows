﻿/*
 * PROJECT:   LVGL ported to Windows Desktop
 * FILE:      LVGL.Windows.Desktop.cpp
 * PURPOSE:   Implementation for LVGL ported to Windows Desktop
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <Windows.h>
#include <windowsx.h>

#pragma comment(lib, "Imm32.lib")

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
// operator 'operator-name': deprecated between enumerations of different types
#pragma warning(disable:5054)
#endif

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#include <LVGL.Windows.Font.h>

/**
 * @brief Creates a B8G8R8A8 frame buffer.
 * @param WindowHandle A handle to the window for the creation of the frame
 *                     buffer. If this value is nullptr, the entire screen will
 *                     be referenced.
 * @param Width The width of the frame buffer.
 * @param Height The height of the frame buffer.
 * @param PixelBuffer The raw pixel buffer of the frame buffer you created.
 * @param PixelBufferSize The size of the frame buffer you created.
 * @return If the function succeeds, the return value is a handle to the device
 *         context (DC) for the frame buffer. If the function fails, the return
 *         value is nullptr, and PixelBuffer parameter is nullptr.
*/
EXTERN_C HDC WINAPI LvglCreateFrameBuffer(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_ UINT32** PixelBuffer,
    _Out_ SIZE_T* PixelBufferSize)
{
    HDC hFrameBufferDC = nullptr;

    if (PixelBuffer && PixelBufferSize)
    {
        HDC hWindowDC = ::GetDC(WindowHandle);
        if (hWindowDC)
        {
            hFrameBufferDC = ::CreateCompatibleDC(hWindowDC);
            ::ReleaseDC(WindowHandle, hWindowDC);
        }

        if (hFrameBufferDC)
        {
            BITMAPINFO BitmapInfo = { 0 };
            BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapInfo.bmiHeader.biWidth = Width;
            BitmapInfo.bmiHeader.biHeight = -Height;
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;

            HBITMAP hBitmap = ::CreateDIBSection(
                hFrameBufferDC,
                &BitmapInfo,
                DIB_RGB_COLORS,
                reinterpret_cast<void**>(PixelBuffer),
                nullptr,
                0);
            if (hBitmap)
            {
                *PixelBufferSize = Width * Height * sizeof(UINT32);
                ::DeleteObject(::SelectObject(hFrameBufferDC, hBitmap));
                ::DeleteObject(hBitmap);
            }
            else
            {
                ::DeleteDC(hFrameBufferDC);
                hFrameBufferDC = nullptr;
            }
        }
    }

    return hFrameBufferDC;
}

/**
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
EXTERN_C UINT WINAPI LvglGetDpiForWindow(
    _In_ HWND WindowHandle)
{
    UINT Result = static_cast<UINT>(-1);

    HMODULE ModuleHandle = ::LoadLibraryW(L"SHCore.dll");
    if (ModuleHandle)
    {
        typedef enum MONITOR_DPI_TYPE_PRIVATE {
            MDT_EFFECTIVE_DPI = 0,
            MDT_ANGULAR_DPI = 1,
            MDT_RAW_DPI = 2,
            MDT_DEFAULT = MDT_EFFECTIVE_DPI
        } MONITOR_DPI_TYPE_PRIVATE;

        typedef HRESULT(WINAPI* FunctionType)(
            HMONITOR, MONITOR_DPI_TYPE_PRIVATE, UINT*, UINT*);

        FunctionType pFunction = reinterpret_cast<FunctionType>(
            ::GetProcAddress(ModuleHandle, "GetDpiForMonitor"));
        if (pFunction)
        {
            HMONITOR MonitorHandle = ::MonitorFromWindow(
                WindowHandle,
                MONITOR_DEFAULTTONEAREST);

            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(pFunction(
                MonitorHandle,
                MDT_EFFECTIVE_DPI,
                &dpiX,
                &dpiY)))
            {
                Result = dpiX;
            }
        }

        ::FreeLibrary(ModuleHandle);
    }

    if (Result == static_cast<UINT>(-1))
    {
        HDC hWindowDC = ::GetDC(WindowHandle);
        if (hWindowDC)
        {
            Result = ::GetDeviceCaps(hWindowDC, LOGPIXELSX);
            ::ReleaseDC(WindowHandle, hWindowDC);
        }
    }

    if (Result == static_cast<UINT>(-1))
    {
        Result = USER_DEFAULT_SCREEN_DPI;
    }

    return Result;
}

EXTERN_C BOOL WINAPI LvglEnableChildWindowDpiMessage(
    HWND WindowHandle)
{
    // This hack is only for Windows 10 TH1/TH2 only.
    // We don't need this hack if the Per Monitor Aware V2 is existed.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwMajorVersion = 10;
    OSVersionInfoEx.dwMinorVersion = 0;
    OSVersionInfoEx.dwBuildNumber = 14393;
    if (!::VerifyVersionInfoW(
        &OSVersionInfoEx,
        VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
        ::VerSetConditionMask(
            ::VerSetConditionMask(
                ::VerSetConditionMask(
                    0,
                    VER_MAJORVERSION,
                    VER_GREATER_EQUAL),
                VER_MINORVERSION,
                VER_GREATER_EQUAL),
            VER_BUILDNUMBER,
            VER_LESS)))
    {
        return FALSE;
    }

    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, BOOL);

    FunctionType pFunction = reinterpret_cast<FunctionType>(
        ::GetProcAddress(ModuleHandle, "EnableChildWindowDpiMessage"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(WindowHandle, TRUE);
}

/**
 * @brief Registers a window as being touch-capable.
 * @param hWnd The handle of the window being registered.
 * @param ulFlags A set of bit flags that specify optional modifications.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see RegisterTouchWindow.
*/
EXTERN_C BOOL WINAPI LvglRegisterTouchWindow(
    _In_ HWND hWnd,
    _In_ ULONG ulFlags)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::RegisterTouchWindow)* pFunction =
        reinterpret_cast<decltype(::RegisterTouchWindow)*>(
            ::GetProcAddress(ModuleHandle, "RegisterTouchWindow"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hWnd, ulFlags);
}

/**
 * @brief Retrieves detailed information about touch inputs associated with a
 *        particular touch input handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @param cInputs The number of structures in the pInputs array.
 * @param pInputs A pointer to an array of TOUCHINPUT structures to receive
 *                information about the touch points associated with the
 *                specified touch input handle.
 * @param cbSize The size, in bytes, of a single TOUCHINPUT structure.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see GetTouchInputInfo.
*/
EXTERN_C BOOL WINAPI LvglGetTouchInputInfo(
    _In_ HTOUCHINPUT hTouchInput,
    _In_ UINT cInputs,
    _Out_ PTOUCHINPUT pInputs,
    _In_ int cbSize)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::GetTouchInputInfo)* pFunction =
        reinterpret_cast<decltype(::GetTouchInputInfo)*>(
            ::GetProcAddress(ModuleHandle, "GetTouchInputInfo"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput, cInputs, pInputs, cbSize);
}

/**
 * @brief Closes a touch input handle, frees process memory associated with it,
          and invalidates the handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see CloseTouchInputHandle.
*/
EXTERN_C BOOL WINAPI LvglCloseTouchInputHandle(
    _In_ HTOUCHINPUT hTouchInput)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::CloseTouchInputHandle)* pFunction =
        reinterpret_cast<decltype(::CloseTouchInputHandle)*>(
            ::GetProcAddress(ModuleHandle, "CloseTouchInputHandle"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput);
}




static HINSTANCE g_InstanceHandle = nullptr;
static int volatile g_WindowWidth = 0;
static int volatile g_WindowHeight = 0;
static HWND g_WindowHandle = nullptr;
static int volatile g_WindowDPI = USER_DEFAULT_SCREEN_DPI;
static HDC g_WindowDCHandle = nullptr;

static HDC g_BufferDCHandle = nullptr;
static UINT32* g_PixelBuffer = nullptr;
static SIZE_T g_PixelBufferSize = 0;

static bool volatile g_MousePressed;
static LPARAM volatile g_MouseValue = 0;

static bool volatile g_MouseWheelPressed = false;
static int16_t volatile g_MouseWheelValue = 0;

static bool volatile g_WindowQuitSignal = false;
static bool volatile g_WindowResizingSignal = false;

std::mutex g_KeyboardMutex;
std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> g_KeyQueue;
static uint16_t volatile g_Utf16HighSurrogate = 0;
static uint16_t volatile g_Utf16LowSurrogate = 0;
static lv_group_t* volatile g_DefaultGroup = nullptr;

void LvglDisplayDriverFlushCallback(
    lv_disp_drv_t* disp_drv,
    const lv_area_t* area,
    lv_color_t* color_p)
{
    UNREFERENCED_PARAMETER(color_p);

    if (::lv_disp_flush_is_last(disp_drv))
    {
        lv_coord_t Width = ::lv_area_get_width(area);
        lv_coord_t Height = ::lv_area_get_height(area);

        ::BitBlt(
            g_WindowDCHandle,
            area->x1,
            area->y1,
            Width,
            Height,
            g_BufferDCHandle,
            area->x1,
            area->y1,
            SRCCOPY);
    }

    ::lv_disp_flush_ready(disp_drv);
}

#include <lvgl/src/draw/sw/lv_draw_sw.h>

typedef lv_draw_sw_ctx_t LvglWindowsGdiRendererContext;

std::map<std::uint32_t, HBRUSH> g_SolidBrushCache;

void LvglWindowsGdiRendererBlendCallback(
    lv_draw_ctx_t* draw_ctx,
    const lv_draw_sw_blend_dsc_t* dsc)
{
    // Let's get the blend area which is the intersection of the area to fill
    // and the clip area.
    lv_area_t blend_area;
    if (!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area))
    {
        return;
    }

    // Fallback: The GPU doesn't support these settings. Call the Software
    // Renderer.
    if (!(
        dsc->mask_buf == nullptr &&
        dsc->opa >= LV_OPA_MAX &&
        dsc->blend_mode == LV_BLEND_MODE_NORMAL))
    {
        ::lv_draw_sw_blend_basic(draw_ctx, dsc);
        return;
    }

    if (dsc->src_buf)
    {
        lv_coord_t Width = ::lv_area_get_width(&blend_area);
        lv_coord_t Height = ::lv_area_get_height(&blend_area);

        BITMAPINFO BitmapInfo = { 0 };
        BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        BitmapInfo.bmiHeader.biWidth = Width;
        BitmapInfo.bmiHeader.biHeight = -Height;
        BitmapInfo.bmiHeader.biPlanes = 1;
        BitmapInfo.bmiHeader.biBitCount = 32;
        BitmapInfo.bmiHeader.biCompression = BI_RGB;

        ::StretchDIBits(
            g_BufferDCHandle,
            blend_area.x1,
            blend_area.y1,
            Width,
            Height,
            0,
            0,
            Width,
            Height,
            dsc->src_buf,
            &BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
    else
    {
        // Fill only non masked, fully opaque, normal blended and not too small
        // areas.

        HBRUSH Brush = nullptr;
        {
            std::uint32_t Index = ::lv_color_to32(dsc->color);
            auto Iterator = g_SolidBrushCache.find(Index);
            if (Iterator != g_SolidBrushCache.end())
            {
                Brush = Iterator->second;
            }
            else
            {
                Brush = ::CreateSolidBrush(RGB(
                    LV_COLOR_GET_R(dsc->color),
                    LV_COLOR_GET_G(dsc->color),
                    LV_COLOR_GET_B(dsc->color)));
                if (Brush)
                {
                    g_SolidBrushCache.emplace(std::make_pair(Index, Brush));
                }
            }
        }

        if (Brush)
        {
            RECT RenderArea;
            RenderArea.left = blend_area.x1;
            RenderArea.top = blend_area.y1;
            RenderArea.right = blend_area.x2 + 1;
            RenderArea.bottom = blend_area.y2 + 1;
            ::FillRect(g_BufferDCHandle, &RenderArea, Brush);
        }
    }
}

void LvglWindowsGdiRendererBaseDrawWaitForFinishCallback(
    lv_draw_ctx_t* draw_ctx)
{
    ::lv_draw_sw_wait_for_finish(draw_ctx);
}

void LvglWindowsGdiRendererInitialize(
    lv_disp_drv_t* drv,
    lv_draw_ctx_t* draw_ctx)
{
    // Initialize the LVGL Software Renderer
    ::lv_draw_sw_init_ctx(drv, draw_ctx);

    LvglWindowsGdiRendererContext* RendererContext =
        reinterpret_cast<LvglWindowsGdiRendererContext*>(draw_ctx);

    RendererContext->blend =
        LvglWindowsGdiRendererBlendCallback;
    RendererContext->base_draw.wait_for_finish =
        LvglWindowsGdiRendererBaseDrawWaitForFinishCallback;
}

void LvglCreateDisplayDriver(
    lv_disp_drv_t* disp_drv,
    int hor_res,
    int ver_res)
{
    HDC hNewBufferDC = ::LvglCreateFrameBuffer(
        g_WindowHandle,
        hor_res,
        ver_res,
        &g_PixelBuffer,
        &g_PixelBufferSize);

    ::DeleteDC(g_BufferDCHandle);
    g_BufferDCHandle = hNewBufferDC;

    lv_disp_draw_buf_t* disp_buf = new lv_disp_draw_buf_t();
    ::lv_disp_draw_buf_init(
        disp_buf,
        g_PixelBuffer,
        nullptr,
        hor_res * ver_res);

    disp_drv->hor_res = static_cast<lv_coord_t>(hor_res);
    disp_drv->ver_res = static_cast<lv_coord_t>(ver_res);
    disp_drv->flush_cb = ::LvglDisplayDriverFlushCallback;
    if (disp_drv->draw_buf != NULL)
        delete disp_drv->draw_buf;
    disp_drv->draw_buf = disp_buf;
    disp_drv->dpi = g_WindowDPI;
    disp_drv->direct_mode = 1;
    disp_drv->draw_ctx_init = LvglWindowsGdiRendererInitialize;
    disp_drv->draw_ctx_size = sizeof(LvglWindowsGdiRendererContext);
}

void LvglMouseDriverReadCallback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    data->state = static_cast<lv_indev_state_t>(
        g_MousePressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->point.x = GET_X_LPARAM(g_MouseValue);
    data->point.y = GET_Y_LPARAM(g_MouseValue);
}

void LvglKeyboardDriverReadCallback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

    if (!g_KeyQueue.empty())
    {
        auto Current = g_KeyQueue.front();

        data->key = Current.first;
        data->state = Current.second;

        g_KeyQueue.pop();
    }

    if (!g_KeyQueue.empty())
    {
        data->continue_reading = true;
    }
}

void LvglMousewheelDriverReadCallback(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    data->state = static_cast<lv_indev_state_t>(
        g_MouseWheelPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->enc_diff = g_MouseWheelValue;
    g_MouseWheelValue = 0;
}

LRESULT CALLBACK WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        RECT WindowRect;
        ::GetClientRect(hWnd, &WindowRect);

        g_WindowWidth = WindowRect.right - WindowRect.left;
        g_WindowHeight = WindowRect.bottom - WindowRect.top;

        g_WindowDCHandle = ::GetDC(hWnd);

        return 0;
    }
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        g_MouseValue = lParam;
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
        {
            g_MousePressed = (uMsg == WM_LBUTTONDOWN);
        }
        else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
        {
            g_MouseWheelPressed = (uMsg == WM_MBUTTONDOWN);
        }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

        bool SkipTranslation = false;
        std::uint32_t TranslatedKey = 0;

        switch (wParam)
        {
        case VK_UP:
            TranslatedKey = LV_KEY_UP;
            break;
        case VK_DOWN:
            TranslatedKey = LV_KEY_DOWN;
            break;
        case VK_LEFT:
            TranslatedKey = LV_KEY_LEFT;
            break;
        case VK_RIGHT:
            TranslatedKey = LV_KEY_RIGHT;
            break;
        case VK_ESCAPE:
            TranslatedKey = LV_KEY_ESC;
            break;
        case VK_DELETE:
            TranslatedKey = LV_KEY_DEL;
            break;
        case VK_BACK:
            TranslatedKey = LV_KEY_BACKSPACE;
            break;
        case VK_RETURN:
            TranslatedKey = LV_KEY_ENTER;
            break;
        case VK_NEXT:
            TranslatedKey = LV_KEY_NEXT;
            break;
        case VK_PRIOR:
            TranslatedKey = LV_KEY_PREV;
            break;
        case VK_HOME:
            TranslatedKey = LV_KEY_HOME;
            break;
        case VK_END:
            TranslatedKey = LV_KEY_END;
            break;
        default:
            SkipTranslation = true;
            break;
        }

        if (!SkipTranslation)
        {
            g_KeyQueue.push(std::make_pair(
                TranslatedKey,
                static_cast<lv_indev_state_t>(
                    (uMsg == WM_KEYUP)
                    ? LV_INDEV_STATE_REL
                    : LV_INDEV_STATE_PR)));
        }

        break;
    }
    case WM_CHAR:
    {
        std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

        uint16_t RawCodePoint = static_cast<std::uint16_t>(wParam);

        if (RawCodePoint >= 0x20 && RawCodePoint != 0x7F)
        {
            if (IS_HIGH_SURROGATE(RawCodePoint))
            {
                g_Utf16HighSurrogate = RawCodePoint;
            }
            else if (IS_LOW_SURROGATE(RawCodePoint))
            {
                g_Utf16LowSurrogate = RawCodePoint;
            }

            uint32_t CodePoint = RawCodePoint;

            if (g_Utf16HighSurrogate && g_Utf16LowSurrogate)
            {
                CodePoint = (g_Utf16LowSurrogate & 0x03FF);
                CodePoint += (((g_Utf16HighSurrogate & 0x03FF) + 0x40) << 10);

                g_Utf16HighSurrogate = 0;
                g_Utf16LowSurrogate = 0;
            }

            uint32_t LvglCodePoint = ::_lv_txt_unicode_to_encoded(CodePoint);

            g_KeyQueue.push(std::make_pair(
                LvglCodePoint,
                static_cast<lv_indev_state_t>(LV_INDEV_STATE_PR)));

            g_KeyQueue.push(std::make_pair(
                LvglCodePoint,
                static_cast<lv_indev_state_t>(LV_INDEV_STATE_REL)));
        }

        break;
    }
    case WM_IME_SETCONTEXT:
    {
        if (wParam == TRUE)
        {
            HIMC hInputMethodContext = ::ImmGetContext(hWnd);
            if (hInputMethodContext)
            {
                ::ImmAssociateContext(hWnd, hInputMethodContext);
                ::ImmReleaseContext(hWnd, hInputMethodContext);
            }
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
    case WM_IME_STARTCOMPOSITION:
    {
        HIMC hInputMethodContext = ::ImmGetContext(hWnd);
        if (hInputMethodContext)
        {
            lv_obj_t* TextareaObject = nullptr;
            lv_obj_t* FocusedObject = ::lv_group_get_focused(g_DefaultGroup);
            if (FocusedObject)
            {
                const lv_obj_class_t* ObjectClass = ::lv_obj_get_class(
                    FocusedObject);

                if (ObjectClass == &lv_textarea_class)
                {
                    TextareaObject = FocusedObject;
                }
                else if (ObjectClass == &lv_keyboard_class)
                {
                    TextareaObject = ::lv_keyboard_get_textarea(FocusedObject);
                }
            }

            COMPOSITIONFORM CompositionForm;
            CompositionForm.dwStyle = CFS_POINT;
            CompositionForm.ptCurrentPos.x = 0;
            CompositionForm.ptCurrentPos.y = 0;

            if (TextareaObject)
            {
                lv_textarea_t* Textarea = reinterpret_cast<lv_textarea_t*>(
                    TextareaObject);
                lv_obj_t* Label = ::lv_textarea_get_label(TextareaObject);

                CompositionForm.ptCurrentPos.x =
                    Label->coords.x1 + Textarea->cursor.area.x1;
                CompositionForm.ptCurrentPos.y =
                    Label->coords.y1 + Textarea->cursor.area.y1;
            }

            ::ImmSetCompositionWindow(hInputMethodContext, &CompositionForm);
            ::ImmReleaseContext(hWnd, hInputMethodContext);
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
    case WM_MOUSEWHEEL:
    {
        g_MouseWheelValue = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        break;
    }
    case WM_TOUCH:
    {
        UINT cInputs = LOWORD(wParam);
        HTOUCHINPUT hTouchInput = reinterpret_cast<HTOUCHINPUT>(lParam);

        PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
        if (pInputs)
        {
            if (::LvglGetTouchInputInfo(
                hTouchInput,
                cInputs,
                pInputs,
                sizeof(TOUCHINPUT)))
            {
                for (UINT i = 0; i < cInputs; ++i)
                {
                    POINT Point;
                    Point.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
                    Point.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
                    if (!::ScreenToClient(hWnd, &Point))
                    {
                        continue;
                    }

                    std::uint16_t x = static_cast<std::uint16_t>(
                        Point.x & 0xffff);
                    std::uint16_t y = static_cast<std::uint16_t>(
                        Point.y & 0xffff);

                    DWORD MousePressedMask =
                        TOUCHEVENTF_MOVE | TOUCHEVENTF_DOWN;

                    g_MouseValue = (y << 16) | x;
                    g_MousePressed = (pInputs[i].dwFlags & MousePressedMask);
                }
            }

            delete[] pInputs;
        }

        ::LvglCloseTouchInputHandle(hTouchInput);

        break;
    }
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            int CurrentWindowWidth = LOWORD(lParam);
            int CurrentWindowHeight = HIWORD(lParam);
            if (CurrentWindowWidth != g_WindowWidth ||
                CurrentWindowHeight != g_WindowHeight)
            {
                g_WindowWidth = CurrentWindowWidth;
                g_WindowHeight = CurrentWindowHeight;

                g_WindowResizingSignal = true;
            }
        }
        break;
    }
    case WM_DPICHANGED:
    {
        g_WindowDPI = HIWORD(wParam);

        // Resize the window
        auto lprcNewScale = reinterpret_cast<RECT*>(lParam);

        ::SetWindowPos(
            hWnd,
            nullptr,
            lprcNewScale->left,
            lprcNewScale->top,
            lprcNewScale->right - lprcNewScale->left,
            lprcNewScale->bottom - lprcNewScale->top,
            SWP_NOZORDER | SWP_NOACTIVATE);

        break;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

#include "resource.h"

bool LvglWindowsInitialize(
    _In_ HINSTANCE hInstance,
    _In_ int nShowCmd,
    _In_opt_ LPCWSTR DefaultFontName)
{
    ::LvglWindowsGdiFontInitialize(DefaultFontName);

    HICON IconHandle = ::LoadIconW(
        ::GetModuleHandleW(nullptr),
        MAKEINTRESOURCE(IDI_LVGL));

    WNDCLASSEXW WindowClass;

    WindowClass.cbSize = sizeof(WNDCLASSEXW);

    WindowClass.style = 0;
    WindowClass.lpfnWndProc = ::WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = IconHandle;
    WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = nullptr;
    WindowClass.lpszClassName = L"lv_port_windows";
    WindowClass.hIconSm = IconHandle;

    if (!::RegisterClassExW(&WindowClass))
    {
        return false;
    }

    g_InstanceHandle = hInstance;

    g_WindowHandle = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WindowClass.lpszClassName,
        L"LVGL ported to Windows Desktop",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!g_WindowHandle)
    {
        return false;
    }

    ::LvglRegisterTouchWindow(g_WindowHandle, 0);

    ::LvglEnableChildWindowDpiMessage(g_WindowHandle);
    g_WindowDPI = ::LvglGetDpiForWindow(g_WindowHandle);

    static lv_disp_drv_t disp_drv;
    ::lv_disp_drv_init(&disp_drv);
    ::LvglCreateDisplayDriver(&disp_drv, g_WindowWidth, g_WindowHeight);
    ::lv_disp_drv_register(&disp_drv);

    g_DefaultGroup = ::lv_group_create();
    ::lv_group_set_default(g_DefaultGroup);

    static lv_indev_drv_t indev_drv;
    ::lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ::LvglMouseDriverReadCallback;
    ::lv_indev_set_group(
        ::lv_indev_drv_register(&indev_drv),
        g_DefaultGroup);

    static lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = ::LvglKeyboardDriverReadCallback;
    ::lv_indev_set_group(
        ::lv_indev_drv_register(&kb_drv),
        g_DefaultGroup);

    static lv_indev_drv_t enc_drv;
    lv_indev_drv_init(&enc_drv);
    enc_drv.type = LV_INDEV_TYPE_ENCODER;
    enc_drv.read_cb = ::LvglMousewheelDriverReadCallback;
    ::lv_indev_set_group(
        ::lv_indev_drv_register(&enc_drv),
        g_DefaultGroup);

    ::ShowWindow(g_WindowHandle, nShowCmd);
    ::UpdateWindow(g_WindowHandle);

    return true;
}

void LvglTaskSchedulerLoop()
{
    while (!g_WindowQuitSignal)
    {
        if (g_WindowResizingSignal)
        {
            lv_disp_t* CurrentDisplay = ::lv_disp_get_default();
            if (CurrentDisplay)
            {
                ::LvglCreateDisplayDriver(
                    CurrentDisplay->driver,
                    g_WindowWidth,
                    g_WindowHeight);
                ::lv_disp_drv_update(
                    CurrentDisplay,
                    CurrentDisplay->driver);

                ::lv_refr_now(CurrentDisplay);
            }

            g_WindowResizingSignal = false;
        }

        ::lv_timer_handler();
        ::Sleep(1);
    }
}

int LvglWindowsLoop()
{
    MSG Message;
    while (::GetMessageW(&Message, nullptr, 0, 0))
    {
        ::TranslateMessage(&Message);
        ::DispatchMessageW(&Message);

        if (Message.message == WM_QUIT)
        {
            g_WindowQuitSignal = true;
        }
    }

    return static_cast<int>(Message.wParam);
}

#include <thread>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    ::lv_init();

    if (!LvglWindowsInitialize(
        hInstance,
        nShowCmd,
        nullptr))
    {
        return -1;
    }

    //::lv_demo_widgets();
    //::lv_demo_keypad_encoder();
    ::lv_demo_benchmark();

    std::thread(::LvglTaskSchedulerLoop).detach();

    return ::LvglWindowsLoop();
}
