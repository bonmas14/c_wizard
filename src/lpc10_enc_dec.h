/*
    LPC10 - simple audio encoder/decoder for tms5220.

    Language version C89/C99. (compiles as -std=c89 with gcc)
    standard math library.

    USAGE:

    In exactly one translation unit define LPC_ENC_DEC_IMPLEMENTATION
    before including this library:

    ```c

    #define LPC_ENC_DEC_IMPLEMENTATION
    #include "lpc10_enc_dec.h" 

    ```

    DEFINES:
    
    LPC_ENC_DEC_IMPLEMENTATION (required once) - will include implementation
    LPC_STATIC_DECL            (optional)      - makes all declarations static.
    NDEBUG                     (optional)      - will define internal asserts as (void)(expr).

    assert(expr)    - redefine to bypass standard assertion mechanism, it also bypases including stdio.h.
    LPC_ALLOC(size) - redefine to change allocation strategies (also need to redefine LPC_FREE).
    LPC_FREE(ptr)   - same as LPC_ALLOC.


    LICENSE:

    Copyright (C) 2025 Bogdan Masyutin (bonmas14)

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

      1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
      2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
      3. This notice may not be removed or altered from any source distribution.

    Bogdan Masyutin - bonmas14@gmail.com

    CHANGELOG:
    v1.0 Init version.
    v1.1 Adding LPC_UNUSED and LPC_INLINE macro as I forget to add it, preprocessor typos and coments style.
*/

#if !defined(LPC_ENC_DEC_H)
#define LPC_ENC_DEC_H

#if defined(LPC_STATIC_DECL)
#define LPC_API static
#else
#define LPC_API 
#endif /* LPC_STATIC_DECL */

#if !defined(CLITERAL)
#if defined(__cplusplus)
#define CLITERAL(type) type
#else
#define CLITERAL(type) (type)
#endif /* __cplusplus */
#endif /* CLITERAL    */

#define LPC_UNUSED(x) (void)(x)

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <math.h>
#include <string.h>
#include <float.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define LPC_MAX(a, b) (a) > (b) ? (a) : (b)
#define LPC_MIN(a, b) (a) < (b) ? (a) : (b)

#define LPC_PI 3.14159265358979323846f
#define LPC_TAU (LPC_PI * 2)

#define LPC_SAMPLE_RATE 8000
#define LPC_SAMPLES     200
#define LPC_ENERGY_ZERO 0x0
#define LPC_ENERGY_STOP 0xf

#if !defined(LPC_ALLOC)
#include <stdlib.h>
#define LPC_ALLOC(size) calloc(1, size)
#else
#if !defined(LPC_FREE)
#error "LPC_ALLOC was redefined but not LPC_FREE"
#endif /* LPC_FREE  */
#endif /* LPC_ALLOC */

#if !defined(LPC_FREE)
#define LPC_FREE(ptr)   free(ptr)
#else
#if !defined(LPC_ALLOC)
#error "LPC_FREE was redefined but not LPC_ALLOC"
#endif /* LPC_ALLOC */
#endif /* LPC_FREE */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define LPC_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define LPC_INLINE __inline__
#elif defined(_MSC_VER)
#define LPC_INLINE __inline
#else
#define LPC_INLINE
#endif


#if !defined(assert)
#if !defined(NDEBUG)
#include <stdio.h>

#define assert(expr) if ((int)(expr) == 0) {\
    fprintf(stderr, "Assert at %s:%d failed!", __FILE__, __LINE__); \
    *((int *)0) = 0; \
}

#else
#define assert(expr) (void)(expr)
#endif /* NDEBUG */
#endif /* assert */

/*
// Type declarations
*/

typedef uint64_t lpc_u64;
typedef uint32_t lpc_u32;
typedef uint16_t lpc_u16;
typedef uint8_t  lpc_u8;

typedef int64_t lpc_s64;
typedef int32_t lpc_s32;
typedef int16_t lpc_s16;
typedef int8_t  lpc_s8;

typedef int32_t lpc_b32;
typedef float   lpc_f32;

typedef lpc_u8 lpc_u1;
typedef lpc_u8 lpc_u2;
typedef lpc_u8 lpc_u3;
typedef lpc_u8 lpc_u4;
typedef lpc_u8 lpc_u5;
typedef lpc_u8 lpc_u6;
typedef lpc_u8 lpc_u7;

/*
// Intermediate representation of tms5220 code,
// just before converting it into bit stream
// @todo, skip this step?
*/

typedef lpc_u64 lpc_bitcode;

typedef struct {
    lpc_f32 pitch_low_cut, pitch_high_cut, pitch_q_factor;                /* filter settings that will be applied on pitch recognition        */
    lpc_f32 processing_low_cut, processing_high_cut, processing_q_factor; /* filter settings that will be applied on calculating K parameters */

    lpc_f32 unvoiced_thresh;
    lpc_f32 unvoiced_rms_multiply;
    lpc_b32 do_pre_emphasis;
    lpc_f32 pre_emphasis_alpha;
    
    lpc_u32 frame_size_ms;
    lpc_u32 window_size_in_segments;
} Lpc_Encoder_Settings;

#define LPC_DEFAULT_SETTINGS CLITERAL(Lpc_Encoder_Settings) {\
    50.0f, 500.0f, 4.0f,   \
    50.0f, 4000.0f, 1.0f,  \
    -0.1f, 2.0f,           \
    true, -0.9373,         \
    25, 2                  \
}

/* 
// @todo, fixed version
*/
typedef struct {
    lpc_u32 sample_rate;
    lpc_u32 channels;
    lpc_u32 frame_count;
    lpc_f32 *samples;
} Lpc_Sample_Buffer;

typedef struct {
    lpc_u4 energy;
    lpc_u1 repeat;
    lpc_u6 pitch;

    union {
        struct {
            lpc_u5 k1, k2;
            lpc_u4 k3, k4, k5, k6, k7;
            lpc_u3 k8, k9, k10;
        };

        lpc_u8 k[10];
    };
} Lpc_Code;

typedef struct {
    lpc_u32 count;
    /*
    // @note: I use offsets from buffer start instead of pointers because
    // it allows me to use multiple buffers with same segment info.
    // So the info is shared.                       @bonmas
    */
    lpc_u32 buffer_offset;

    lpc_u32 table_energy;
    lpc_u32 table_pitch;
    lpc_u32 table_k[10];
} Lpc_Segment;

typedef struct {
    lpc_u32 count;
    Lpc_Code *code;
} Lpc_Codes;

typedef struct {
    lpc_u32      count;
    Lpc_Segment *data;
} Lpc_Segments;

typedef struct {
    lpc_u32 count;
    lpc_u8 *bytes;
} Lpc_TMS5220_Buffer;

