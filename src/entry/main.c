// for custom title bar: https://handmade.network/forums/articles/t/9073-custom_window_title_bar_and_almost_correctly_drawing_windows_10_borders

/*
-----------------------------------
Cached software renderer by rxi (from lite editor)

Command buffer, Hash grid, renderer

each frame push draw commands to command buffer
end of frame iterate command buffer and add to hash grid
compare each cell of hash grid with prev to see which have changed to redraw
-----------------------------------
For when you tackle animations (from ryan's substack comments)
What he means by self-correcting exponential animation curves:
"Yup. I just mean that this kind of thing runs every frame:

current = current + (target - current) * rate;

If you work it out mathematically, I *believe* you can use whatever your refresh rate's delta-time value to calculate a rate that produces an identical curve across various displays. But, truthfully, I just never do that - I just multiply by delta_time and tweak a constant rate value for each animation. It's not exactly identical across, say, 144 Hz and 60 Hz, but it's close enough, and being "exact" is just normally not that important when you're talking about simple UI animations (whereas being more precise is much more important in e.g. a game or cutscene).

This produces an exponential curve of motion across frames, which means that the fastest motion is on the first frame of the animation, and the slowest is on the last frame. This fits very naturally with the characteristics you more-or-less always want in a UI, which is as little time as possible between user interaction and perceived effect of user interaction. It's also robust to changes in the *target value* overtime. So, for example, if you have a scrolling offset, you want the user dragging the scroll bar *while the scroll offset is animating* to gracefully adjust the animation to the new target. This simple way of animating things achieves that, so it's a very low-friction way to get high-quality results, for the purposes of UI."

"Here's the closed form curve, when you have a constant rate and target value: https://www.desmos.com/calculator/8f1cpfqlmw

On the graph, t is the target, r is the rate, m is some epsilon that encodes a difference between the target and the current value at which the animation is considered complete, f is the "target refresh rate" in Hz, and z is the x value at which the animation is complete.

If the x axis is the number of frames it takes to complete the animation, then you can notice the slight differences between refresh rates with the z / f expression I put at the bottom. If f = 60, then it takes 411 frames, or 6.85 seconds, to complete. If f = 144, then it takes 911 frames, or 6.88 seconds, to complete. You can see that these are a bit off. I've never done the actual work to figure out the adjustment to the rate that is required to correct for this difference, but like I said, it's just not that big of an issue. I should probably figure it out though nevertheless. :)

EDIT: I figured it out. r should be specified as 1 - 2^(-rate/f). This will ensure that the amount of time it takes to complete the animation remains identical across all refresh rates. It will be a bit slower to compute the rate. Updated graph here: https://www.desmos.com/calculator/xwerlmi4cs"

also this video has visulisations on what he means (going frame rate independent section): https://www.youtube.com/watch?v=LSNQuFEDOyQ 
-----------------------------------

If you are confused about argb vs bgra again:
- // U32 colors are specified as ARGB values but stored in memory as BGRA due to little-endian byte order; structs directly control memory layout, so they must be defined as BGRA.
*/
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define internal static
#define global static
#define true 1 
#define false 0

#include "win32/win32_base.h"
#include "base/base_inc.h"

#include "win32/win32_base.c"
#include "base/base_inc.c"
// TODO(S): Seperate win32 and platform code once you've figured it out


// TODO(S): make hash grid dynamically sized based on screen resolution
#define CELLS_X 80
#define CELLS_Y 50
#define CELL_SIZE 96
#define COMMAND_BUF_SIZE KiB(1)

/* 32bit fnv-1a hash */
#define HASH_INITIAL 2166136261

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


global S32 frames_requested;
global B32 needs_render;

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
global Rng2S32 clip;

typedef struct Command Command;
struct Command 
{
    CommandType type;
    S32 size;       // for iterating over variable length commands
    Rng2S32 rect;
    Vec4U8 color;
}; 

internal void Win32RequestFrame()
{
    frames_requested = 4;
    needs_render = true;
}



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
    //TODO(S): arena popto earlier for decommiting unused large memory, maybe
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

// internal void Win32DisplayBufferInWindow(Win32Bitmap* buffer, HDC device_context)
// {
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

    // Vec2S32 window_rect_size = Dim2S32(global_state.window_rect);

    // StretchDIBits(
//         device_context,
//         0, 0, window_rect_size.x, window_rect_size.y,
//         0, 0, buffer->width, buffer->height,
//         buffer->pixels,
//         &buffer->info,
//         DIB_RGB_COLORS,
//         SRCCOPY);
// }

internal void 
SetClipRect(Rng2S32 rect)
{
    clip = rect;
}

