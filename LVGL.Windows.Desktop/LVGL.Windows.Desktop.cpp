/*
 * PROJECT:   LVGL ported to Windows Desktop
 * FILE:      LVGL.Windows.Desktop.cpp
 * PURPOSE:   Implementation for LVGL ported to Windows Desktop
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "LVGL.Windows.h"

#include <Windows.h>
#include <windowsx.h>

//#include <d3dkmthk.h>
//
//#include <dwmapi.h>
//#pragma comment(lib,"dwmapi.lib")

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
#endif

#include "lvgl/lvgl.h"
#include "lv_examples/lv_examples.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#include "LVGL.Windows.SymbolFont.h"




typedef struct lv_font_fmt_win_gdi_dsc_struct
{
    HDC FontDCHandle;
    HDC SymbolFontDCHandle;
    //TEXTMETRICW TextMetrics;
    OUTLINETEXTMETRICW OutlineTextMetrics;
    std::map<std::uint32_t, std::pair<GLYPHMETRICS, std::uint8_t*>> GlyphSet;
} lv_font_fmt_win_gdi_dsc_t;

void win_gdi_add_glyph(
    lv_font_fmt_win_gdi_dsc_t* dsc,
    std::uint32_t UnicodeLetter)
{
    MAT2 TransformationMatrix;
    TransformationMatrix.eM11.fract = 0;
    TransformationMatrix.eM11.value = 1;
    TransformationMatrix.eM12.fract = 0;
    TransformationMatrix.eM12.value = 0;
    TransformationMatrix.eM21.fract = 0;
    TransformationMatrix.eM21.value = 0;
    TransformationMatrix.eM22.fract = 0;
    TransformationMatrix.eM22.value = 1;

    bool IsSymbol = false;
    switch (UnicodeLetter)
    {
    case 61441:
    case 61448:
    case 61451:
    case 61452:
    case 61453:
    case 61457:
    case 61459:
    case 61461:
    case 61465:
    case 61468:
    case 61473:
    case 61478:
    case 61479:
    case 61480:
    case 61502:
    case 61512:
    case 61515:
    case 61516:
    case 61517:
    case 61521:
    case 61522:
    case 61523:
    case 61524:
    case 61543:
    case 61544:
    case 61550:
    case 61552:
    case 61553:
    case 61556:
    case 61559:
    case 61560:
    case 61561:
    case 61563:
    case 61587:
    case 61589:
    case 61636:
    case 61637:
    case 61639:
    case 61671:
    case 61674:
    case 61683:
    case 61724:
    case 61732:
    case 61787:
    case 61931:
    case 62016:
    case 62017:
    case 62018:
    case 62019:
    case 62020:
    case 62087:
    case 62099:
    case 62212:
    case 62189:
    case 62810:
    case 63426:
    case 63650:
    case 20042:
        IsSymbol = true;
    default:
        break;
    }

    wchar_t InBuffer[2];
    InBuffer[0] = static_cast<wchar_t>(UnicodeLetter);
    InBuffer[1] = L'\0';

    WORD OutBuffer[2];
    OutBuffer[0] = 0;
    OutBuffer[1] = 0;

    if (::GetGlyphIndicesW(
        IsSymbol ? dsc->SymbolFontDCHandle : dsc->FontDCHandle,
        InBuffer,
        1,
        OutBuffer,
        GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR)
    {
        return;
    }

    GLYPHMETRICS GlyphMetrics;
    uint8_t* GlyphBitmap = nullptr;

    DWORD Length = ::GetGlyphOutlineW(
        IsSymbol ? dsc->SymbolFontDCHandle : dsc->FontDCHandle,
        OutBuffer[0],
        GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX,
        &GlyphMetrics,
        0,
        nullptr,
        &TransformationMatrix);
    if (Length != GDI_ERROR)
    {
        if (Length > 0)
        {
            GlyphBitmap = new uint8_t[Length];
            if (GlyphBitmap)
            {
                if (::GetGlyphOutlineW(
                    IsSymbol ? dsc->SymbolFontDCHandle : dsc->FontDCHandle,
                    OutBuffer[0],
                    GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX,
                    &GlyphMetrics,
                    Length,
                    GlyphBitmap,
                    &TransformationMatrix) != GDI_ERROR)
                {
                    for (size_t i = 0; i < Length; ++i)
                    {
                        GlyphBitmap[i] =
                            GlyphBitmap[i] == 0x40
                            ? 0xFF
                            : GlyphBitmap[i] << 2;
                    }
                }
            }
        }

        dsc->GlyphSet.emplace(std::make_pair(
            UnicodeLetter,
            std::make_pair(GlyphMetrics, GlyphBitmap)));
    }
}

bool win_gdi_get_glyph_dsc(
    const lv_font_t* font,
    lv_font_glyph_dsc_t* dsc_out,
    uint32_t unicode_letter,
    uint32_t unicode_letter_next)
{
    unicode_letter_next;

    lv_font_fmt_win_gdi_dsc_t* dsc =
        reinterpret_cast<lv_font_fmt_win_gdi_dsc_t*>(font->dsc);

    auto iterator = dsc->GlyphSet.find(unicode_letter);
    if (iterator == dsc->GlyphSet.end())
    {
        ::win_gdi_add_glyph(
            dsc,
            unicode_letter);

        iterator = dsc->GlyphSet.find(unicode_letter);
        if (iterator == dsc->GlyphSet.end())
        {
            return false;
        }
    }

    GLYPHMETRICS& GlyphMetrics = iterator->second.first;

    /*int face__glyph__bitmap__rows = dsc->TextMetrics.tmHeight;
    int face__glyph__bitmap__width = dsc->TextMetrics.tmMaxCharWidth;
    int face__glyph__bitmap_left = 0;
    int face__glyph__bitmap_top = -dsc->TextMetrics.tmAscent;*/

    dsc_out->adv_w = GlyphMetrics.gmCellIncX;
    dsc_out->box_w = static_cast<std::uint16_t>(
        (GlyphMetrics.gmBlackBoxX + 0x0003) & 0xFFFC);
    dsc_out->box_h = static_cast<std::uint16_t>(
        GlyphMetrics.gmBlackBoxY);
    dsc_out->ofs_x = static_cast<std::int16_t>(
        GlyphMetrics.gmptGlyphOrigin.x);
    dsc_out->ofs_y = static_cast<std::int16_t>(
        GlyphMetrics.gmptGlyphOrigin.y - GlyphMetrics.gmBlackBoxY);
    dsc_out->bpp = 8;

    /*OUTLINETEXTMETRICW OutlineTextMetrics;
    if (::GetOutlineTextMetricsW(
        dsc->FontDCHandle,
        sizeof(OUTLINETEXTMETRICW),
        &OutlineTextMetrics))
    {
        dsc_out->adv_w = GlyphMetrics.gmCellIncX;
        dsc_out->box_w = static_cast<std::uint16_t>(
            (GlyphMetrics.gmBlackBoxX + 0x0003) & ~0x0003);
        dsc_out->box_h = static_cast<std::uint16_t>(
            GlyphMetrics.gmBlackBoxY);
        dsc_out->ofs_x = static_cast<std::int16_t>(
            GlyphMetrics.gmptGlyphOrigin.x);
        dsc_out->ofs_y = static_cast<std::int16_t>(
            GlyphMetrics.gmptGlyphOrigin.y - GlyphMetrics.gmBlackBoxY);
    }*/

    //dsc_out->adv_w = static_cast<std::uint16_t>(
    //    GlyphMetrics.gmCellIncX);
    //dsc_out->box_w = static_cast<std::uint16_t>(
    //    (face__glyph__bitmap__width + 3) & ~0x0003);
    //    //face__glyph__bitmap__width);
    //dsc_out->box_h = static_cast<std::uint16_t>(
    //    face__glyph__bitmap__rows);
    //dsc_out->ofs_x = static_cast<std::uint16_t>(
    //    face__glyph__bitmap_left);
    //dsc_out->ofs_y = static_cast<std::uint16_t>(
    //    face__glyph__bitmap_top - face__glyph__bitmap__rows);
    //dsc_out->bpp = 8;

    return true;
}