typedef struct {
    lpc_bitcode code;
    lpc_u32     bits_count;
    lpc_b32     not_enough_bits;
} Lpc_Bitcode_Info;

typedef struct {
    lpc_f32 energy;
    lpc_u32 pitch;
    lpc_f32 k[10];
} Lpc_Synth;

typedef struct {
    lpc_f32 b0, b1, b2;
    lpc_f32 a0, a1, a2;

    lpc_f32 x1, x2;
    lpc_f32 y1, y2;
} Lpc_Biquad_Filter;

typedef struct {
    lpc_u64 count;
    lpc_u64 capacity;
    lpc_u64 element_size;
    void *data;
} Lpc_List;

/*
// API
*/

/* Helper function to make sure, codes are correct */
LPC_API Lpc_Code           lpc_code_clamp(Lpc_Code code);

LPC_API Lpc_Codes          lpc_encode(Lpc_Sample_Buffer buffer, Lpc_Encoder_Settings settings);
LPC_API Lpc_Sample_Buffer  lpc_decode(Lpc_Codes codes);

LPC_API void               lpc_codes_free(Lpc_Codes *codes);
LPC_API void               lpc_buffer_free(Lpc_Sample_Buffer *buffer);

LPC_API Lpc_TMS5220_Buffer lpc_tms5220_encode(Lpc_Codes codes);
LPC_API Lpc_Codes          lpc_tms5220_decode(Lpc_TMS5220_Buffer buffer);
LPC_API void               lpc_tms5220_buffer_free(Lpc_TMS5220_Buffer *buffer);

LPC_API Lpc_List           lpc_list_create(lpc_u64 init_size, lpc_u64 element_size);
LPC_API void               lpc_list_destroy(Lpc_List *list);

LPC_API void*              lpc_list_get(Lpc_List *list, lpc_u64 index);
LPC_API lpc_b32            lpc_list_append(Lpc_List *list, void *data);


#if defined(__cplusplus)
}
#endif

#if defined(LPC_ENC_DEC_IMPLEMENTATION)

#define LPC_START_BIT         49LL
#define LPC_UNVOICED_STOP_BIT 21LL
#define LPC_REPEAT_STOP_BIT   38LL

#define LPC_SIGNAL_BIT        46LL
#define LPC_REPEAT_BIT        45LL

#define LPC_ENERGY_MASK         0x0FLL
#define LPC_REP_MASK            0x01LL
#define LPC_PITCH_MASK          0x3FLL

#define LPC_K1_K2_MASK          0x1FLL
#define LPC_K3_K4_K5_K6_K7_MASK 0x0FLL
#define LPC_K8_K9_K10_MASK      0x07LL

#define LPC_K10_OFFSET     0LL
#define LPC_K9_OFFSET      3LL
#define LPC_K8_OFFSET      6LL
#define LPC_K7_OFFSET      9LL
#define LPC_K6_OFFSET     13LL
#define LPC_K5_OFFSET     17LL
#define LPC_K4_OFFSET     21LL
#define LPC_K3_OFFSET     25LL
#define LPC_K2_OFFSET     29LL
#define LPC_K1_OFFSET     34LL
#define LPC_PITCH_OFFSET  39LL
#define LPC_REP_OFFSET    45LL
#define LPC_ENERGY_OFFSET 46LL

#define LPC_INTEPR_SAMPLES    25
#define LPC_BIT_FRAME_SIZE    50
#define LPC_CHIRP_TABLE_SIZE  52

/* LATER_CHIRP, from python_wizard: https://github.com/ptwz/python_wizard */
LPC_API lpc_f32 chirp_table[LPC_CHIRP_TABLE_SIZE] = {
     0,   3,   15,  40,  76,  108, 113,  80,
     37,  38,  76,  68,  26,  50,  59,  19,
     55,  26,  37,  31,  29,  0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,  
};

LPC_API lpc_f32 energy_table[LPC_ENERGY_MASK + 1] = {
       0,   52,   87,  123,
     174,  246,  348,  491,
     694,  981, 1385, 1957,
    2764, 3904, 5514, 7789
};

LPC_API lpc_u32 pitch_table[LPC_PITCH_MASK + 1] = { 
    0,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30, 31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  44,  46,  48,
    50, 52,  53,  56,  58,  60,  62,  65,  68,  70,  72,  76,  78,  80,  84,  86,
    91, 94,  98, 101, 105, 109, 114, 118, 122, 127, 132, 137, 142, 148, 153, 159,
};

LPC_API lpc_f32 k1_table[LPC_K1_K2_MASK + 1] = {
    -0.97850, -0.97270, -0.97070, -0.96680,
    -0.96290, -0.95900, -0.95310, -0.94140,
    -0.93360, -0.92580, -0.91600, -0.90620,
    -0.89650, -0.88280, -0.86910, -0.85350,

    -0.80420, -0.74058, -0.66019, -0.56116,
    -0.44296, -0.30706, -0.15735, -0.00005,
     0.15725,  0.30696,  0.44288,  0.56109,
     0.66013,  0.75054,  0.80416,  0.85350,
};

LPC_API lpc_f32 k2_table[LPC_K1_K2_MASK + 1] = {
    -0.64000, -0.58999, -0.53500, -0.47507,
    -0.41039, -0.34129, -0.26830, -0.19209,
    -0.11350, -0.03345,  0.04702,  0.12690,
     0.20515,  0.28087,  0.35325,  0.42163,

     0.48553,  0.54464,  0.59878,  0.64796,
     0.69227,  0.73190,  0.76714,  0.79828,
     0.82567,  0.84965,  0.87057,  0.88875,
     0.90451,  0.91813,  0.92988,  0.98830
};

LPC_API lpc_f32 k3_table[LPC_K3_K4_K5_K6_K7_MASK + 1] = {
    -0.86000, -0.75467, -0.64933, -0.54400,
    -0.43867, -0.33333, -0.22800, -0.12267,
    -0.01733,  0.08800,  0.19333,  0.29867,
     0.40400,  0.50933,  0.61467,  0.72000
};

LPC_API lpc_f32 k4_table[LPC_K3_K4_K5_K6_K7_MASK + 1] = {
    -0.64000, -0.53145, -0.42289, -0.31434,
    -0.20579, -0.09723,  0.01132,  0.11987,
     0.22843,  0.33698,  0.44553,  0.55409,
     0.66264,  0.77119,  0.87975,  0.98830
};

LPC_API lpc_f32 k5_table[LPC_K3_K4_K5_K6_K7_MASK + 1] = {
    -0.64000, -0.54933, -0.45867, -0.36800,
    -0.27733, -0.18667, -0.09600, -0.00533,
     0.08533,  0.17600,  0.26667,  0.35733,
     0.44800,  0.53867,  0.62933,  0.72000
};