// from rxi
internal Vec4U8 BlendPixel(Vec4U8 dst, Vec4U8 src)
{
    U32 inverse_alpha = 255 - src.w;

    dst.x = (U8)(((src.x * src.w) + (dst.x * inverse_alpha)) >> 8);
    dst.y = (U8)(((src.y * src.w) + (dst.y * inverse_alpha)) >> 8);
    dst.z = (U8)(((src.z * src.w) + (dst.z * inverse_alpha)) >> 8);

    return dst;
}


#define DrawPixel(x, y, bgra)  \
    do {\
        ((Vec4U8* )global_bitmap.pixels)[(x) + (y)*global_bitmap.width] = (bgra); \
    } while (0)



/**
 * @author Zingl Alois
 * @date 22.08.2016
 * @version 1.2
*/
internal void DrawLine(S32 x0, S32 y0, S32 x1, S32 y1, Vec4U8 c)
{
    S32 dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    S32 dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    S32 err = dx+dy, e2;                                  /* error value e_xy */

    Vec4U8 bgra = V4U8(c.z, c.y, c.x, c.w);

    for (;;) {                                                        /* loop */
        DrawPixel(x0,y0,bgra);
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
internal void DrawCircle(S32 xm, S32 ym, S32 r, Vec4U8 c)
{
   S32 x = -r, y = 0, err = 2-2*r; /* II. Quadrant */ 
   Vec4U8 bgra = V4U8(c.z, c.y, c.x, c.w);
   do {
      DrawPixel(xm-x, ym+y, bgra); /*   I. Quadrant */
      DrawPixel(xm-y, ym-x, bgra); /*  II. Quadrant */
      DrawPixel(xm+x, ym-y, bgra); /* III. Quadrant */
      DrawPixel(xm+y, ym+x, bgra); /*  IV. Quadrant */
      r = err;
      if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
      if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
   } while (x < 0);
}


// internal void DrawBlock(Rng2S32 rect, Vec4U8 color)
// {
//     U32 argb = PackARGBFromRGBA(color);
//     for (S32 y = rect.y0; y < rect.y1; y++)
//     {
//         U32* row = (U32* )global_bitmap.pixels + y * global_bitmap.width;
//         for (S32 x = rect.x0; x < rect.x1; x++)
//             row[x] = argb;
//     }
// }
// taken from lite by rxi
internal void DrawBlock(Rng2S32 rect, Vec4U8 c)
{
    // U32 argb = PackARGBFromRGBA(color);
    Vec4U8 bgra = V4U8(c.z, c.y, c.x, c.w);
    Vec2S32 rect_size = Dim2S32(rect);

    Vec4U8* dest =  (Vec4U8* )global_bitmap.pixels;

    // move to top-left of rect
    dest += rect.x0 + rect.y0 * global_bitmap.width;

    // how many pixels to skip after each row to land on next row
    S32 dest_skip = global_bitmap.width - rect_size.x;

    for (S32 y = 0; y < rect_size.y; ++y)
    {
        for (S32 x = 0; x < rect_size.x; ++x)
        {
            *dest++ = bgra;
        }
        dest += dest_skip;
    }
}
#define BgraFromRgba(c) V4U8((c).z, (c).y, (c).x, (c).w);
internal void DrawBlockBlend(Rng2S32 rect, Vec4U8 c)
{
    // Vec4U8 bgra_c = BgraFromRgba(c);
    // Vec2S32 rect_size = Dim2S32(rect);

    // Vec4U8* dest =  (Vec4U8* )global_bitmap.pixels;

    // // move to top-left of rect
    // dest += rect.x0 + rect.y0 * global_bitmap.width;

    // // how many pixels to skip after each row to land on next row
    // S32 dest_skip = global_bitmap.width - rect_size.x;

    // for (S32 y = 0; y < rect_size.y; ++y)
    // {
    //     for (S32 x = 0; x < rect_size.x; ++x)
    //     {
    //         *dest = BlendPixel(*dest, bgra_c);
    //         dest++;
    //     }
    //     dest += dest_skip;
    // }
    Vec4U8 bgra_c = BgraFromRgba(c);
    Vec4U8* base = (Vec4U8*)global_bitmap.pixels;

    for (S32 y = rect.y0; y < rect.y1; ++y)
    {
        Vec4U8* row = base + y * global_bitmap.width;
        for (S32 x = rect.x0; x < rect.x1; ++x)
        {
            row[x] = BlendPixel(row[x], bgra_c);
        }
    }
}

internal void DrawRect(Rng2S32 r, Vec4U8 color)
{   
     r = Intersect2S32(r, clip);
    if (color.w == 255)
        DrawBlock(r, color);
    else
        DrawBlockBlend(r, color);
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
    // and resize bitmap
    Assert(Vec2S32Equal(screen_bounds.max, V2S32(0, 0)));
    Vec2S32 window_size = Dim2S32(global_state.window_rect);
    if (!Vec2S32Equal(screen_bounds.max, window_size))
    {
        Win32ResizeBitmap(window_size.x, window_size.y);
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


internal void PushDirtyRect(Rng2S32 r, S32* count)
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
    Rng2S32 cr = screen_bounds;
    while (NextCommand(&cmd))
    {   
        // 1.
        if (cmd->type == SET_CLIP) { cr = cmd->rect; }
        Rng2S32 r = Intersect2S32(cmd->rect, cr);
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
                PushDirtyRect((Rng2S32){x, y, x+1, y+1}, &rect_count);
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
        Rng2S32 r = dirty_rect_buf[i]; // clip rect with dirty rect to only draw in dirty region
        SetClipRect(r);

        cmd = NULL;
        while (NextCommand(&cmd))
        {
            switch(cmd->type)
            {
                case DRAW_RECT:
                    DrawRect(cmd->rect, cmd->color);
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

internal void RencacheDrawRect(Rng2S32 rect, Vec4U8 color)
{
    if (!Overlap2S32(screen_bounds, rect)) {return;}
    Command* cmd = PushCommand(DRAW_RECT, sizeof(Command));
    if (cmd)
    {
        cmd->rect = rect;
        cmd->color = color;
    }
}

#define ColorHex(hex) Vec4U8FromU32((hex))
#define ColorRGBA(r,g,b,a) V4U8((r), (g), (b), (a))

internal void Render(HDC device_ctx)
{
    RencacheBeginFrame();
    Vec2S32 window_size = Dim2S32(global_state.window_rect);
    RencacheDrawRect((Rng2S32){0, 0, global_bitmap.width, global_bitmap.height}, ColorRGBA(255, 255, 255, 255));
    // RencacheDrawRect((Rng2S32){0, 0, global_bitmap.width, global_bitmap.height}, ColorRGBA(0, 255, 0, 75));
    RencacheDrawRect((Rng2S32){0, 0, (window_size.x/2)+20, window_size.y/2}, ColorHex(0xff00007f));
    RencacheDrawRect((Rng2S32){window_size.x/2, 0, window_size.x, (window_size.y/2)+20}, ColorHex(0x00ff007f));
    RencacheDrawRect((Rng2S32){(window_size.x/2)-20, window_size.y/2, window_size.x, window_size.y}, ColorHex(0xfff0007f));
    RencacheDrawRect((Rng2S32){0, (window_size.y/2)-20, window_size.x/2, window_size.y}, ColorHex(0x0000ff7f));

    RencacheEndFrame(); 
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

        case WM_SIZE:
        {
            S32 width = LOWORD(lparam);
            S32 height = HIWORD(lparam);

            //-S: Changing window_rect causes RencacheInvalidate on RencacheBeginFrame
            global_state.window_rect.max.x = width;
            global_state.window_rect.max.y = height;
            
            // InvalidateRect(window, NULL, FALSE);
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint = {};
            HDC dc = BeginPaint(window, &paint);
            Render(dc);
            EndPaint(window, &paint);   
            // ValidateRect(window, NULL);
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


    global_running = true;
    while (global_running)
    {
        MSG msg;
        B32 wait_for_event = (frames_requested == 0);

        if (!GetMessageW(&msg, 0, 0, 0))
          {
              global_running = false;
              break;
          }
          TranslateMessage(&msg);
          DispatchMessageW(&msg);

       //  if (wait_for_event)
       //  {
       //    // nothing animating -> block until an event arrives. ~0% CPU.
       //    if (!GetMessageW(&msg, 0, 0, 0))
       //    {
       //        global_running = false;
       //        break;
       //    }
       //    TranslateMessage(&msg);
       //    DispatchMessageW(&msg);

       //    // drain any other messages that piled up
       //    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
       //    {
       //        TranslateMessage(&msg);
       //        DispatchMessageW(&msg);
       //    }
       //  }
       //  else
       //  {
       //    // still animating -> don't block, just drain whatever's there
       //    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
       //    {
       //        TranslateMessage(&msg);
       //        DispatchMessageW(&msg);
       //    }
       //  }


       //  // HDC device_ctx = GetDC(global_state.window);
       //  // Update(device_ctx);
       //  // ReleaseDC(global_state.window, device_ctx);

       // if (frames_requested > 0)
       // {
       //      --frames_requested;
       // }
    }
    
    return 0;
}