const uint8_t* win_gdi_get_glyph_bitmap(
    const lv_font_t* font,
    uint32_t unicode_letter)
{
    lv_font_fmt_win_gdi_dsc_t* dsc =
        reinterpret_cast<lv_font_fmt_win_gdi_dsc_t*>(font->dsc);

    auto iterator = dsc->GlyphSet.find(unicode_letter);
    if (iterator == dsc->GlyphSet.end())
    {
        ::win_gdi_add_glyph(
            dsc,
            unicode_letter);

        iterator = dsc->GlyphSet.find(unicode_letter);
        if (iterator == dsc->GlyphSet.end())
        {
            return nullptr;
        }
    }

    return iterator->second.second;
}

lv_font_t* lv_win_gdi_create_font(
    _In_ HWND WindowHandle,
    _In_ int FontSize,
    _In_opt_ LPCWSTR FontName)
{
    HDC FontDCHandle = ::GetDC(WindowHandle);
    if (FontDCHandle)
    {
        HFONT FontHandle = ::CreateFontW(
            -FontSize,                  // nHeight
            0,                         // nWidth
            0,                         // nEscapement
            0,                         // nOrientation
            FW_NORMAL,                 // nWeight
            FALSE,                     // bItalic
            FALSE,                     // bUnderline
            0,                         // cStrikeOut
            DEFAULT_CHARSET,           // nCharSet
            OUT_DEFAULT_PRECIS,        // nOutPrecision
            CLIP_DEFAULT_PRECIS,       // nClipPrecision
            CLEARTYPE_NATURAL_QUALITY, // nQuality
            FF_DONTCARE,  // nPitchAndFamily
            FontName);
        if (FontHandle)
        {
            ::DeleteObject(::SelectObject(FontDCHandle, FontHandle));
            ::DeleteObject(FontHandle);
        }
        else
        {
            ::ReleaseDC(WindowHandle, FontDCHandle);
            FontDCHandle = nullptr;
        }
    }

    if (!FontDCHandle)
    {
        return nullptr;
    }

    HDC SymbolFontDCHandle = ::GetDC(WindowHandle);
    if (SymbolFontDCHandle)
    {
        HFONT SymbolFontHandle = ::CreateFontW(
            -FontSize,                  // nHeight
            0,                         // nWidth
            0,                         // nEscapement
            0,                         // nOrientation
            FW_NORMAL,                 // nWeight
            FALSE,                     // bItalic
            FALSE,                     // bUnderline
            0,                         // cStrikeOut
            DEFAULT_CHARSET,           // nCharSet
            OUT_DEFAULT_PRECIS,        // nOutPrecision
            CLIP_DEFAULT_PRECIS,       // nClipPrecision
            CLEARTYPE_NATURAL_QUALITY, // nQuality
            FF_DONTCARE,  // nPitchAndFamily
            ::LvglGetSymbolFontName());
        if (SymbolFontHandle)
        {
            ::DeleteObject(::SelectObject(SymbolFontDCHandle, SymbolFontHandle));
            ::DeleteObject(SymbolFontHandle);
        }
        else
        {
            ::ReleaseDC(WindowHandle, SymbolFontDCHandle);
            SymbolFontDCHandle = nullptr;
        }
    }

    if (!SymbolFontDCHandle)
    {
        return nullptr;
    }

    lv_font_t* font = new lv_font_t();
    if (!font)
    {
        return nullptr;
    }

    lv_font_fmt_win_gdi_dsc_t* dsc = new lv_font_fmt_win_gdi_dsc_t();
    if (!dsc)
    {
        delete font;
        return nullptr;
    }

    dsc->FontDCHandle = FontDCHandle;

    dsc->SymbolFontDCHandle = SymbolFontDCHandle;

    /*if (::GetTextMetricsW(dsc->FontDCHandle, &dsc->TextMetrics))
    {
        font->get_glyph_dsc = ::win_gdi_get_glyph_dsc;
        font->get_glyph_bitmap = ::win_gdi_get_glyph_bitmap;
        font->line_height = static_cast<lv_coord_t>(
            dsc->TextMetrics.tmHeight);
        font->base_line = static_cast<lv_coord_t>(
            dsc->TextMetrics.tmDescent);
        font->subpx = LV_FONT_SUBPX_NONE;
        font->underline_position = 0;
        font->underline_thickness = 0;
        font->dsc = dsc;
    }

    OUTLINETEXTMETRICW OutlineTextMetrics;
    if (::GetOutlineTextMetricsW(
        dsc->FontDCHandle,
        sizeof(OUTLINETEXTMETRICW),
        &OutlineTextMetrics))
    {
        font->line_height = static_cast<lv_coord_t>(
            OutlineTextMetrics.otmLineGap - OutlineTextMetrics.otmDescent + OutlineTextMetrics.otmAscent);
        font->base_line = static_cast<lv_coord_t>(
            -OutlineTextMetrics.otmDescent);
        font->underline_position = static_cast<std::int8_t>(
            OutlineTextMetrics.otmsUnderscorePosition);
        font->underline_thickness = static_cast<std::int8_t>(
            OutlineTextMetrics.otmsUnderscoreSize);
    }*/

    if (::GetOutlineTextMetricsW(
        dsc->FontDCHandle,
        sizeof(OUTLINETEXTMETRICW),
        &dsc->OutlineTextMetrics))
    {
        font->get_glyph_dsc = ::win_gdi_get_glyph_dsc;
        font->get_glyph_bitmap = ::win_gdi_get_glyph_bitmap;
        font->line_height = static_cast<lv_coord_t>(
            dsc->OutlineTextMetrics.otmLineGap
            - dsc->OutlineTextMetrics.otmDescent
            + dsc->OutlineTextMetrics.otmAscent);
        font->base_line = static_cast<lv_coord_t>(
            -dsc->OutlineTextMetrics.otmDescent);
        font->subpx = LV_FONT_SUBPX_NONE;
        font->underline_position = static_cast<std::int8_t>(
            dsc->OutlineTextMetrics.otmsUnderscorePosition);
        font->underline_thickness = static_cast<std::int8_t>(
            dsc->OutlineTextMetrics.otmsUnderscoreSize);
        font->dsc = dsc; 
    }


    return font;
}

