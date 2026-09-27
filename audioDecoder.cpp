#include "audioDecoder.h"
#include <iostream>

AudioDecoder::AudioDecoder() {
    fmtCtx = nullptr;
    codecCtx = nullptr;
    swrCtx = nullptr;
    streamIndex = -1;
    avFrame = av_frame_alloc();
    avPacket = av_packet_alloc();
}

AudioDecoder::~AudioDecoder() {
    close();
    if (avFrame) av_frame_free(&avFrame);
    if (avPacket) av_packet_free(&avPacket);
}

bool AudioDecoder::openFile(const std::string& path) {
    if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Error: no se pudo abrir archivo " << path << std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        std::cerr << "Error: no se pudo leer información del stream" << std::endl;
        return false;
    }

    streamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (streamIndex < 0) {
        std::cerr << "Error: no se encontró stream de audio" << std::endl;
        return false;
    }

    AVCodecParameters* codecPar = fmtCtx->streams[streamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecPar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Error: no se pudo abrir codec" << std::endl;
        return false;
    }

    // Inicializar conversión a PCM S16 usando API moderna
    if (swr_alloc_set_opts2(&swrCtx,
        &codecCtx->ch_layout,        // salida
        AV_SAMPLE_FMT_S16,
        codecCtx->sample_rate,
        &codecCtx->ch_layout,        // entrada
        codecCtx->sample_fmt,
        codecCtx->sample_rate,
        0, nullptr) < 0) {
        std::cerr << "Error: no se pudo inicializar SwrContext" << std::endl;
        return false;
    }

    // swr_alloc_set_opts2 solo configura las opciones, todavía falta
    // inicializar de verdad el contexto antes de poder usar swr_convert
    if (swr_init(swrCtx) < 0) {
        std::cerr << "Error: no se pudo abrir SwrContext (swr_init)" << std::endl;
        return false;
    }

    return true;
}

bool AudioDecoder::getNextFrame(PCMFrame& frame) {
    while (av_read_frame(fmtCtx, avPacket) >= 0) {
        if (avPacket->stream_index == streamIndex) {
            if (avcodec_send_packet(codecCtx, avPacket) == 0) {
                while (avcodec_receive_frame(codecCtx, avFrame) == 0) {
                    // Convertir a PCM S16
                    int outSamples = swr_get_out_samples(swrCtx, avFrame->nb_samples);
                    int bufferSize = av_samples_get_buffer_size(nullptr,
                        codecCtx->ch_layout.nb_channels,
                        outSamples,
                        AV_SAMPLE_FMT_S16,
                        1);

                    frame.data.resize(bufferSize);
                    uint8_t* outArr[1] = { frame.data.data() };

                    swr_convert(swrCtx,
                        outArr,
                        outSamples,
                        (const uint8_t**)avFrame->data,
                        avFrame->nb_samples);

                    frame.sampleRate = codecCtx->sample_rate;
                    frame.channels = codecCtx->ch_layout.nb_channels;
                    frame.format = AV_SAMPLE_FMT_S16;

                    av_packet_unref(avPacket);
                    return true;
                }
            }
        }
        av_packet_unref(avPacket);
    }
    return false; // no más frames
}

int AudioDecoder::getDurationSeconds() const {
    if (!fmtCtx || fmtCtx->duration <= 0) return 0;
    return (int)(fmtCtx->duration / AV_TIME_BASE);
}

void AudioDecoder::close() {
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (fmtCtx) {
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
    }
    if (swrCtx) {
        swr_free(&swrCtx);
        swrCtx = nullptr;
    }
}