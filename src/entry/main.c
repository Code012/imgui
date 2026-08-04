// for custom title bar: https://handmade.network/forums/articles/t/9073-custom_window_title_bar_and_almost_correctly_drawing_windows_10_borders

/*
Cached software renderer by rxi (from lite editor)

Command buffer, Hash grid, renderer

each frame push draw commands to command buffer
end of frame iterate command buffer and add to hash grid
compare each cell of hash grid with prev to see which have changed to redraw

*/
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "win32/win32_base.h"
#include "base/base_inc.h"

#include "win32/win32_base.c"
#include "base/base_inc.c"
// TODO(S): Seperate win32 and platform code once you've figured it out

#define internal static
#define global static
#define true 1 
#define false 0

// TODO(S): make hash grid dynamically sized based on screen resolution
#define CELLS_X 80
#define CELLS_Y 50
#define CELL_SIZE 96
#define COMMAND_BUF_SIZE KiB(1)

/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

#define COLOR(r,g,b) \
    ( 0xFF000000u \
    | ((U32)((r)*255.0f) << 16) \
    | ((U32)((g)*255.0f) << 8)  \
    |  (U32)((b)*255.0f) )

#define COLORA(r,g,b,a) \
    ( ((U32)((a)*0.01f) << 24) \
    | ((U32)((r)*255.0f) << 16) \
    | ((U32)((g)*255.0f) << 8)  \
    |  (U32)((b)*255.0f) )

typedef enum CommandType
{
    SET_CLIP,
    DRAW_RECT
}
CommandType;

typedef struct Win32Bitmap Win32Bitmap;
struct Win32Bitmap
{
    // Note(S): Memory order BGRA
    BITMAPINFO info;
    void* pixels;
    S32 biggest_size;
    S32 width;
    S32 height;
    S32 pitch;
    S32 bytes_per_pixel;
};

typedef struct MainState MainState;
struct MainState
{
    Arena* bitmap_arena;

    // S32 window_width;
    // S32 window_height;
    HWND window;
    Rng2S32 window_rect;
};

global B32 show_debug;

global B32 global_running;
global Win32Bitmap global_bitmap;
global MainState global_state;


global U32 cells_buf1[CELLS_X * CELLS_Y];
global U32 cells_buf2[CELLS_X * CELLS_Y];
global U32* cells_prev = cells_buf1;
global U32* cells = cells_buf2;
global Rng2S32 dirty_rect_buf[CELLS_X * CELLS_Y / 2];
global U8 command_buf[COMMAND_BUF_SIZE];    // holds variable length commands
global S32 command_buf_offset;

global Rng2S32 screen_bounds;

typedef struct Command Command;
struct Command 
{
    CommandType type;
    S32 size;       // for iterating over variable length commands
    Rng2S32 rect;
    U32 color;
}; 

internal Win32Bitmap Win32InitBitmap(S32 width, S32 height)
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


    S32 size = result.bytes_per_pixel*(width*height);
    result.pixels = PushArray(global_state.bitmap_arena, U8, size);


    return result;
}

internal void Win32ResizeBitmap(S32 width, S32 height)
{
    //TODO(S): arenapopto earlier for decommiting unused large memory, maybe
    if (width > global_bitmap.width || height > global_bitmap.height)
    {   
        S32 size = global_bitmap.bytes_per_pixel*(width*height);
        if (size > global_bitmap.biggest_size)
        {
            PushArray(global_state.bitmap_arena, U8, size-global_bitmap.biggest_size);
            global_bitmap.biggest_size = size;
        }
    }

    global_bitmap.width = width;
    global_bitmap.height = height;
    global_bitmap.pitch = global_bitmap.bytes_per_pixel * width;

    global_bitmap.info.bmiHeader.biWidth = width;
    global_bitmap.info.bmiHeader.biHeight = -height;

}

