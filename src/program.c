#define LPC_STATIC_DECL
#define LPC_ENC_DEC_IMPLEMENTATION
#include "lpc10_enc_dec.h" 

#define MAX_SAMPLES_UPDATE 512
#define SAMPLE_RATE 8000

// AudioStream audio_stream;

typedef enum {
    STATUS_IDLE,
    STATUS_CONVERTING,
} Program_Status;

typedef struct {
    Program_Status status;
    u64            index;
    FilePathList   path_list;
    Lpc_Encoder_Settings settings;
} Program_State;

Program_State state;

void program_init(void) {
    state.status = STATUS_IDLE;
    state.settings = LPC_DEFAULT_SETTINGS; 
}

void program_deinit(void) {
}

void program_update(void) {
    switch (state.status) {
        case STATUS_IDLE:
        {
            if (IsFileDropped()) {
                if (state.path_list.paths != NULL) {
                    UnloadDroppedFiles(state.path_list);
                    memset(&state.path_list, 0, sizeof(FilePathList));
                }

                state.path_list = LoadDroppedFiles();

                state.status = STATUS_CONVERTING;
            }
        } break;

        case STATUS_CONVERTING:
        {
            Lpc_Sample_Buffer samples;
            Lpc_TMS5220_Buffer buffer;
            Lpc_Codes codes;
            Wave wave;
            const char *file_name;

            if (state.index >= state.path_list.count) {
                state.status = STATUS_IDLE;
                state.index  = 0;
                break;
            }

            wave      = LoadWave(state.path_list.paths[state.index]);
            file_name = GetFileNameWithoutExt(state.path_list.paths[state.index]);

            state.index++;

            if (!IsWaveValid(wave)) { 
                break;
            }

            WaveFormat(&wave, LPC_SAMPLE_RATE, 32, 1);

            samples.sample_rate = LPC_SAMPLE_RATE;
            samples.channels    = 1;
            samples.frame_count = wave.frameCount;
            samples.samples     = (f32*)wave.data;

            codes  = lpc_encode(samples, state.settings);
            buffer = lpc_tms5220_encode(codes);

            UnloadWave(wave);
            memset(&wave, 0, sizeof(Wave));

            samples         = lpc_decode(codes);
            wave.sampleRate = samples.sample_rate;
            wave.sampleSize = 32;
            wave.channels   = 1;
            wave.frameCount = samples.frame_count;
            wave.data       = (void*)samples.samples;

            ExportWave(wave, TextFormat("lpc10_%s.wav", file_name));
            ExportDataAsCode(buffer.bytes, buffer.count, TextFormat("lpc10_%s.h", file_name));

            lpc_codes_free(&codes);
            lpc_buffer_free(&samples);
            lpc_tms5220_buffer_free(&buffer);
        } break;
    }
}

void program_render(void) {
    Font font;
    Vector2 pos, size;
    const char *text;
    u64 i;

    BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x1c, 0x1c, 0x1c, 0xff});

    text = ">> drag and drop files here <<";
    font = GetFontDefault();

    size = MeasureTextEx(font, text, 24, 1);

    pos.x = window_width / 2 - size.x / 2;
    pos.y = window_height / 2 - size.y / 2;

    DrawTextEx(font, text, pos, 24, 1, WHITE);

    for (i = 0; i < state.path_list.count; i++) {
        text = GetFileNameWithoutExt(state.path_list.paths[i]);

        text = TextFormat("[%s] %s", i < state.index ? "DONE" : "----", text);
        size = MeasureTextEx(font, text, 24, 1);

        pos.x = size.y;
        pos.y = size.y * (i + 1);

        DrawTextEx(font, text, pos, 24, 1, WHITE);
    }

    EndDrawing();
}
