#include "raylib.h"

#define RAYMATH_IMPLEMENTATION
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <string.h>

/// -------------

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  f32;
typedef double f64;

typedef uint8_t  b8;
typedef uint32_t b32;

/// -------------

#define UNUSED(x) (void)(x)

#define KB(s) ((u64)(s) * 1024LL)
#define MB(s) (KB(s) * 1024LL)
#define GB(s) (MB(s) * 1024LL)

#define PG(s) ((u64)(s) * KB(4))

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

#if defined(__clang__) || defined(__GNUC__)
#   define CLANG_COMPILER
#   define __TRAP() __builtin_trap()

#elif _MSC_VER >= 1939
#   define MSVC_COMPILER
#   define __TRAP() *((int *)0) = 0

#else
#   error "Unknown compiler"

#endif

#ifndef CLITERAL
#   if defined(__cplusplus)
#       define CLITERAL(type) type
#   else
#       define CLITERAL(type) (type)
#   endif
#endif

#ifndef ERRLOG
#define ERRLOG(...) TraceLog(LOG_ERROR, __VA_ARGS__)
#endif
#ifndef INFLOG
#define INFLOG(...) TraceLog(LOG_INFO, __VA_ARGS__)
#endif

#if DEBUG
#   define assert(result) { \
        if ((result) == 0) { \
            ERRLOG("%s,%zu: --- assertion failed at %s.", __FILE__, (u64)__LINE__, __func__);\
            __TRAP();\
        } \
    }
#else 
#   define assert(...)
#endif

#define MEMSET(dest, data, size)   memset(dest, data, size)
#define MEMCPY(dest, source, size) memcpy(dest, source, size)
#define MEMCMP(a, b, size)         memcmp(a, b, size)

#define TAU (2.0 * PI)

#define WINDOW_WIDTH  900
#define WINDOW_HEIGHT 700 

f32 window_width  = WINDOW_WIDTH;
f32 window_height = WINDOW_HEIGHT;

#include "allocators.c" 
#include "program.c"

int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);

    SetTraceLogLevel(LOG_FATAL);

    SetExitKey(KEY_ESCAPE);

    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(window_width, window_height, "c-wizard");
    SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

    if (!IsWindowReady()) return -1;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_FATAL, "Failed to open audio device.");
    }

    SetTraceLogLevel(LOG_INFO);
    program_init();

    while (!WindowShouldClose()) {
        window_width  = GetScreenWidth();
        window_height = GetScreenHeight();

        temp_reset();

        program_update();
        program_render();
    }

    program_deinit();
    CloseAudioDevice();

    return 0;
}