internal void Win32DisplayBufferInWindow(Win32Bitmap* buffer, HDC device_context)
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

    Vec2S32 window_rect_size = Dim2S32(global_state.window_rect);

    StretchDIBits(
        device_context,
        0, 0, window_rect_size.x, window_rect_size.y,
        0, 0, buffer->width, buffer->height,
        buffer->pixels,
        &buffer->info,
        DIB_RGB_COLORS,
        SRCCOPY);
}


#define DrawPixel(x, y, color)  \
    do {\
        ((U32* )global_bitmap.pixels)[(x) + (y)*global_bitmap.width] = (color); \
    } while (0)

/**
 * @author Zingl Alois
 * @date 22.08.2016
 * @version 1.2
*/
internal void DrawLine(S32 x0, S32 y0, S32 x1, S32 y1, U32 color)
{
    S32 dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    S32 dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    S32 err = dx+dy, e2;                                  /* error value e_xy */

    for (;;) {                                                        /* loop */
        DrawPixel(x0,y0,color);
        e2 = 2*err;
        if (e2 >= dy) {                                       /* e_xy+e_x > 0 */
            if (x0 == x1) break;
            err += dy; x0 += sx;
        }
        if (e2 <= dx) {                                       /* e_xy+e_y < 0 */
            if (y0 == y1) break;
            err += dx; y0 += sy;
        }
    }
}
/**
 * @author Zingl Alois
 * @date 22.08.2016
 * @version 1.2
*/
internal void DrawCircle(S32 xm, S32 ym, S32 r, U32 color)
{
   S32 x = -r, y = 0, err = 2-2*r; /* II. Quadrant */ 
   do {
      DrawPixel(xm-x, ym+y, color); /*   I. Quadrant */
      DrawPixel(xm-y, ym-x, color); /*  II. Quadrant */
      DrawPixel(xm+x, ym-y, color); /* III. Quadrant */
      DrawPixel(xm+y, ym+x, color); /*  IV. Quadrant */
      r = err;
      if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
      if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
   } while (x < 0);
}


internal void DrawBlock(Rng2S32 rect, U32 color)
{
    for (S32 y = rect.y0; y < rect.y1; y++)
    {
        U32* row = (U32* )global_bitmap.pixels + y * global_bitmap.width;
        for (S32 x = rect.x0; x < rect.x1; x++)
            row[x] = color;
    }
}

internal void DrawRect(Rng2S32 r, U32 border_color, U32 main_color)
{
    // Top-border
    DrawBlock((Rng2S32){r.x0, r.y0,   r.x1,   r.y0+1}, border_color);
    // Left-border
    DrawBlock((Rng2S32){r.x0, r.y0+1, r.x0+1, r.y1-1}, border_color);
    // Right-border
    DrawBlock((Rng2S32){r.x1-1, r.y0+1, r.x1, r.y1-1}, border_color);
    // Bottom-border
    DrawBlock((Rng2S32){r.x0, r.y1-1, r.x1, r.y1}, border_color);
    // Inner rect
    DrawBlock((Rng2S32){r.x0+1, r.y0+1, r.x1-1, r.y1-1}, main_color);

}

// FNV-1a
// order sensitive, very important
internal void Hash(U32* h, void* data, S32 size)
{
    U8* p = data;
    while (size--)
        *h = (*h ^ *p++) * 16777619;
}

internal Command* PushCommand(S32 type, S32 size)
{
    Command* cmd = (Command* )(command_buf + command_buf_offset);
    S32 n = command_buf_offset + size;
    if (n > COMMAND_BUF_SIZE)
    {
        fprintf(stderr, "Warning: ("__FILE__"): exhausted command buffer\n");
        return NULL;
    }  
    command_buf_offset = n;
    MemoryZeroStruct(cmd);
    cmd->type = type;
    cmd->size = size;
    return cmd;
}

internal void RencacheInvalidate()
{
    MemorySet(cells_prev, 0xFF, sizeof(cells_buf1));
}

internal void RencacheBeginFrame()
{
    //- S: Reset all cells if the screen width/height changed
    Assert(Vec2S32Equal(screen_bounds.max, V2S32(0, 0)));
    Vec2S32 window_size = Dim2S32(global_state.window_rect);
    if (!Vec2S32Equal(screen_bounds.max, window_size))
    {
        screen_bounds.max = window_size;
        RencacheInvalidate();
    }
}