static HINSTANCE g_InstanceHandle = nullptr;
static int volatile g_WindowWidth = 0;
static int volatile g_WindowHeight = 0;
static HWND g_WindowHandle = nullptr;
static int volatile g_WindowDPI = USER_DEFAULT_SCREEN_DPI;

static HDC g_BufferDCHandle = nullptr;
static UINT32* g_PixelBuffer = nullptr;
static SIZE_T g_PixelBufferSize = 0;

static lv_disp_t* lv_windows_disp;

static bool volatile g_MousePressed;
static LPARAM volatile g_MouseValue = 0;

static bool volatile g_MouseWheelPressed = false;
static int16_t volatile g_MouseWheelValue = 0;

void win_drv_flush(
    lv_disp_drv_t* disp_drv,
    const lv_area_t* area,
    lv_color_t* color_p)
{
    area;
    color_p;

    if (::lv_disp_flush_is_last(disp_drv))
    {
        HDC hWindowDC = ::GetDC(g_WindowHandle);
        if (hWindowDC)
        {
            ::BitBlt(
                hWindowDC,
                0,
                0,
                g_WindowWidth,
                g_WindowHeight,
                g_BufferDCHandle,
                0,
                0,
                SRCCOPY);

            ::ReleaseDC(g_WindowHandle, hWindowDC);
        }


        
    }

    ::lv_disp_flush_ready(disp_drv);
}

