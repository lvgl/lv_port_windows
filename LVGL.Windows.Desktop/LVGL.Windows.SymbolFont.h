/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.SymbolFont.h
 * PURPOSE:   Definition for Windows LVGL symbol font utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_WINDOWS_SYMBOL_FONT
#define LVGL_WINDOWS_SYMBOL_FONT

#include <Windows.h>

/**
 * @brief Loads the LVGL symbol font to the system.
 * @return If the function succeeds, the return value specifies the handle to
 *         the font added. This handle uniquely identifies the fonts that were
 *         installed on the system. If the function fails, the return value is
 *         zero. No extended error information is available.
 * @remark A font that is added by LvglLoadSymbolFont is always private to the
 *         process that made the call and is not enumerable. To remove the
 *         fonts that were installed, call RemoveFontMemResourceEx. However,
 *         when the process goes away, the system will unload the fonts even if
 *         the process did not call RemoveFontMemResourceEx.
*/
EXTERN_C HANDLE WINAPI LvglLoadSymbolFont();

/**
 * @brief Gets the name of LVGL symbol font.
 * @return The string of the name of LVGL symbol font.
*/
EXTERN_C LPCWSTR WINAPI LvglGetSymbolFontName();

#endif // !LVGL_WINDOWS_SYMBOL_FONT
