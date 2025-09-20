#define LPC_STATIC_DECL
#define LPC_ENC_DEC_IMPLEMENTATION
#include "lpc10_enc_dec.h" 

#define MAX_SAMPLES_UPDATE 512
#define SAMPLE_RATE 8000

// AudioStream audio_stream;

Lpc_Encoder_Settings lpc_settings;

void program_init(void) {
    // SetAudioStreamBufferSizeDefault(MAX_SAMPLES_UPDATE);
    //
    // audio_stream = LoadAudioStream(SAMPLE_RATE, 32, 1);
    //
    // if (IsAudioStreamValid(audio_stream)) {
    //     PlayAudioStream(audio_stream);
    //     SetAudioStreamVolume(audio_stream, 1.0f);
    // }

    lpc_settings = LPC_DEFAULT_SETTINGS; 
}

void program_deinit(void) {
    // if (IsAudioStreamValid(audio_stream)) {
    //     StopAudioStream(audio_stream);
    //     UnloadAudioStream(audio_stream);
    // }
}

s64 mouse_coord, mouse_power, index;

void program_update(void) {
    FilePathList paths;
    Lpc_Sample_Buffer samples;
    Lpc_TMS5220_Buffer buffer;
    Lpc_Codes codes;
    Wave wave;
    u64 i;

    if (IsFileDropped()) {
        paths = LoadDroppedFiles();

        // @todo, load all files and then store them at work dir
        for (i = 0; i < paths.count; i++) {
            wave = LoadWave(paths.paths[i]);
            if (!IsWaveValid(wave)) continue;

            WaveFormat(&wave, LPC_SAMPLE_RATE, 32, 1);

            samples.sample_rate = LPC_SAMPLE_RATE;
            samples.channels    = 1;
            samples.frame_count = wave.frameCount;
            samples.samples     = (f32*)wave.data;

            codes  = lpc_encode(samples, lpc_settings);
            buffer = lpc_tms5220_encode(codes);

            UnloadWave(wave);
            memset(&wave, 0, sizeof(Wave));

            samples         = lpc_decode(codes);
            wave.sampleRate = samples.sample_rate;
            wave.sampleSize = 32;
            wave.channels   = 1;
            wave.frameCount = samples.frame_count;
            wave.data       = (void*)samples.samples;

            ExportWave(wave, TextFormat("lpc10_%s.wav", GetFileNameWithoutExt(paths.paths[i])));
            ExportDataAsCode(buffer.bytes, buffer.count, TextFormat("lpc10_%s.h", GetFileNameWithoutExt(paths.paths[i])));

            lpc_codes_free(&codes);
            lpc_buffer_free(&samples);
            lpc_tms5220_buffer_free(&buffer);
        }
    
        UnloadDroppedFiles(paths);
    }

    // if (IsAudioStreamProcessed(audio_stream)) {
    //     UpdateAudioStream(audio_stream,
    //             audio_buffer_read(),
    //             MAX_SAMPLES_UPDATE);
    //     time_pos = time;
    // } else {
    //     time_pos += GetFrameTime() * SAMPLE_RATE;
    // }
}

void program_render(void) {
    Font font;
    Vector2 pos, size;
    char *text;
    BeginDrawing();
    ClearBackground(CLITERAL(Color) {0x1c, 0x1c, 0x1c, 0xff});

    text = ">> drag and drop files here <<";
    font = GetFontDefault();

    size = MeasureTextEx(font, text, 24, 1);

    pos.x = window_width / 2 - size.x / 2;
    pos.y = window_height / 2 - size.y / 2;

    DrawTextEx(font, text, pos, 24, 1, WHITE);

    EndDrawing();
}