LPC_API lpc_f32 k6_table[LPC_K3_K4_K5_K6_K7_MASK + 1] = {
    -0.50000, -0.41333, -0.32667, -0.24000,
    -0.15333, -0.06667,  0.02000,  0.10667,
     0.19333,  0.28000,  0.36667,  0.45333,
     0.54000,  0.62667,  0.71333,  0.80000
};

LPC_API lpc_f32 k7_table[LPC_K3_K4_K5_K6_K7_MASK + 1] = {
    -0.60000, -0.50667, -0.41333, -0.32000,
    -0.22667, -0.13333, -0.04000,  0.05333,
     0.14667,  0.24000,  0.33333,  0.42667,
     0.52000,  0.61333,  0.70667,  0.80000
};

LPC_API lpc_f32 k8_table[LPC_K8_K9_K10_MASK + 1]  = {
    -0.50000, -0.31429, -0.12857,  0.05714,
     0.24286,  0.42857,  0.61429,  0.80000
};

LPC_API lpc_f32 k9_table[LPC_K8_K9_K10_MASK + 1]  = {
    -0.50000, -0.34286, -0.18571, -0.02857,
     0.12857,  0.28571,  0.44286,  0.60000
};

LPC_API lpc_f32 k10_table[LPC_K8_K9_K10_MASK + 1] = {
    -0.40000, -0.25714, -0.11429,  0.02857,
     0.17143,  0.31429,  0.45714,  0.60000
};

/*
// Filtering
*/

LPC_API Lpc_Biquad_Filter biquad_bandpass_design(lpc_u32 sample_rate, lpc_f32 low_cut, lpc_f32 high_cut, lpc_f32 q_factor, lpc_b32 q_amplify) {
    lpc_f32 center, w, w_cos, w_sin, alpha;
    Lpc_Biquad_Filter filter;

    center = (low_cut + high_cut) / 2.0f;

    w = LPC_TAU * (center / (lpc_f32)sample_rate);

    w_cos = cosf(w);
    w_sin = sinf(w);

    alpha = w_sin / (2.0f * q_factor);

    if (q_amplify) {
        filter.b0 = alpha * q_factor;
        filter.b1 = 0.0f;
        filter.b2 = -alpha * q_factor;
    } else {
        filter.b0 = alpha;
        filter.b1 = 0.0f;
        filter.b2 = -alpha;
    }

    filter.a0 =  1.0f + alpha;
    filter.a1 = -2.0f * w_cos;
    filter.a2 =  1.0f - alpha;

    filter.b0 /= filter.a0;
    filter.b1 /= filter.a0;
    filter.b2 /= filter.a0;
    filter.a1 /= filter.a0;
    filter.a2 /= filter.a0;
    filter.a0  = 1.0f;

    filter.x1 = filter.x2 = 0;
    filter.y1 = filter.y2 = 0;

    return filter;
}

LPC_API LPC_INLINE lpc_f32 biquad_process(Lpc_Biquad_Filter *filter, lpc_f32 input) {
    lpc_f32 output;

    output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                                - filter->a1 * filter->y1 - filter->a2 * filter->y2;

    filter->x2 = filter->x1;
    filter->y2 = filter->y1;
    filter->x1 = input;
    filter->y1 = output;

    return output;
}

/* 
// Helpers
*/

LPC_API lpc_f32 lpc_lerpf(lpc_f32 a, lpc_f32 b, lpc_f32 t) {
    return (1.0f - t) * a + b * t;
}

LPC_API Lpc_Code lpc_code_clamp(Lpc_Code code) {
    Lpc_Code output;

    memset(&output, 0, sizeof(Lpc_Code));
    output.energy = ((lpc_u64)code.energy) & LPC_ENERGY_MASK;

    if (output.energy == LPC_ENERGY_ZERO || output.energy == LPC_ENERGY_STOP) {
        return output;
    }

    output.repeat = ((lpc_u64)code.repeat) & LPC_REP_MASK;
    output.pitch  = ((lpc_u64)code.pitch)  & LPC_PITCH_MASK;

    if (output.repeat) {
        return output;
    }
                  
    output.k1  = ((lpc_u64)code.k1)  & LPC_K1_K2_MASK;
    output.k2  = ((lpc_u64)code.k2)  & LPC_K1_K2_MASK;
    output.k3  = ((lpc_u64)code.k3)  & LPC_K3_K4_K5_K6_K7_MASK;
    output.k4  = ((lpc_u64)code.k4)  & LPC_K3_K4_K5_K6_K7_MASK;

    if (!output.pitch) {
        return output;
    }

    output.k5  = ((lpc_u64)code.k5)  & LPC_K3_K4_K5_K6_K7_MASK;
    output.k6  = ((lpc_u64)code.k6)  & LPC_K3_K4_K5_K6_K7_MASK;
    output.k7  = ((lpc_u64)code.k7)  & LPC_K3_K4_K5_K6_K7_MASK;
    output.k8  = ((lpc_u64)code.k8)  & LPC_K8_K9_K10_MASK;
    output.k9  = ((lpc_u64)code.k9)  & LPC_K8_K9_K10_MASK;
    output.k10 = ((lpc_u64)code.k10) & LPC_K8_K9_K10_MASK;

    return output;
}

LPC_API lpc_bitcode lpc_convert_to_bitcode_internal(Lpc_Code bitcode) {
    lpc_bitcode output = 0;

    output |= (((lpc_u64)bitcode.energy) & LPC_ENERGY_MASK) << LPC_ENERGY_OFFSET;
    output |= (((lpc_u64)bitcode.repeat) & LPC_REP_MASK)    << LPC_REP_OFFSET;
    output |= (((lpc_u64)bitcode.pitch)  & LPC_PITCH_MASK)  << LPC_PITCH_OFFSET;

    output |= (((lpc_u64)bitcode.k1)  & LPC_K1_K2_MASK)          << LPC_K1_OFFSET;
    output |= (((lpc_u64)bitcode.k2)  & LPC_K1_K2_MASK)          << LPC_K2_OFFSET;
    output |= (((lpc_u64)bitcode.k3)  & LPC_K3_K4_K5_K6_K7_MASK) << LPC_K3_OFFSET;
    output |= (((lpc_u64)bitcode.k4)  & LPC_K3_K4_K5_K6_K7_MASK) << LPC_K4_OFFSET;
    output |= (((lpc_u64)bitcode.k5)  & LPC_K3_K4_K5_K6_K7_MASK) << LPC_K5_OFFSET;
    output |= (((lpc_u64)bitcode.k6)  & LPC_K3_K4_K5_K6_K7_MASK) << LPC_K6_OFFSET;
    output |= (((lpc_u64)bitcode.k7)  & LPC_K3_K4_K5_K6_K7_MASK) << LPC_K7_OFFSET;
    output |= (((lpc_u64)bitcode.k8)  & LPC_K8_K9_K10_MASK)      << LPC_K8_OFFSET;
    output |= (((lpc_u64)bitcode.k9)  & LPC_K8_K9_K10_MASK)      << LPC_K9_OFFSET;
    output |= (((lpc_u64)bitcode.k10) & LPC_K8_K9_K10_MASK);

    return output;
}

