// for custom title bar: https://handmade.network/forums/articles/t/9073-custom_window_title_bar_and_almost_correctly_drawing_windows_10_borders
#include <Windows.h>
#include <stdint.h>

#include "win32/win32_base.h"
#include "base/base_inc.h"

#include "win32/win32_base.c"
#include "base/base_inc.c"
// TODO(S): Seperate win32 and platform code once you've figured it out

typedef uint8_t     U8; 
typedef uint16_t    U16;
typedef uint32_t    U32;
typedef uint64_t    U64;
typedef int8_t      S8; 
typedef int16_t     S16;
typedef int32_t     S32;
typedef int64_t     S64;
typedef S32         B32; 
typedef float       F32;
typedef double      F64;

#define internal static
#define global static
#define true 1 
#define false 0

typedef struct Win32Bitmap Win32Bitmap;
struct Win32Bitmap
{
    BITMAPINFO info;
    void* pixels;
    S32 width;
    S32 height;
    S32 pitch;
    S32 bytes_per_pixel;
};

typedef struct MainState MainState;
struct MainState
{
    Arena* bitmap_arena;

    S32 window_width;
    S32 window_height;
};

global B32 global_running;
global Win32Bitmap global_bitmap;
global MainState global_state;

internal Win32Bitmap 
Win32InitBitmap(S32 width, S32 height)
{
    Win32Bitmap result = {};
    result.width = width;
    result.height = height;
    result.bytes_per_pixel = 4;
    result.pitch = width * result.bytes_per_pixel;

    result.info.bmiHeader.biSize = sizeof(result.info.bmiHeader);
    result.info.bmiHeader.biWidth = width;
    result.info.bmiHeader.biHeight = -height; // top-down 
    result.info.bmiHeader.biPlanes = 1;
    result.info.bmiHeader.biBitCount = 32;
    result.info.bmiHeader.biCompression = BI_RGB;


    result.pixels = PushArray(global_state.bitmap_arena, U8, result.bytes_per_pixel*(width*height));


    return result;
}

internal void 
Win32ResizeBitmap(S32 width, S32 height)
{
    global_bitmap.width = width;
    global_bitmap.height = height;
    global_bitmap.pitch = global_bitmap.bytes_per_pixel * width;
    global_bitmap.info.bmiHeader.biWidth = width;
    global_bitmap.info.bmiHeader.biHeight = -height;

}

internal void
Win32DisplayBufferInWindow(Win32Bitmap* buffer, HDC device_context)
{
    // TODO(S): try halftone see how much slower and better it is
    /*
SetStretchBltMode(DeviceContext, HALFTONE);
SetBrushOrgEx(DeviceContext, 0, 0, 0);
StretchDIBits(DeviceContext,
    0, 0, WindowWidth, WindowHeight,
    0, 0, Buffer.Width, Buffer.Height,
    Buffer.Memory, &Buffer.Info,
    DIB_RGB_COLORS, SRCCOPY);
    */
    StretchDIBits(
        device_context,
        0, 0, global_state.window_width, global_state.window_height,
        0, 0, buffer->width, buffer->height,
        buffer->pixels,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY);
}


LRESULT CALLBACK 
Win32MainWindowCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam) // WPARAM unsigned, LPARAM signed
{
    LRESULT result = 0;
    switch (message)
    {   
        case WM_CLOSE: 
        {
            global_running = false;
            PostQuitMessage(0);   
        } break;
        default:
        {
            result = DefWindowProc(window, message, wparam, lparam);
        } break;
    }

    return result;
}



int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE prev_instance,
                   LPSTR command_line,
                   int show_code)
{
    global_state.window_width = 960;
    global_state.window_height = 640;
    global_state.bitmap_arena = ArenaAlloc();

    WNDCLASSW window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = Win32MainWindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"My IMGUI SONNN";

    RegisterClassW(&window_class);

    HWND window = CreateWindowExW(0,
                                  window_class.lpszClassName,
                                  L"SONN",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  global_state.window_width, global_state.window_height, // window rect, not client rect
                                  0, 0, instance, 0);

    HDC device_ctx = GetDC(window);

    MSG msg;
    global_running = true;
    global_bitmap = Win32InitBitmap(global_state.window_width, global_state.window_height);
    while (global_running)
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            switch (msg.message) {
                
                default: {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                } break;
            }
        }
    
        Win32DisplayBufferInWindow(&global_bitmap, device_ctx);
    }

    ReleaseDC(window, device_ctx);
    return 0;
}