internal B32 NextCommand(Command** prev)
{   
    //-S: First command
    if (*prev == NULL)
    {
        *prev = (Command* )command_buf;
    } 
    else
    {   //-S: Next command
        *prev = (Command* ) (((char* ) *prev) + (*prev)->size);
    }
    // are we opl?
    return *prev != ((Command* ) (command_buf + command_buf_offset));
}


internal void UpdateOverlappingCells(Rng2S32 r, U32 h)
{
    S32 x0 = r.min.x / CELL_SIZE;
    S32 y0 = r.min.y / CELL_SIZE;
    S32 x1 = r.max.x / CELL_SIZE;
    S32 y1 = r.max.y / CELL_SIZE;

    for (S32 y = y0; y <= y1; ++y)   // worst case you draw one extra cell
    {
        for (S32 x = x0; x <= x1; ++x)
        {
            Hash(&cells[x + y*CELLS_X], &h, sizeof(h));
        }
    }
}


internal void PushRect(Rng2S32 r, S32* count)
{
    // try to merge with existing rectangle
    for (S32 i = *count - 1; i >= 0; --i)
    {
        Rng2S32 *dr = &dirty_rect_buf[i];
        if (Overlap2S32(*dr, r))
        {
            *dr = Union2S32(*dr, r);
            return;
        }
    }

    // couldn't merge with previous rectangle: push
    dirty_rect_buf[(*count)++] = r;
}