void win_drv_rounder_cb(
    lv_disp_drv_t* disp_drv,
    lv_area_t* area)
{
    area->x1 = 0;
    area->x2 = disp_drv->hor_res - 1;
    area->y1 = 0;
    area->y2 = disp_drv->ver_res - 1;
}

void lv_create_display_driver(
    lv_disp_drv_t* disp_drv,
    int hor_res,
    int ver_res)
{
    ::lv_disp_drv_init(disp_drv);

    HDC hNewBufferDC = ::LvglCreateFrameBuffer(
        g_WindowHandle,
        hor_res,
        ver_res,
        &g_PixelBuffer,
        &g_PixelBufferSize);

    ::DeleteDC(g_BufferDCHandle);
    g_BufferDCHandle = hNewBufferDC;

    lv_disp_buf_t* disp_buf = new lv_disp_buf_t();
    ::lv_disp_buf_init(
        disp_buf,
        g_PixelBuffer,
        nullptr,
        hor_res * ver_res);

    disp_drv->hor_res = static_cast<lv_coord_t>(hor_res);
    disp_drv->ver_res = static_cast<lv_coord_t>(ver_res);
    disp_drv->flush_cb = ::win_drv_flush;
    disp_drv->buffer = disp_buf;
    disp_drv->dpi = g_WindowDPI;
    disp_drv->rounder_cb = win_drv_rounder_cb;
}

