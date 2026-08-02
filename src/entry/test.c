/*  date = July 31st 2026 06:47 PM  *//*  date = July 28th 2026 06:27 PM  */

/*
Boxs that consume input events first must be rendered last (on top).

The structure of a frame:
1. Build box heirarchy (one pass)
2. Render (separate pass)

To fit offline aytolayout algorithm, accept single frame of delay for rectangles for consumption of events, 
while preserving no frame delar for final rendering of frame.
Update frame structure:
1. Build box heirarchy (use last frame's data)
2. Autolayout pass (produce fresh layout data)
3. Render (use up-to-date layout data)

*/
#if 0

#include <Windows.h>
#include <stdint.h>

typedef uint8_t  	U8; 
typedef uint16_t 	U16;
typedef uint32_t 	U32;
typedef uint64_t 	U64;
typedef int8_t  	S8; 
typedef int16_t 	S16;
typedef int32_t 	S32;
typedef int64_t 	S64;
typedef S32     	B32; 
typedef float   	F32;
typedef double  	F64;

//////////////////////////////////////////////
//- UI 
typedef struct Rng2F322 Rng2F322;
struct Rng2F322
{
	F32 x;
	F32 y;
};


typedef enum UI_SizeKind
{
	UI_SizeKind_Null,
	UI_SizeKind_Pixels,			// size is computed via a preferred pixel value
 	UI_SizeKind_TextContent, 	// size is computed via the dimensions of box's rendered string
    UI_SizeKind_ParentPct,   	// size is computed via a well-determined parent or grandparent size
    UI_SizeKind_ChildrenSum, 	// size is computed via summing well-determined sizes of children
}
UI_SizeKind;

typedef struct UI_Size UI_Size;
struct UI_Size
{
	UI_SizeKind kind;
	F32 value;
	F32 strictness;		// percentage of final size you refuse to give up
};

typedef enum Axis2
{
	Axis2_Invalid = -1,
	Axis2_X,
	Axis2_Y,
	Axis2_COUNT,
}
Axis2;

typedef U32 UI_BoxFlags;
enum
{
	UI_BoxFlag_Clickable       = (1<<0),
 	UI_BoxFlag_ViewScroll      = (1<<1),
 	UI_BoxFlag_DrawText        = (1<<2),
 	UI_BoxFlag_DrawBorder      = (1<<3),
 	UI_BoxFlag_DrawBackground  = (1<<4),
 	UI_BoxFlag_DrawDropShadow  = (1<<5),
 	UI_BoxFlag_Clip            = (1<<6),
 	UI_BoxFlag_HotAnimation    = (1<<7),
 	UI_BoxFlag_ActiveAnimation = (1<<8),
};

// input: semantic_size
// output: computed position, size and final rectangle of autolayout alg
// On every frame the tree links will be written from scratch for the entire heirarchy.
// The hash links are used to look up the persistent part of the structure every frame.
typedef struct UI_Box UI_Box;
struct UI_Box
{
	// tree linkds
	UI_Box* first;
	UI_Box* last;
	UI_Box* next;
	UI_Box* prev;
	UI_Box* parent;

	// hash links
	UI_Box* hash_next;
	UI_Box* hash_prev;

	// key+generation info
	UI_Key key;

	// per-frame info provided by builders
	UI_BoxFlags flags;
	String8 string;
	UI_Size semantic_size[Axis2_COUNT];

	// computed every frame
	UI_Size semantic_size[Axis2_COUNT];	
	F32 computed_rel_position[Axis2_COUNT];		// computed position relative to parent position
	F32 computed_size[Axis2_COUNT];				// in pixels
	Rng2F322 rect;								// final onscreen rect coords produced

	// persistent data
	F32 hot_transition;
	F32 active_transition;

};

// (interaction requires the keying mechanism
typedef struct UI_Signal UI_Signal;
struct UI_Signal
{
	UI_Box* box;
	Vec2F32 mouse;
	Vec2F32 drag_delta;
	B8 blicked;
	B8 double_clicked;
	B8 right_clicked;
	B8 pressed;
	B8 released;
	B8 dragging;
	B8 hovering;
};

// basic key type helpers
UI_Key UI_KeyNull(void);
UI_Key UI_KeyFromString(String8 string);
B32 UI_KeyMatch(UI_Key a, UI_Key b);

// construct a box, looking up from the cache if
// possible, and pushing it as a new child of the
// active parent.
UI_Box *UI_BoxMake(UI_BoxFlags flags, String8 string);
UI_Box *UI_BoxMakeF(UI_BoxFlags flags, char *fmt, ...);

// some other possible building parameterizations
void UI_BoxEquipDisplayString(UI_Box *box,
                                 String8 string);