LPC_API Lpc_Code lpc_convert_from_bitcode_internal(lpc_bitcode bitcode) {
    Lpc_Code output;

    memset(&output, 0, sizeof(output));
    output.energy = ((lpc_u64)bitcode & (LPC_ENERGY_MASK << LPC_ENERGY_OFFSET)) >> LPC_ENERGY_OFFSET;

    if (output.energy == LPC_ENERGY_ZERO || output.energy == LPC_ENERGY_STOP) {
        return output;
    }

    output.repeat = ((lpc_u64)bitcode & (LPC_REP_MASK   << LPC_REP_OFFSET))    >> LPC_REP_OFFSET;
    output.pitch  = ((lpc_u64)bitcode & (LPC_PITCH_MASK << LPC_PITCH_OFFSET))  >> LPC_PITCH_OFFSET;

    if (output.repeat) {
        return output;
    }

    output.k1  = ((lpc_u64)bitcode & (LPC_K1_K2_MASK << LPC_K1_OFFSET)) >> LPC_K1_OFFSET;
    output.k2  = ((lpc_u64)bitcode & (LPC_K1_K2_MASK << LPC_K2_OFFSET)) >> LPC_K2_OFFSET;
    output.k3  = ((lpc_u64)bitcode & (LPC_K3_K4_K5_K6_K7_MASK << LPC_K3_OFFSET)) >> LPC_K3_OFFSET;
    output.k4  = ((lpc_u64)bitcode & (LPC_K3_K4_K5_K6_K7_MASK << LPC_K4_OFFSET)) >> LPC_K4_OFFSET;

    if (!output.pitch) {
        return output;
    }

    output.k5  = ((lpc_u64)bitcode & (LPC_K3_K4_K5_K6_K7_MASK << LPC_K5_OFFSET)) >> LPC_K5_OFFSET;
    output.k6  = ((lpc_u64)bitcode & (LPC_K3_K4_K5_K6_K7_MASK << LPC_K6_OFFSET)) >> LPC_K6_OFFSET;
    output.k7  = ((lpc_u64)bitcode & (LPC_K3_K4_K5_K6_K7_MASK << LPC_K7_OFFSET)) >> LPC_K7_OFFSET;
    output.k8  = ((lpc_u64)bitcode & (LPC_K8_K9_K10_MASK << LPC_K8_OFFSET)) >> LPC_K8_OFFSET;
    output.k9  = ((lpc_u64)bitcode & (LPC_K8_K9_K10_MASK << LPC_K9_OFFSET)) >> LPC_K9_OFFSET;
    output.k10 = ((lpc_u64)bitcode &  LPC_K8_K9_K10_MASK);

    return output;
}

/*
// Encoding
*/

LPC_API Lpc_Sample_Buffer lpc_buffer_prepare_internal(Lpc_Sample_Buffer buffer) {
    Lpc_Sample_Buffer converted;
    lpc_u64 i, j, k;
    lpc_f32 sum;

    assert(buffer.samples != NULL);
    assert(buffer.channels <= 2);
    assert(buffer.channels > 0);

    converted.sample_rate = LPC_SAMPLE_RATE;
    converted.channels    = 1;
    converted.frame_count = roundf((lpc_f32)buffer.frame_count / ((lpc_f32)buffer.sample_rate / (lpc_f32)LPC_SAMPLE_RATE));
    converted.samples     = (lpc_f32*)LPC_ALLOC((sizeof(lpc_f32) * converted.frame_count));

    assert(converted.samples != NULL); /* @todo, proper recovery if no memory */

    for (i = 0; i < converted.frame_count; i++) {
        j = roundf((lpc_f32)i * ((lpc_f32)buffer.sample_rate / (lpc_f32)LPC_SAMPLE_RATE));

        if (j >= buffer.frame_count) {
            converted.samples[i] = 0;
            continue;
        }

        if (buffer.channels == 1) {
            if (j >= buffer.frame_count) {
                converted.samples[i] = 0;
                continue;
            }

            converted.samples[i] = buffer.samples[j];
        } else {
            sum = 0;

            for (k = 0; k < buffer.channels; k++) {
                if ((j * buffer.channels + k) >= (buffer.frame_count * buffer.channels)) {
                    break;
                }

                sum += buffer.samples[j * buffer.channels + k];
            }

            converted.samples[i] = sum / (lpc_f32)buffer.channels;
        }
    }

    return converted;
}

LPC_API Lpc_Sample_Buffer lpc_buffer_copy_internal(Lpc_Sample_Buffer buffer) {
    Lpc_Sample_Buffer new_buffer;

    assert(buffer.channels    == 1);
    assert(buffer.sample_rate == LPC_SAMPLE_RATE);

    new_buffer = buffer;
    new_buffer.samples = (lpc_f32*)LPC_ALLOC((sizeof(lpc_f32) * buffer.frame_count));
    
    assert(new_buffer.samples != NULL); /* @todo, proper recovery */
    memcpy(new_buffer.samples, buffer.samples, sizeof(lpc_f32) * new_buffer.frame_count);

    return new_buffer;
}


LPC_API void lpc_buffer_normalize_internal(Lpc_Sample_Buffer buffer) {
    lpc_u64 i;
    lpc_f32 max, min;

    max = FLT_MIN;
    min = FLT_MAX;

    assert(buffer.sample_rate == LPC_SAMPLE_RATE);
    assert(buffer.channels == 1);

    for (i = 0; i < buffer.frame_count; i++) {
        if (buffer.samples[i] < min) min = buffer.samples[i];
        if (buffer.samples[i] > max) max = buffer.samples[i];
    }

    for (i = 0; i < buffer.frame_count; i++) {
        buffer.samples[i] = (buffer.samples[i] - min) / (max - min);
    }
}

LPC_API void lpc_buffer_filter_internal(Lpc_Sample_Buffer buffer, lpc_f32 low_cut_freq, lpc_f32 high_cut_freq, lpc_f32 q_factor, lpc_b32 amplify) {
    Lpc_Biquad_Filter filter;
    lpc_u64 i;

    assert(buffer.channels    == 1);
    assert(buffer.sample_rate == LPC_SAMPLE_RATE);

    filter = biquad_bandpass_design(buffer.sample_rate, low_cut_freq, high_cut_freq, q_factor, amplify);

    for (i = 0; i < buffer.frame_count; i++) {
        buffer.samples[i] = biquad_process(&filter, buffer.samples[i]);
    }
}