bool win_drv_read(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    indev_drv;

    data->state = static_cast<lv_indev_state_t>(
        g_MousePressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->point.x = GET_X_LPARAM(g_MouseValue);
    data->point.y = GET_Y_LPARAM(g_MouseValue);
    return false;
}

std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> key_queue;
std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> char_queue;
std::mutex kb_mutex;

bool win_kb_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    std::lock_guard guard(kb_mutex);

    if (!char_queue.empty())
    {
        auto current = char_queue.front();

        data->key = current.first;
        data->state = current.second;

        char_queue.pop();
    }
    else if (!key_queue.empty())
    {
        auto current = key_queue.front();

        switch (current.first)
        {
        case VK_UP:
            data->key = LV_KEY_UP;
            break;
        case VK_DOWN:
            data->key = LV_KEY_DOWN;
            break;
        case VK_LEFT:
            data->key = LV_KEY_LEFT;
            break;
        case VK_RIGHT:
            data->key = LV_KEY_RIGHT;
            break;
        case VK_ESCAPE:
            data->key = LV_KEY_ESC;
            break;
        case VK_DELETE:
            data->key = LV_KEY_DEL;
            break;
        case VK_BACK:
            data->key = LV_KEY_BACKSPACE;
            break;
        case VK_RETURN:
            data->key = LV_KEY_ENTER;
            break;
        case VK_NEXT:
            data->key = LV_KEY_NEXT;
            break;
        case VK_PRIOR:
            data->key = LV_KEY_PREV;
            break;
        case VK_HOME:
            data->key = LV_KEY_HOME;
            break;
        case VK_END:
            data->key = LV_KEY_END;
            break;
        default:
            data->key = 0;
            break;
        }
        
        data->state = current.second;

        key_queue.pop();
    }

    return false;
}

bool win_mousewheel_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    data->state = static_cast<lv_indev_state_t>(
        g_MouseWheelPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->enc_diff = g_MouseWheelValue;
    g_MouseWheelValue = 0;

    return false;       /*No more data to read so return false*/
}