void UI_BoxEquipChildLayoutAxis(UI_Box *box,
                                   Axis2 axis);

// managing the parent stack
UI_Box *UI_PushParent(UI_Box *box);
UI_Box *UI_PopParent(void);

// implements all interactions for all boxs - 
// "get the user signal from this box"
UI_Signal UI_SignalFromBox(UI_Box* box);
/////////////////////////////////////////////


typedef struct Rect Rect;
struct Rect
{
	S32 l, r, t, b;
};

typedef struct Window Window;
struct Window
{
	Rect clip;
	U32* bits;
	S32 width, height;

};

int main()
{



	return 1;
}

#endif


// ============================================================================
// ui.c — Software-rendered UI, Week 1 + Week 2
//
// WEEK 1: window + raw pixel backbuffer, presented every frame via
//         StretchDIBits. No GDI drawing primitives used — we own every pixel.
// WEEK 2: a tiny draw list (push_rect) + a CPU rasterizer that alpha-blends
//         solid rectangles into the backbuffer. This is the CPU equivalent
//         of Ryan Fleury's "Pass I" (solid color rects), just as an explicit
//         nested loop instead of a GPU pixel shader.
//
// BUILD (MSVC, from a "Developer Command Prompt"):
//   cl ui.c user32.lib gdi32.lib /Fe:ui.exe
//
// BUILD (mingw-w64):
//   gcc ui.c -o ui.exe -luser32 -lgdi32
//
// Run ui.exe. You should see a window with a background color and a few
// overlapping rectangles, one of them semi-transparent, proving the alpha
// blend is working. Resize the window — the backbuffer should resize with
// it (check WM_SIZE handling below).
// ============================================================================

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef int32_t  i32;
typedef float    f32;

// ----------------------------------------------------------------------------
// WEEK 1: Backbuffer — a plain array of pixels we control entirely ourselves.
// ----------------------------------------------------------------------------

typedef struct {
    BITMAPINFO info;
    u32 *pixels;   // one u32 per pixel, packed as 0x00RRGGBB
    i32  width;
    i32  height;
} Backbuffer;

static Backbuffer g_bb;
static HWND       g_hwnd;
static bool       g_running = true;

static void backbuffer_resize(Backbuffer *bb, i32 w, i32 h) {
    if (w <= 0 || h <= 0) return;

    if (bb->pixels) {
        free(bb->pixels);
    }
    bb->width  = w;
    bb->height = h;
    bb->pixels = (u32 *)malloc(sizeof(u32) * (size_t)w * (size_t)h);

    ZeroMemory(&bb->info, sizeof(bb->info));
    bb->info.bmiHeader.biSize        = sizeof(bb->info.bmiHeader);
    bb->info.bmiHeader.biWidth       = w;
    bb->info.bmiHeader.biHeight      = -h; // negative => top-down DIB (row 0 = top)
    bb->info.bmiHeader.biPlanes      = 1;
    bb->info.bmiHeader.biBitCount    = 32;
    bb->info.bmiHeader.biCompression = BI_RGB;
}

// Present is the one place we still touch a GDI call — StretchDIBits just
// blits our own pixel buffer onto the window's device context. We are not
// using any GDI drawing primitives (Rectangle, FillRect, TextOut, etc.).
static void backbuffer_present(Backbuffer *bb, HDC dc, i32 window_w, i32 window_h) {
    StretchDIBits(dc,
        0, 0, window_w, window_h,
        0, 0, bb->width, bb->height,
        bb->pixels, &bb->info,
        DIB_RGB_COLORS, SRCCOPY);
}

// ----------------------------------------------------------------------------
// WEEK 2: Draw list + rasterizer
//
// Instead of drawing immediately, app code "pushes" rectangles into a list.
// Once per frame, we iterate that list and rasterize each rect by hand —
// this is the CPU stand-in for a GPU draw call. Keeping this separation now
// (push commands -> execute commands) is exactly the shape rxi's cached
// renderer wants later: we're just not caching/diffing yet.
// ----------------------------------------------------------------------------

typedef struct {
    i32 x0, y0, x1, y1;
    u8  r, g, b, a;
} RectCmd;

#define MAX_COMMANDS 4096
static RectCmd g_cmds[MAX_COMMANDS];
static i32     g_cmd_count = 0;

static void push_rect(i32 x0, i32 y0, i32 x1, i32 y1, u8 r, u8 g, u8 b, u8 a) {
    if (g_cmd_count >= MAX_COMMANDS) return; // silently drop; fine for now
    RectCmd *c = &g_cmds[g_cmd_count++];
    c->x0 = x0; c->y0 = y0; c->x1 = x1; c->y1 = y1;
    c->r = r; c->g = g; c->b = b; c->a = a;
}

