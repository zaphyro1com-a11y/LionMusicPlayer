#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}

#include <string>
#include <vector>

struct PCMFrame {
    std::vector<uint8_t> data;  // buffer PCM
    int sampleRate;             // frecuencia de muestreo
    int channels;               // número de canales
    AVSampleFormat format;      // formato de muestra (ej. AV_SAMPLE_FMT_S16)
};

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // Inicializa FFmpeg y abre archivo
    bool openFile(const std::string& path);

    // Decodifica el siguiente frame PCM
    bool getNextFrame(PCMFrame& frame);

    // Duración total del archivo en segundos (0 si no se pudo determinar).
    // Válido después de un openFile() exitoso.
    int getDurationSeconds() const;

    // Cierra y libera recursos
    void close();

private:
    AVFormatContext* fmtCtx;
    AVCodecContext* codecCtx;
    SwrContext* swrCtx;
    int streamIndex;
    AVFrame* avFrame;
    AVPacket* avPacket;
};

#endif // AUDIO_DECODER_H