internal void RenUpdateRects(Rng2S32 *rects, S32 count)
{
    HDC dc = GetDC(global_state.window);
    for (S32 i = 0; i < count; ++i)
    {
        Rng2S32 r = rects[i];

        StretchDIBits(
            dc,
            r.x0, r.y0, r.x1-r.x0, r.y1-r.y0, // dest (window)
            r.x0, r.y0, r.x1-r.x0, r.y1-r.y0, // src (bitmap) its the same coords so no scaling
            global_bitmap.pixels,
            &global_bitmap.info,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
    ReleaseDC(global_state.window, dc);
}

internal void RencacheEndFrame()
{
    //- S: Update cells from commands
    // 1. clipping
    // 2. Iterate command buffer. For each cmd, hash the cmd.
    // For each hash value for the cells in the hash grid, which 
    // the cmd's rect overlaps with. Would then be updated with 
    // the cmd's hash value.
    Command* cmd = NULL;
    Rng2S32 clip = screen_bounds;
    while (NextCommand(&cmd))
    {   
        // 1.
        if (cmd->type == SET_CLIP) { clip = cmd->rect; }
        Rng2S32 r = Intersect2S32(cmd->rect, clip);
        Vec2S32 r_size = Dim2S32(r);
        if (r_size.x == 0 || r_size.y == 0) { continue; }
        // 2.
        U32 h = HASH_INITIAL;
        Hash(&h, cmd, cmd->size);
        UpdateOverlappingCells(r, h);
    }

    //- S: Push rects for all cells changed from last frame (push dirty rects), 
    // and reset cells
    S32 rect_count = 0;
    Vec2S32 screen_size = Dim2S32(screen_bounds);
    S32 max_x = screen_size.x / CELL_SIZE + 1;
    S32 max_y = screen_size.y / CELL_SIZE + 1;
    for (S32 y = 0; y < max_y; ++y)
    {
        for (S32 x = 0; x < max_x; ++x)
        {
            // compare previous and current cell for change
            S32 idx = x + y * CELLS_X;
            if (cells[idx] != cells_prev[idx])
            {
                PushRect((Rng2S32){x, y, x+1, y+1}, &rect_count);
            }
            cells_prev[idx] = HASH_INITIAL; // reset cells
        }
    }

    // expand rects from cels to pixels, cell-space to screen-space
    for (S32 i = 0; i < rect_count; ++i)
    {
        Rng2S32 r = dirty_rect_buf[i];
        Rng2S32 scaled;
        scaled.p0 = Scale2S32(r.p0, CELL_SIZE);
        scaled.p1 = Scale2S32(r.p1, CELL_SIZE);
        dirty_rect_buf[i] = Intersect2S32(scaled, screen_bounds);  // clip with screen
    }

    // redraw updated regions
    for (S32 i = 0; i < rect_count; ++i)
    {
        // draw
        // Rng2S32 r = dirty_rect_buf[i]; // for debug view when we want to see which cells were dirty

        cmd = NULL;
        while (NextCommand(&cmd))
        {
            switch(cmd->type)
            {
                case DRAW_RECT:
                    DrawRect(cmd->rect, cmd->color, cmd->color);
                    break;
            }
        }


        // if (show_debug)
            // DrawRect(r, COLORA(rand(), rand(), rand(), 50), COLORA(rand(), rand(), rand(), 50));
    }


    // update dirty rects
    if (rect_count > 0)
        RenUpdateRects(dirty_rect_buf, rect_count);

    // swap cell buffer and reset
    U32* temp = cells;
    cells = cells_prev;
    cells_prev = temp;
    command_buf_offset = 0;
}

internal void RencacheDrawRect(Rng2S32 rect, U32 color)
{
    if (!Overlap2S32(screen_bounds, rect)) {return;}
    Command* cmd = PushCommand(DRAW_RECT, sizeof(Command));
    if (cmd)
    {
        cmd->rect = rect;
        cmd->color = color;
    }
}

internal void Render(HDC device_ctx)
{
    RencacheBeginFrame();
    RencacheDrawRect((Rng2S32){0, 0, global_bitmap.width, global_bitmap.height}, COLOR(0.1f, 0.4f, 1.0f));
    RencacheDrawRect((Rng2S32){0, 0, 100, 100}, COLOR(0.1f, 0.0f, 0.0f));

    RencacheEndFrame(); 
    // Rng2S32 rect = {0, 0, global_bitmap.width, global_bitmap.height};
    // DrawRect(rect, COLOR(0.1f, 0.0f, 0.0f), COLOR(1.0f, 0.0f, 0.0f));
    // Win32DisplayBufferInWindow(&global_bitmap, device_ctx);
}

// TODO(S): handle wm_size and wm_paint
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

        case WM_SIZE:
        {
            S32 width = LOWORD(lparam);
            S32 height = HIWORD(lparam);

            global_state.window_rect.max.x = width;
            global_state.window_rect.max.y = height;
            Win32ResizeBitmap(width, height);
            RencacheInvalidate();
            // HDC dc = GetDC(window);
            // Render(dc);
            // ReleaseDC(window, dc);
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC dc = BeginPaint(window, &paint);

            Render(dc);

            EndPaint(window, &paint);   
        } break;
        default:
        {
            result = DefWindowProc(window, message, wparam, lparam);
        } break;
    }

    return result;
}

int WINAPI 
WinMain(HINSTANCE instance,
                   HINSTANCE prev_instance,
                   LPSTR command_line,
                   int show_code)
{
    global_state.window_rect.max = V2S32(960, 640);
    global_state.bitmap_arena = ArenaAlloc();
    Vec2S32 window_size = Dim2S32(global_state.window_rect);
    global_bitmap = Win32InitBitmap(window_size.x, window_size.y);

    WNDCLASSW window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = Win32MainWindowCallback;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"My IMGUI SONNN";

    RegisterClassW(&window_class);

    global_state.window = CreateWindowExW(0,
                                  window_class.lpszClassName,
                                  L"SONN",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  window_size.x, window_size.y, // window rect, not client rect
                                  0, 0, instance, 0);

    HDC device_ctx = GetDC(global_state.window);

    MSG msg;
    global_running = true;
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
        Render(device_ctx);

        


        // MemoryZero(global_bitmap.pixels, global_bitmap.size);
    }

    ReleaseDC(global_state.window, device_ctx);
    return 0;
}