// Standard "over" alpha compositing against an opaque destination.
// This is the CPU version of GPU blend state / the final color write.
static inline void blend_pixel(Backbuffer *bb, i32 x, i32 y, u8 r, u8 g, u8 b, u8 a) {
    if ((u32)x >= (u32)bb->width || (u32)y >= (u32)bb->height) return; // bounds check

    u32 *p = &bb->pixels[y * bb->width + x];

    if (a == 255) {
        *p = ((u32)r << 16) | ((u32)g << 8) | (u32)b;
        return;
    }
    if (a == 0) return;

    u32 dst = *p;
    u8 dr = (u8)((dst >> 16) & 0xFF);
    u8 dg = (u8)((dst >> 8)  & 0xFF);
    u8 db = (u8)( dst        & 0xFF);

    f32 sa = (f32)a / 255.0f;
    u8 or_ = (u8)((f32)r * sa + (f32)dr * (1.0f - sa));
    u8 og  = (u8)((f32)g * sa + (f32)dg * (1.0f - sa));
    u8 ob  = (u8)((f32)b * sa + (f32)db * (1.0f - sa));

    *p = ((u32)or_ << 16) | ((u32)og << 8) | (u32)ob;
}

// The "unified draw function" -- every rect command funnels through here.
// Right now it only handles solid color, but this is the function that
// Week 3 (rounded corners / soft edges / borders / gradients) will grow.
static void rasterize_rect(Backbuffer *bb, RectCmd *c) {
    i32 x0 = c->x0 < 0 ? 0 : c->x0;
    i32 y0 = c->y0 < 0 ? 0 : c->y0;
    i32 x1 = c->x1 > bb->width  ? bb->width  : c->x1;
    i32 y1 = c->y1 > bb->height ? bb->height : c->y1;

    for (i32 y = y0; y < y1; y++) {
        for (i32 x = x0; x < x1; x++) {
            blend_pixel(bb, x, y, c->r, c->g, c->b, c->a);
        }
    }
}

static void execute_commands(Backbuffer *bb) {
    for (i32 i = 0; i < g_cmd_count; i++) {
        rasterize_rect(bb, &g_cmds[i]);
    }
    g_cmd_count = 0; // reset for next frame (full redraw every frame, for now)
}

static void clear_backbuffer(Backbuffer *bb, u8 r, u8 g, u8 b) {
    u32 color = ((u32)r << 16) | ((u32)g << 8) | (u32)b;
    i32 count = bb->width * bb->height;
    for (i32 i = 0; i < count; i++) {
        bb->pixels[i] = color;
    }
}

// ----------------------------------------------------------------------------
// Frame: builds the draw list, then executes it. This function is the thing
// that grows every future week -- for now it just proves rects + blending.
// ----------------------------------------------------------------------------

static void render_frame(Backbuffer *bb) {
    clear_backbuffer(bb, 30, 30, 34); // dark background

    // Opaque red square
    push_rect(50, 50, 250, 250, 220, 60, 60, 255);

    // Opaque green square, overlapping the red one
    push_rect(150, 120, 350, 320, 60, 200, 90, 255);

    // Semi-transparent blue square on top of both -- if blending is
    // correct you should see it tint whatever's beneath it, not just
    // stomp over it.
    push_rect(220, 90, 420, 290, 60, 90, 220, 140);

    execute_commands(bb);
}

// ----------------------------------------------------------------------------
// WEEK 1: window plumbing
// ----------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_SIZE: {
            RECT r;
            GetClientRect(hwnd, &r);
            backbuffer_resize(&g_bb, r.right - r.left, r.bottom - r.top);
            return 0;
        }
        case WM_CLOSE:
        case WM_DESTROY:
            g_running = false;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;

    WNDCLASSA wc = {0};
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = "SoftwareUIWindowClass";
    RegisterClassA(&wc);

    g_hwnd = CreateWindowExA(
        0, wc.lpszClassName, "Software UI - Week 1/2",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,
        0, 0, hInstance, 0);

    // WM_SIZE should fire during CreateWindowExA, but resize explicitly too
    // just in case, so we never present with a null backbuffer.
    RECT r;
    GetClientRect(g_hwnd, &r);
    backbuffer_resize(&g_bb, r.right - r.left, r.bottom - r.top);

    while (g_running) {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_running = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        render_frame(&g_bb);

        HDC dc = GetDC(g_hwnd);
        RECT client;
        GetClientRect(g_hwnd, &client);
        backbuffer_present(&g_bb, dc, client.right - client.left, client.bottom - client.top);
        ReleaseDC(g_hwnd, dc);
    }

    return 0;
}