LRESULT CALLBACK WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
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
        std::lock_guard guard(kb_mutex);

        key_queue.push(
            std::make_pair(
                static_cast<std::uint32_t>(wParam),
                static_cast<lv_indev_state_t>(
                    (uMsg == WM_KEYUP)
                    ? LV_INDEV_STATE_REL
                    : LV_INDEV_STATE_PR)));

        break;
    }
    case WM_CHAR:
    {
        std::lock_guard guard(kb_mutex);

        char_queue.push(std::make_pair(
            static_cast<std::uint32_t>(wParam),
            static_cast<lv_indev_state_t>(LV_INDEV_STATE_PR)));

        break;
    }
    case WM_MOUSEWHEEL:
    {
        g_MouseWheelValue = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
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
;
                lv_disp_buf_t* old_disp_buf = lv_windows_disp->driver.buffer;

                lv_disp_drv_t disp_drv;
                ::lv_create_display_driver(&disp_drv, g_WindowWidth, g_WindowHeight);
                ::lv_disp_drv_update(lv_windows_disp, &disp_drv);
                delete old_disp_buf;
            }
        }
        break;
    }
    case WM_ERASEBKGND:
    {
        ::lv_refr_now(lv_windows_disp);
        return TRUE;
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

bool g_WindowQuitSignal = false;

static void win_msg_handler(lv_task_t* param)
{
    param;

    MSG Message;
    BOOL Result = ::PeekMessageW(&Message, nullptr, 0, 0, TRUE);
    if (Result != 0 && Result != -1)
    {
        ::TranslateMessage(&Message);
        ::DispatchMessageW(&Message);

        if (Message.message == WM_QUIT)
        {
            g_WindowQuitSignal = true;
        }
    }
}

bool win_hal_init(
    _In_ HINSTANCE hInstance,
    _In_ int nShowCmd)
{
    WNDCLASSEXW WindowClass;

    WindowClass.cbSize = sizeof(WNDCLASSEX);

    WindowClass.style = 0;
    WindowClass.lpfnWndProc = ::WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = nullptr;
    WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = nullptr;
    WindowClass.lpszClassName = L"lv_port_windows";
    WindowClass.hIconSm = nullptr;

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

    ::lv_task_create(win_msg_handler, 0, LV_TASK_PRIO_HIGHEST, nullptr);

    ::LvglEnableChildWindowDpiMessage(g_WindowHandle);
    g_WindowDPI = ::LvglGetDpiForWindow(g_WindowHandle);

    lv_disp_drv_t disp_drv;
    ::lv_create_display_driver(&disp_drv, g_WindowWidth, g_WindowHeight);
    lv_windows_disp = ::lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    ::lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ::win_drv_read;
    ::lv_indev_drv_register(&indev_drv);

    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = win_kb_read;
    ::lv_indev_drv_register(&kb_drv);

    lv_indev_drv_t enc_drv;
    lv_indev_drv_init(&enc_drv);
    enc_drv.type = LV_INDEV_TYPE_ENCODER;
    enc_drv.read_cb = win_mousewheel_read;
    ::lv_indev_drv_register(&enc_drv);


    ::LvglLoadSymbolFont();


    wchar_t font_name[] = L""; //L"Segoe UI";

    lv_font_t* font_small = lv_win_gdi_create_font(
        g_WindowHandle,
        12,
        font_name);

    lv_font_t* font_normal = lv_win_gdi_create_font(
        g_WindowHandle,
        16,
        font_name);

    lv_font_t* font_subtitle = lv_win_gdi_create_font(
        g_WindowHandle,
        20,
        font_name);

    lv_font_t* font_title = lv_win_gdi_create_font(
        g_WindowHandle,
        24,
        font_name);

    ::lv_theme_set_act(::lv_theme_material_init(
        ::lv_color_hex(0x01a2b1),
        ::lv_color_hex(0x44d1b6),
        LV_THEME_MATERIAL_FLAG_LIGHT,
        font_small,
        font_normal,
        font_subtitle,
        font_title));

    ::ShowWindow(g_WindowHandle, nShowCmd);
    ::UpdateWindow(g_WindowHandle);
   
    return true;
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    ::lv_init();

    if (!win_hal_init(hInstance, nShowCmd))
    {
        return -1;
    }

    ::lv_demo_widgets();
    //::lv_demo_keypad_encoder();
    //::lv_demo_benchmark();

    /*std::thread([]()
    {
        while (!g_WindowQuitSignal)
        {
            if (!::lv_disp_flush_is_last(&lv_windows_disp->driver))
            {
                continue;
            }

            MONITORINFOEX MonitorInfo;
            MonitorInfo.cbSize = sizeof(MONITORINFOEX);
            if (::GetMonitorInfoW(
                ::MonitorFromWindow(g_WindowHandle, MONITOR_DEFAULTTONEAREST),
                &MonitorInfo))
            {
                HDC hMonitorDC = CreateDCW(
                    nullptr,
                    MonitorInfo.szDevice,
                    nullptr,
                    nullptr);
                if (hMonitorDC)
                {
                    D3DKMT_OPENADAPTERFROMHDC OpenAdapterFromHdc;
                    OpenAdapterFromHdc.hDc = hMonitorDC;
                    if (::D3DKMTOpenAdapterFromHdc(&OpenAdapterFromHdc) == 0)
                    {
                        D3DKMT_WAITFORVERTICALBLANKEVENT WaitForVerticalBlankEvent;
                        WaitForVerticalBlankEvent.hAdapter = OpenAdapterFromHdc.hAdapter;
                        WaitForVerticalBlankEvent.hDevice = 0;
                        WaitForVerticalBlankEvent.VidPnSourceId = OpenAdapterFromHdc.VidPnSourceId;
                        if (::D3DKMTWaitForVerticalBlankEvent(&WaitForVerticalBlankEvent) == 0)
                        {
                            HDC hWindowDC = ::GetDC(g_WindowHandle);
                            if (hWindowDC)
                            {
                                ::BitBlt(
                                    hWindowDC,
                                    0,
                                    0,
                                    g_WindowWidth,
                                    g_WindowHeight,
                                    g_BufferDCHandle,
                                    0,
                                    0,
                                    SRCCOPY);

                                ::ReleaseDC(g_WindowHandle, hWindowDC);
                            }
                        }

                        D3DKMT_CLOSEADAPTER CloseAdapter;
                        CloseAdapter.hAdapter = OpenAdapterFromHdc.hAdapter;
                        ::D3DKMTCloseAdapter(&CloseAdapter);
                    }

                    ::DeleteDC(hMonitorDC);
                }
            }
        }

    }).detach();*/

    /*std::thread([]()
    {
        while (!g_WindowQuitSignal)
        {
            if (!::lv_disp_flush_is_last(&lv_windows_disp->driver))
            {
                continue;
            }

            ::DwmFlush();

            HDC hWindowDC = ::GetDC(g_WindowHandle);
            if (hWindowDC)
            {
                ::BitBlt(
                    hWindowDC,
                    0,
                    0,
                    g_WindowWidth,
                    g_WindowHeight,
                    g_BufferDCHandle,
                    0,
                    0,
                    SRCCOPY);

                ::ReleaseDC(g_WindowHandle, hWindowDC);
            }
        }

    }).detach();*/

    while (!g_WindowQuitSignal)
    {
        ::lv_task_handler();
        ::Sleep(10);
    }

    return 0;
}