/* 
// Pre emphasis
*/

LPC_API lpc_f32 lpc_buffer_energy_sqr_sum_internal(Lpc_Sample_Buffer buffer) {
    lpc_u64 i;
    lpc_f32 energy = 0;

    for (i = 0; i < buffer.frame_count; i++) {
        energy += buffer.samples[i] * buffer.samples[i];
    }

    return energy / (buffer.frame_count - 1);
}

LPC_API void lpc_buffer_pre_emphasis(Lpc_Sample_Buffer buffer, lpc_f32 alpha) {
    lpc_u64 i;
    lpc_f32 pre_energy, post_energy, scale;
    pre_energy = lpc_buffer_energy_sqr_sum_internal(buffer);

    for (i = buffer.frame_count - 1; i > 0; i--) {
        buffer.samples[i] = 1 - buffer.samples[i - 1] * alpha;
    }

    post_energy = lpc_buffer_energy_sqr_sum_internal(buffer);
    scale = sqrtf(pre_energy / post_energy);

    for (i = 0; i < buffer.frame_count; i++) {
        buffer.samples[i] *= scale;
    }
}

/*
// Segments
*/

LPC_API Lpc_Segments lpc_get_segments_internal(Lpc_Sample_Buffer buffer, lpc_u32 segment_size, lpc_u32 num_segments) {
    lpc_u64 i;
    Lpc_Segments segments;

    segments.count = num_segments;
    segments.data  = (Lpc_Segment *)LPC_ALLOC(sizeof(Lpc_Segment) * num_segments);

    assert(segments.data != NULL); /* @todo, proper recovery from memory allocation errors */
    assert(buffer.frame_count < num_segments * segment_size);

    for (i = 0; i < num_segments; i++) {
        segments.data[i].count   = LPC_MIN(buffer.frame_count - i * segment_size, segment_size);
        segments.data[i].buffer_offset = i * segment_size;
    }

    return segments;
}

LPC_API void lpc_pitch_estimate_internal(Lpc_Sample_Buffer buffer, Lpc_Segments segments, lpc_u32 window_size, lpc_f32 low_freq, lpc_f32 high_freq) {
    lpc_u64 i, j, k, offset, best_period_i, min_dist_i, segment_size, work_buffer_size;
    lpc_u32 min_period, max_period, best_period, period_count;
    lpc_f32 *work_buffer, *window, *periods, best_period_value;
    lpc_f32 min_dist, dist;

    assert(segments.count > 0);

    min_period  = buffer.sample_rate / high_freq;
    max_period  = buffer.sample_rate / low_freq;
    best_period = min_period;
    
    period_count = max_period - min_period;
    periods      = (lpc_f32 *)LPC_ALLOC(sizeof(lpc_f32) * period_count);

    assert(periods != NULL); /* @todo, proper recovery from memory allocation errors */

    /*
    // we assume that first segment is maximum size, @todo, test for that,
    // as it should be always like that, except the garbage data
    */
    
    segment_size     = segments.data[0].count;
    work_buffer_size = window_size * segment_size;
    work_buffer      = (lpc_f32 *)LPC_ALLOC(sizeof(lpc_f32) * work_buffer_size);
    window           = (lpc_f32 *)LPC_ALLOC(sizeof(lpc_f32) * work_buffer_size);
    
    assert(work_buffer != NULL); /* @todo, proper recovery from memory allocation errors */
    assert(window      != NULL); /* @todo, proper recovery from memory allocation errors */

    /*
    // @note apparently we need normalized coefficients in here, so we can get more accurate pitch correlation
    // it is made in python-wizard via calculating correlations coefficients for every lag value
    // but it works anyway?
    */

    for (i = 0; i < work_buffer_size; i++) {
        window[i] = 0.54f - 0.46f * cosf(LPC_TAU * ((lpc_f32)i / (lpc_f32)(work_buffer_size - 1)));
    }

    for (i = 0; i < segments.count; i++) {
        offset = 0;
        memset(work_buffer, 0, sizeof(lpc_f32) * work_buffer_size);
        memcpy(work_buffer, buffer.samples + segments.data[i].buffer_offset, sizeof(lpc_f32) * segments.data[i].count);
        offset += segments.data[i].count;

        for (j = 1; j < window_size; j++) {
            if ((i + j) >= segments.count) break;
            memcpy(work_buffer + offset, buffer.samples + segments.data[i + j].buffer_offset, sizeof(lpc_f32) * segments.data[i + j].count);
            offset += segments.data[i + j].count;
        }

        for (j = 0; j < work_buffer_size; j++) {
            work_buffer[j] *= window[j];
        }

        { /* calculate best correlation factor */
            for (j = 0; j < period_count; j++) {
                periods[j] = 0;

                for (k = 0; k < segment_size; k++) {
                    periods[j] += work_buffer[k + min_period + j] * work_buffer[k];
                }
            }

            best_period_i     = 0;
            best_period_value = periods[0];

            for (j = 1; j < period_count; j++) {
                if (periods[j] > best_period_value) {
                    best_period_i = j;
                    best_period_value = fabsf(periods[j]);
                }
            }
        }

        best_period = min_period + best_period_i;

        min_dist = max_period;
        min_dist_i = 0;

        for (k = 0; k < LPC_PITCH_MASK; k++) {
            dist = fabsf((lpc_f32)pitch_table[k] - best_period);

            if (min_dist > dist) {
                min_dist = dist;
                min_dist_i = k;
            }
        }

        segments.data[i].table_pitch = min_dist_i;
    }

    LPC_FREE(work_buffer);
    LPC_FREE(periods);
}

LPC_API Lpc_Codes lpc_get_codes_from_segments_internal(Lpc_Segments segments) {
    Lpc_Codes codes;
    Lpc_Code code;
    lpc_u64 i, j;

    codes.count = segments.count + 1;
    codes.code  = (Lpc_Code *)LPC_ALLOC(sizeof(Lpc_Code) * codes.count);

    if (codes.code == NULL) {
        codes.count = 0;
        return codes;
    }

    for (i = 0; i < segments.count; i++) {
        code.energy = (lpc_u4)segments.data[i].table_energy;
        code.repeat = 0; /* python wizard doesnt support it, but we can @todo */
        code.pitch  = (lpc_u6)segments.data[i].table_pitch;

        for (j = 0; j < 10; j++) {
            code.k[j] = segments.data[i].table_k[j];
        }

        codes.code[i] = lpc_code_clamp(code);
    }

    code.energy = LPC_ENERGY_STOP;
    codes.code[codes.count - 1] = lpc_code_clamp(code);

    return codes;
}


