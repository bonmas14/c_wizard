#define LPC_STATIC_DECL
#define LPC_ENC_DEC_IMPLEMENTATION
#include "lpc10_enc_dec.h" 
#include "blissful_orange.h" 

#define MAX_SAMPLES_UPDATE 512
#define SAMPLE_RATE 8000

#define PADDING_PX 10
#define FONT_SIZE 24

#define BACKGROUND_COLOR CLITERAL(Color) {0x1c, 0x1c, 0x1c, 0xff}

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

    SetWindowMinSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    GuiLoadStyleBlissfulOrange();
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
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
    Rectangle rect;
    s32 x, height, width;

    font = GetFontDefault();

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    switch (state.status) {
        case STATUS_IDLE: 
        {
            x = rect.x = window_width / 4;

            width  = rect.width  = window_width  /  2;
            height = rect.height = window_height / 15;
            rect.height -= PADDING_PX / 2;
            rect.y       = PADDING_PX / 2;

            rect.width = window_width;
            rect.x     = 0;
            GuiLabel(rect, "Pitch buffer settings");
            rect.x     = x;
            rect.width = width;

            rect.y += height;
            GuiSlider(rect, "Low-cut", TextFormat("%.0f", state.settings.pitch_low_cut), &state.settings.pitch_low_cut, 1.0f, 500.0f);
            rect.y += height;
            GuiSlider(rect, "High-cut", TextFormat("%.0f", state.settings.pitch_high_cut), &state.settings.pitch_high_cut, 100.0f, 1000.0f);
            rect.y += height;
            GuiSlider(rect, "Q-Factor", TextFormat("%.2f", state.settings.pitch_q_factor), &state.settings.pitch_q_factor, 0.01f, 8.0f);

            rect.y += height;
            rect.width = window_width;
            rect.x     = 0;
            GuiLabel(rect, "Ks processing buffer settings");
            rect.x     = x;
            rect.width = width;

            rect.y += height;
            GuiSlider(rect, "Low-cut", TextFormat("%.0f",  state.settings.processing_low_cut),  &state.settings.processing_low_cut, 1.0f, 500.0f);
            rect.y += height;
            GuiSlider(rect, "High-cut", TextFormat("%.0f", state.settings.processing_high_cut), &state.settings.processing_high_cut, 100.0f, 4000.0f);
            rect.y += height;
            GuiSlider(rect, "Q-Factor", TextFormat("%.2f", state.settings.processing_q_factor), &state.settings.processing_q_factor, 0.01f, 8.0f);
            rect.y += height;

            rect.y += height;
            GuiSlider(rect, "Unvoiced thresh.", TextFormat("%.2f",  state.settings.unvoiced_thresh),  &state.settings.unvoiced_thresh, -1.0f, 1.0f);
            rect.y += height;
            GuiSlider(rect, "Unvoiced RMS mult.", TextFormat("%.2f", state.settings.unvoiced_rms_multiply), &state.settings.unvoiced_rms_multiply, 0.0f, 8.0f);

            rect.y += height;
            GuiToggle(rect, "Pre Emphasis", (bool*)&state.settings.do_pre_emphasis);
            rect.y += height;
            GuiSlider(rect, "Alpha", TextFormat("%.6f", state.settings.pre_emphasis_alpha), &state.settings.pre_emphasis_alpha, -1.0f, 1.0f);

            rect.y    += height * 2;
            rect.width = window_width;
            rect.x     = 0;
            GuiLabel(rect, "Drag and drop files you need to convert");
        } break;
        case STATUS_CONVERTING: 
        {
            text = "---- PROCESSING ----";
            size = MeasureTextEx(font, text, 24, 1);
            pos.x = window_width / 2 - size.x / 2;
            pos.y = window_height / 2 - size.y / 2;

            DrawTextEx(font, text, pos, 24, 1, WHITE);

            for (i = 0; i < state.path_list.count; i++) {
                if (i < state.index) continue;

                text = GetFileNameWithoutExt(state.path_list.paths[i]);

                text = TextFormat("%s", text);
                size = MeasureTextEx(font, text, 24, 1);

                pos.x = size.y;
                pos.y = size.y * ((i - state.index) + 1);

                DrawTextEx(font, text, pos, 24, 1, WHITE);
            }
        } break;
    }

    EndDrawing();
}
