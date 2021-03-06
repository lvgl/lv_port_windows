/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.Font.h
 * PURPOSE:   Definition for Windows LVGL font utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_WINDOWS_FONT
#define LVGL_WINDOWS_FONT

#include <Windows.h>

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

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#else
#define EXTERN_C       extern
#endif
#endif // !EXTERN_C

/**
 * @brief Initialize the Windows GDI font engine.
*/
EXTERN_C void WINAPI LvglWindowsGdiFontInitialize();

/**
 * @brief Creates a LVGL font object.
 * @param FontSize The font size.
 * @param FontName The font name.
 * @return The LVGL font object.
*/
EXTERN_C lv_font_t* WINAPI LvglWindowsGdiFontCreateFont(
    _In_ int FontSize,
    _In_opt_ LPCWSTR FontName);

#endif // !LVGL_WINDOWS_SYMBOL_FONT