LPC_API Lpc_Codes lpc_encode(Lpc_Sample_Buffer buffer, Lpc_Encoder_Settings settings) {
    Lpc_Sample_Buffer pitch_buffer;
    Lpc_Codes codes;
    lpc_u64 size, i, j, k, l;
    Lpc_Segments segments;
    lpc_f32 sum, k_params[11], coeff[11];

    assert(buffer.sample_rate >= LPC_SAMPLE_RATE);
    buffer       = lpc_buffer_prepare_internal(buffer);
    pitch_buffer = lpc_buffer_copy_internal(buffer);

    lpc_u32 segment_size = buffer.sample_rate / 1000 * settings.frame_size_ms;
    lpc_u32 num_segments = ceilf((lpc_f32)buffer.frame_count / (lpc_f32)segment_size);

    segments = lpc_get_segments_internal(buffer, segment_size, num_segments);

    if (settings.do_pre_emphasis) {
        lpc_buffer_pre_emphasis(buffer, settings.pre_emphasis_alpha);
    }

    lpc_buffer_filter_internal(buffer, settings.processing_low_cut, settings.processing_high_cut, settings.processing_q_factor, true);
    lpc_buffer_filter_internal(pitch_buffer, settings.pitch_low_cut, settings.pitch_high_cut, settings.pitch_q_factor, false);
    lpc_pitch_estimate_internal(pitch_buffer, segments, settings.window_size_in_segments, settings.pitch_low_cut, settings.pitch_high_cut);

    for (i = 0; i < num_segments; i++) {
        memset(coeff, 0, sizeof(coeff));

        /* so we need to get the LPC coefficients, and this loop basically does it */
        for (j = 0; j < 11; j++) {
            size = segment_size - j;
            sum = 0;

            for (k = 0; k < size; k++) {
                l = k + i * segment_size;

                if ((l + j) >= buffer.frame_count) continue;

                sum += buffer.samples[l] * buffer.samples[l + j];
            }

            coeff[j] = sum;
        }

        /* here we convert the lpc coefficients to K reflection coeffs */

        { /* Leroux Guegen algorithm for finding K */
            lpc_f32 y, b_params[11], d_params[12];

            memset(k_params, 0, sizeof(k_params));
            memset(b_params, 0, sizeof(b_params));
            memset(d_params, 0, sizeof(d_params));

            k_params[1] = -coeff[1] / coeff[0];
            d_params[1] =  coeff[1];
            d_params[2] =  coeff[0] + (k_params[1] * coeff[1]);

            for (j = 2; j < 11; j++) {
                y = coeff[j];
                b_params[1] = y;

                for (k = 1; k < j; k++) {
                    b_params[k + 1] = d_params[k] + (k_params[k] * y);
                    y += k_params[k] * d_params[k];
                    d_params[k] = b_params[k];
                }

                k_params[j] = -y / d_params[j];
                d_params[j + 1] = d_params[j] + (k_params[j] * y);
                d_params[j] = b_params[j];
            }


            if (k_params[1] > settings.unvoiced_thresh) {
                segments.data[i].table_pitch = 0;
            }

            { /* setting RMS of signal */
                lpc_f32 rms, dist, min_dist;
                lpc_u64 min_dist_i;

                rms = sqrtf(d_params[11] / segment_size) * (1 << 18);

                if (segments.data[i].table_pitch == 0) {
                    rms *= settings.unvoiced_rms_multiply;
                }

                min_dist = fabsf(energy_table[0] - rms);
                min_dist_i = 0;

                for (j = 1; j < LPC_ENERGY_MASK; j++) {
                    dist = fabsf(energy_table[j]  - rms);

                    if (dist < min_dist) {
                        min_dist = dist;
                        min_dist_i = j;
                    }
                }

                segments.data[i].table_energy = min_dist_i;
            }
        }

        {
            /* and then we set the Ks to segments */
            lpc_f32 dist, min_dist;
            lpc_u64 min_dist_i;
            lpc_f32 *k_table = NULL;

            /*
            // K1 K2
            */
            for (j = 0; j < 2; j++) {
                switch (j) {
                    case 0:  k_table = k1_table; break;
                    case 1:  k_table = k2_table; break;
                    default: assert(false);      break;
                }

                min_dist   = fabsf(k_table[0] - k_params[j + 1]); 
                min_dist_i = 0;

                for (k = 1; k <= LPC_K1_K2_MASK; k++) {
                    dist = fabsf(k_table[k] - k_params[j + 1]);

                    if (dist < min_dist) {
                        min_dist = dist;
                        min_dist_i = k;
                    }
                }

                segments.data[i].table_k[j] = min_dist_i;
            }

            /*
            // K3-K7
            */
            for (j = 2; j < 7; j++) {
                switch (j) {
                    case 2:  k_table = k3_table; break;
                    case 3:  k_table = k4_table; break;
                    case 4:  k_table = k5_table; break;
                    case 5:  k_table = k6_table; break;
                    case 6:  k_table = k7_table; break;
                    default: assert(false);      break;
                }


                min_dist   = fabsf(k_table[0] - k_params[j + 1]);
                min_dist_i = 0;

                for (k = 1; k <= LPC_K3_K4_K5_K6_K7_MASK; k++) {
                    dist = fabsf(k_table[k] - k_params[j + 1]);

                    if (dist < min_dist) {
                        min_dist = dist;
                        min_dist_i = k;
                    }
                }

                segments.data[i].table_k[j] = min_dist_i;
            }

            for (j = 7; j < 10; j++) {
                switch (j) {
                    case 7:  k_table = k8_table;  break;
                    case 8:  k_table = k9_table;  break;
                    case 9:  k_table = k10_table; break;
                    default: assert(false);       break;
                }

                min_dist   = fabsf(k_table[0] - k_params[j + 1]); 
                min_dist_i = 0;

                for (k = 1; k <= LPC_K8_K9_K10_MASK; k++) {
                    dist = fabsf(k_table[k] - k_params[j + 1]);

                    if (dist < min_dist) {
                        min_dist = dist;
                        min_dist_i = k;
                    }
                }

                segments.data[i].table_k[j] = min_dist_i;
            }
        }
    }

    codes = lpc_get_codes_from_segments_internal(segments);

    LPC_FREE(buffer.samples);
    LPC_FREE(pitch_buffer.samples);
    LPC_FREE(segments.data);

    return codes;
}

/*
// Decoding
*/

