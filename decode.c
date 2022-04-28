 /*
  * Copyright (c) 2001 Fabrice Bellard
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  * THE SOFTWARE.
  */
  
 /**
  * @file
  * video decoding with libavcodec API example
  *
  * @example decode_video.c
  */
  
#include "decode.h"
#include "common.h"  

 
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = NULL;

static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize;
 
static int video_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket *pkt = NULL;



/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
static int refcount = 0;
int ret = 0, got_frame;

void output_video_frame(AVFrame *frame)
{
	if (frame)
	{
		glBindTexture(GL_TEXTURE_2D, 2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->width, frame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[0]);
		glBindTexture(GL_TEXTURE_2D, 3);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->width/2, frame->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[1]);
    	glBindTexture(GL_TEXTURE_2D, 4);
   		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame->width/2, frame->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->data[2]);			   
	}
}

static int decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
    int ret = 0;
 
    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }
 
    // get all the available frames from the decoder
    if (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;
 
            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }
 
        // write the frame data to output file
        if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
            output_video_frame(frame);
    }
 
    return 0;
}
 
int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream'\n",
                av_get_media_type_string(type));
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];
        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }
        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }
        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }
        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }
    return 0;
}

int open_video_to_decode(const char *filename)
{

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return -1;
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }
    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];

        /* allocate image where the decoded image will be put */
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             width, height, pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            close_video();
            return -1;
        }
        video_dst_bufsize = ret;
    }
    else{
        fprintf(stderr, "No video stream available\n");
        close_video();
        return -1;
    }
 
     /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, filename, 0);
 
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        close_video();
        return -1;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate packet\n");
        close_video();
        return -1;
    }
    return 0;
 }

void read_file_to_decode()
{
    /* read frames from the file */
    if (av_read_frame(fmt_ctx, pkt) >= 0) {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        if (pkt->stream_index == video_stream_idx)
        {
            ret = decode_packet(video_dec_ctx, pkt);

        }
        av_packet_unref(pkt);
    }
    else
    {
        avformat_seek_file(fmt_ctx, video_stream_idx, 0,0,0,AVSEEK_FLAG_BYTE|AVSEEK_FLAG_ANY);
        avformat_seek_file(fmt_ctx, video_stream_idx, 0,0,0,AVSEEK_FLAG_ANY|AVSEEK_FLAG_FRAME);
    }
}

int reopen_video()
{
 
    decode_packet(video_dec_ctx, NULL);
    avcodec_free_context(&video_dec_ctx);

 
    avformat_close_input(&fmt_ctx);

    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_free(video_dst_data[0]);
 
    return 0;
}

int close_video()
{
    decode_packet(video_dec_ctx, NULL);
    avcodec_free_context(&video_dec_ctx);

    avformat_close_input(&fmt_ctx);

    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_free(video_dst_data[0]);
 
    return 0;
}
