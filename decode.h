#ifndef _DECODE_H
#define _DECODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#define INBUF_SIZE 4096

int open_codec_context(int *stream_idx,AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);
int open_video_to_decode(const char *filename);
void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt);
void read_file_to_decode();
int close_video();
int reopen_video();

#endif /* _DECODE_H */