Lpc_Sample_Buffer lpc_decode(Lpc_Codes codes) {
    lpc_u64 i = 0, j = 0, sample_counter = 0, phase_counter = 0, code_index = 0;
    lpc_f32 in, t, max = FLT_MIN, min = FLT_MAX;
    Lpc_Synth previous, target, current;
    Lpc_Sample_Buffer buffer;
    Lpc_Code curr_code;
    lpc_f32 forward[10], backward[10];
    lpc_u32 noise = 1;
    lpc_b32 repeat;

    memset(forward,   0, sizeof(forward));
    memset(backward,  0, sizeof(backward));
    memset(&previous, 0, sizeof(Lpc_Synth));
    memset(&target,   0, sizeof(Lpc_Synth));
    memset(&current,  0, sizeof(Lpc_Synth));

    buffer.sample_rate = LPC_SAMPLE_RATE;
    buffer.channels    = 1;
    buffer.frame_count = codes.count * LPC_SAMPLES;
    buffer.samples     = (lpc_f32*)LPC_ALLOC(sizeof(lpc_f32) * buffer.frame_count);

    if (buffer.samples == NULL) {
        memset(&buffer, 0, sizeof(Lpc_Sample_Buffer));
        return buffer;
    }

    while (true) {
        if (code_index >= codes.count) {
            break;
        }

        curr_code = lpc_code_clamp(codes.code[code_index++]);

        if (curr_code.energy == LPC_ENERGY_STOP) {
            break;
        } else if (curr_code.energy == LPC_ENERGY_ZERO) {
            target.energy = 0;
        } else {
            target.energy = energy_table[curr_code.energy];
            repeat = curr_code.repeat;
            target.pitch  = pitch_table[curr_code.pitch];

            if (!repeat) {
                target.k[0] = k1_table[curr_code.k1];
                target.k[1] = k2_table[curr_code.k2];
                target.k[2] = k3_table[curr_code.k3];
                target.k[3] = k4_table[curr_code.k4];

                if (target.pitch) {
                    target.k[4] = k5_table[curr_code.k5];
                    target.k[5] = k6_table[curr_code.k6];
                    target.k[6] = k7_table[curr_code.k7];
                    target.k[7] = k8_table[curr_code.k8];
                    target.k[8] = k9_table[curr_code.k9];
                    target.k[9] = k10_table[curr_code.k10];
                } else {
                    target.k[4] = 0;
                    target.k[5] = 0;
                    target.k[6] = 0;
                    target.k[7] = 0;
                    target.k[8] = 0;
                    target.k[9] = 0;
                }
            }
        }

        if (code_index == 0) {
            previous = current = target;
        } else {
            previous = current;
        }

        for (i = 0; i < LPC_SAMPLES; i++) {
            t = ((lpc_f32)i / (lpc_f32)(LPC_SAMPLES - 1));

            current.energy = lpc_lerpf(previous.energy, target.energy, t);
            current.pitch  = (lpc_u32)lpc_lerpf((lpc_f32)previous.pitch, (lpc_f32)target.pitch, t);

            for (j = 0; j < 10; j++) {
                current.k[j] = lpc_lerpf(previous.k[j], target.k[j], t);
            }

            if (current.energy == 0) {
                in = 0;
            } else if (current.pitch > 0) {
                if (phase_counter < current.pitch) {
                    phase_counter++;
                } else {
                    phase_counter = 0;
                }

                if (phase_counter < LPC_CHIRP_TABLE_SIZE) {
                    in = chirp_table[phase_counter] * current.energy;
                } else {
                    in = 0;
                }
            } else {
                noise = (noise >> 1) ^ (noise & 1 ? 0xBD00 : 0);
                in = noise & 1 ? (lpc_f32)(current.energy) : -((lpc_f32)current.energy);
            }

            forward[9] = in         - current.k[9] * backward[9];
            forward[8] = forward[9] - current.k[8] * backward[8];
            forward[7] = forward[8] - current.k[7] * backward[7];
            forward[6] = forward[7] - current.k[6] * backward[6];
            forward[5] = forward[6] - current.k[5] * backward[5];
            forward[4] = forward[5] - current.k[4] * backward[4];
            forward[3] = forward[4] - current.k[3] * backward[3];
            forward[2] = forward[3] - current.k[2] * backward[2];
            forward[1] = forward[2] - current.k[1] * backward[1];
            forward[0] = forward[1] - current.k[0] * backward[0];

            backward[9] = backward[8] + current.k[8] * forward[8];
            backward[8] = backward[7] + current.k[7] * forward[7];
            backward[7] = backward[6] + current.k[6] * forward[6];
            backward[6] = backward[5] + current.k[5] * forward[5];
            backward[5] = backward[4] + current.k[4] * forward[4];
            backward[4] = backward[3] + current.k[3] * forward[3];
            backward[3] = backward[2] + current.k[2] * forward[2];
            backward[2] = backward[1] + current.k[1] * forward[1];
            backward[1] = backward[0] + current.k[0] * forward[0];
            backward[0] = forward[0];

            assert(sample_counter < buffer.frame_count);
            buffer.samples[sample_counter++] = forward[0];
        }
    }

    buffer.frame_count = sample_counter;

    for (i = 0; i < buffer.frame_count; i++) {
        if (buffer.samples[i] > max) max = buffer.samples[i];
        if (buffer.samples[i] < min) min = buffer.samples[i];
    }

    for (i = 0; i < buffer.frame_count; i++) {
        buffer.samples[i] = buffer.samples[i] / (max - min);
    }

    return buffer;
}


LPC_API void lpc_tms5220_encode_bits_internal(Lpc_List *bits, lpc_u64 code) {
    lpc_s64 stop_at = 0, i = LPC_START_BIT;
    lpc_u8 energy, pitch;
    lpc_u1 curr = 0;

    energy = (code >> LPC_ENERGY_OFFSET) & LPC_ENERGY_MASK;
    pitch  = (code >> LPC_PITCH_OFFSET)  & LPC_PITCH_MASK;

    if (stop_at == 0 && (energy == LPC_ENERGY_ZERO || energy == LPC_ENERGY_STOP)) {
        stop_at = LPC_SIGNAL_BIT;
    }

    if (stop_at == 0 && pitch == 0) {
        stop_at = LPC_UNVOICED_STOP_BIT;
    }

    if (stop_at == 0 && code & (1LL << LPC_REPEAT_BIT)) {
        stop_at = LPC_REPEAT_STOP_BIT;
    }

    while (i >= stop_at) {
        curr = ((code & (1LL << i)) >> i);
        lpc_list_append(bits, &curr);
        i--;
    }
}

