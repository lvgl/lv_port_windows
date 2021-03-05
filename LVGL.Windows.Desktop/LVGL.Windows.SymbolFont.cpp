/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.SymbolFont.cpp
 * PURPOSE:   Implementation for Windows LVGL symbol font utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "LVGL.Windows.SymbolFont.h"

#include <LVGL.Resource.FontAwesome5Free.h>
#include <LVGL.Resource.FontAwesome5FreeLVGL.h>

EXTERN_C HANDLE WINAPI LvglLoadSymbolFont()
{
    DWORD NumFonts = 0;
    return ::AddFontMemResourceEx(
        const_cast<uint8_t*>(LvglFontAwesome5FreeLvglFontResource),
        static_cast<DWORD>(LvglFontAwesome5FreeLvglFontResourceSize),
        nullptr,
        &NumFonts);
}

EXTERN_C LPCWSTR WINAPI LvglGetSymbolFontName()
{
    return LvglFontAwesome5FreeLvglFontName;
}
