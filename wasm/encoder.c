/*
 * Minimal WebAssembly wrapper for libopusenc
 * Uses pull-based API for client-side encoding
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "../include/opusenc.h"

typedef struct {
    OggOpusEnc *enc;
    unsigned char *output_buffer;
    int output_size;
    int output_capacity;
} EncoderContext;

EMSCRIPTEN_KEEPALIVE
EncoderContext* encoder_create(int sample_rate, int channels) {
    int error;
    
    EncoderContext *ctx = (EncoderContext*)malloc(sizeof(EncoderContext));
    if (!ctx) return NULL;
    
    // Create comments
    OggOpusComments *comments = ope_comments_create();
    ope_comments_add(comments, "ENCODER", "libopusenc-wasm");
    
    // Create pull-based encoder (no file I/O)
    ctx->enc = ope_encoder_create_pull(comments, sample_rate, channels, 0, &error);
    ope_comments_destroy(comments);
    
    if (!ctx->enc) {
        free(ctx);
        return NULL;
    }
    
    // Set sensible defaults: 128kbps, medium complexity
    ope_encoder_ctl(ctx->enc, OPUS_SET_BITRATE(128000));
    ope_encoder_ctl(ctx->enc, OPUS_SET_COMPLEXITY(5));
    
    // Initialize output buffer (grows as needed)
    ctx->output_capacity = 1024 * 1024; // Start with 1MB
    ctx->output_buffer = (unsigned char*)malloc(ctx->output_capacity);
    ctx->output_size = 0;
    
    if (!ctx->output_buffer) {
        ope_encoder_destroy(ctx->enc);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

EMSCRIPTEN_KEEPALIVE
int encoder_write(EncoderContext *ctx, float *pcm, int samples) {
    if (!ctx || !ctx->enc) return -1;
    
    return ope_encoder_write_float(ctx->enc, pcm, samples);
}

EMSCRIPTEN_KEEPALIVE
int encoder_get_page(EncoderContext *ctx) {
    if (!ctx || !ctx->enc) return -1;
    
    unsigned char *page_data = NULL;
    opus_int32 len = 0;
    
    // Get next Ogg page (returns pointer to page, not copy)
    int ret = ope_encoder_get_page(ctx->enc, &page_data, &len, 0);
    
    if (ret == 0) return 0;  // No page available yet
    if (ret < 0) return -1;  // Error
    
    // Ensure buffer capacity
    if (ctx->output_size + len > ctx->output_capacity) {
        ctx->output_capacity = (ctx->output_size + len) * 2;
        unsigned char *new_buf = (unsigned char*)realloc(ctx->output_buffer, ctx->output_capacity);
        if (!new_buf) return -1;
        ctx->output_buffer = new_buf;
    }
    
    // Append page to output buffer
    memcpy(ctx->output_buffer + ctx->output_size, page_data, len);
    ctx->output_size += len;
    
    return len;
}

EMSCRIPTEN_KEEPALIVE
int encoder_drain(EncoderContext *ctx) {
    if (!ctx || !ctx->enc) return -1;
    
    int ret = ope_encoder_drain(ctx->enc);
    if (ret != OPE_OK) return ret;
    
    // Get all remaining pages
    unsigned char *page_data = NULL;
    opus_int32 len = 0;
    
    while (ope_encoder_get_page(ctx->enc, &page_data, &len, 1) > 0) {
        // Ensure buffer capacity
        if (ctx->output_size + len > ctx->output_capacity) {
            ctx->output_capacity = (ctx->output_size + len) * 2;
            unsigned char *new_buf = (unsigned char*)realloc(ctx->output_buffer, ctx->output_capacity);
            if (!new_buf) return -1;
            ctx->output_buffer = new_buf;
        }
        
        // Append page
        memcpy(ctx->output_buffer + ctx->output_size, page_data, len);
        ctx->output_size += len;
    }
    
    return OPE_OK;
}

EMSCRIPTEN_KEEPALIVE
unsigned char* encoder_get_data(EncoderContext *ctx) {
    if (!ctx) return NULL;
    return ctx->output_buffer;
}

EMSCRIPTEN_KEEPALIVE
int encoder_get_size(EncoderContext *ctx) {
    if (!ctx) return 0;
    return ctx->output_size;
}

EMSCRIPTEN_KEEPALIVE
void encoder_destroy(EncoderContext *ctx) {
    if (!ctx) return;
    
    if (ctx->enc) {
        ope_encoder_destroy(ctx->enc);
    }
    if (ctx->output_buffer) {
        free(ctx->output_buffer);
    }
    free(ctx);
}