LPC_API Lpc_Bitcode_Info lpc_tms5220_decode_bits_internal(lpc_u1 *bits, lpc_u64 bits_count) {
    Lpc_Bitcode_Info info;
    lpc_s64 i = LPC_START_BIT;
    lpc_u8 energy, pitch;

    memset(&info, 0, sizeof(Lpc_Bitcode_Info));

    while (true) {
        if (info.bits_count >= bits_count) {
            info.not_enough_bits = true;
            break;
        }

        info.code |= (lpc_u64)bits[info.bits_count++] << i;

        if (i == 0) break;

        if (i == LPC_ENERGY_OFFSET) {
            energy = (info.code >> LPC_ENERGY_OFFSET) & LPC_ENERGY_MASK;
            if (energy == LPC_ENERGY_ZERO || energy == LPC_ENERGY_STOP) {
                break;
            }
        }

        if (i <= LPC_PITCH_OFFSET) {
            pitch  = (info.code >> LPC_PITCH_OFFSET) & LPC_PITCH_MASK;
            if (pitch == 0 && i == LPC_K4_OFFSET) {
                break;
            }
        }

        i--;
    }

    return info;
}

LPC_API void lpc_tms5220_squash_bits_internal(lpc_u8 *bytes, lpc_u64 bytes_count, lpc_u1 *bits, lpc_u64 bits_count) {
    lpc_u64 i, j = 0;
    
    LPC_UNUSED(bytes_count);

    assert((bytes_count * 8) == bits_count);

    for (i = 0; i < bits_count; i++) {
        if ((i % 8) == 0) {
            j++;
        }

        bytes[j - 1] |= bits[i] <<  (i % 8);
    }
}

LPC_API void lpc_tms5220_unsquash_bits_internal(lpc_u1 *cont, lpc_u64 cont_count, lpc_u8 *from, lpc_u64 from_count) {
    lpc_u64 i, j, k = 0;
    lpc_u1 bit;

    LPC_UNUSED(cont_count);

    assert(cont_count == (from_count * 8));
    
    for (i = 0; i < from_count; i++) {
        for (j = 0; j < 8; j++) {
            bit = (from[i] >> j) & 1;

            assert(k < cont_count);
            cont[k++] = bit;
        }
    }
}

LPC_API Lpc_TMS5220_Buffer lpc_tms5220_encode(Lpc_Codes codes) {
    Lpc_TMS5220_Buffer buff;
    Lpc_List bits;
    lpc_u64 i;

    bits = lpc_list_create(codes.count * LPC_BIT_FRAME_SIZE, sizeof(lpc_u1));

    for (i = 0; i < codes.count; i++) {
        lpc_tms5220_encode_bits_internal(&bits, lpc_convert_to_bitcode_internal(lpc_code_clamp(codes.code[i])));
    }

    buff.count = bits.count / 8;

    if (bits.count != (buff.count * 8)) {
        /* we need to shift last bits on amount of bits */
        i = bits.count - buff.count * 8;
        bits.count -= i;
    }

    buff.bytes = (lpc_u8*)LPC_ALLOC(sizeof(lpc_u8) * buff.count);
    assert(buff.bytes != NULL); /* @todo, proper recovery from memory allocation errors */
    
    lpc_tms5220_squash_bits_internal(buff.bytes, buff.count, (lpc_u1*) bits.data, bits.count);
    lpc_list_destroy(&bits);

    return buff;
}

LPC_API Lpc_Codes lpc_tms5220_decode(Lpc_TMS5220_Buffer buffer) {
    Lpc_List codes;
    Lpc_Code code;
    Lpc_Bitcode_Info info;
    lpc_u1 *bits = NULL;
    lpc_u64 i = 0;

    codes = lpc_list_create(buffer.count * (LPC_BIT_FRAME_SIZE / 8), sizeof(Lpc_Code));
    bits  = (lpc_u1*)LPC_ALLOC(buffer.count * 8);

    assert(bits != NULL); /* @todo, proper recovery from memory allocation errors */

    lpc_tms5220_unsquash_bits_internal(bits, buffer.count * 8, (lpc_u8*)buffer.bytes, buffer.count);

    while (i < (buffer.count * 8)) {
        info = lpc_tms5220_decode_bits_internal(bits + i, (buffer.count * 8) - i);
        code = lpc_convert_from_bitcode_internal(info.code);
        lpc_list_append(&codes, &code);
        i += info.bits_count;
    }

    LPC_FREE(bits);

    return CLITERAL(Lpc_Codes) { (lpc_u32)codes.count, (Lpc_Code *)codes.data };
}

LPC_API Lpc_List lpc_list_create(lpc_u64 init_size, lpc_u64 element_size) {
    Lpc_List list;

    memset(&list, 0, sizeof(Lpc_List));

    list.data = LPC_ALLOC(init_size * element_size);
    if (list.data == NULL) return list;

    list.capacity = init_size; 
    list.element_size = element_size;

    return list;
}

LPC_API void lpc_list_destroy(Lpc_List *list) {
    assert(list->data != NULL);

    LPC_FREE(list->data);

    memset(list, 0, sizeof(Lpc_List));
}

LPC_API void *lpc_list_get(Lpc_List *list, lpc_u64 index) {
    if (index >= list->count) {
        return NULL;
    }

    return (lpc_u8*)list->data + list->element_size * index;
}

LPC_API lpc_b32 lpc_list_append(Lpc_List *list, void *data) {
    lpc_u64 size;
    void *p;
    assert(list->element_size > 0);
    assert(list->capacity > 0);

    if ((list->count + 1) >= list->capacity) {
        size = list->capacity * 2;

        p = LPC_ALLOC(list->element_size * size);
        if (p == NULL) return false;

        memcpy(p, list->data, list->element_size * list->capacity);
        LPC_FREE(list->data);

        list->capacity = size;
        list->data = p;
    }

    p = (lpc_u8*)list->data + list->element_size * list->count;

    memcpy(p, data, list->element_size);

    list->count++;
    return true;
}


LPC_API void lpc_codes_free(Lpc_Codes *codes) {
    assert(codes != NULL);

    if (codes->code) {
        LPC_FREE(codes->code);
    }

    memset(codes, 0, sizeof(Lpc_Codes));
}

LPC_API void lpc_buffer_free(Lpc_Sample_Buffer *buffer) {
    assert(buffer != NULL);

    if (buffer->samples) {
        LPC_FREE(buffer->samples);
    }

    memset(buffer, 0, sizeof(Lpc_Sample_Buffer));
}

LPC_API void lpc_tms5220_buffer_free(Lpc_TMS5220_Buffer *buffer) {
    assert(buffer != NULL);

    if (buffer->bytes) {
        LPC_FREE(buffer->bytes);
    }

    memset(buffer, 0, sizeof(Lpc_TMS5220_Buffer));
}

#endif /* LPC_ENC_DEC_IMPLEMENTATION */
#endif /* LPC_ENC_DEC_H */
