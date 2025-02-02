/* This file is largely based on example code provided by the ffmpeg
 * project which can be retrieved from:
 * https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/demuxing_decoding.c
 *
 * All modifications to the original file were by Scott Kuhl.
 *
 */

/*
 * Copyright (c) 2012 Stefano Sabatini
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

#include <stdio.h>
#include <stdlib.h>

#include "kuhl-util.h"
#include "video.h"

#ifndef HAVE_FFMPEG

video_state* video_get_next_frame(video_state *state, const char *filename)
{
	msg(MSG_FATAL, "Library is not compiled against FFMpeg. This function won't work.");
	exit(EXIT_FAILURE);
}
void video_cleanup(video_state *state)
{
	msg(MSG_FATAL, "Library is not compiled against FFMpeg. This function won't work.");
	exit(EXIT_FAILURE);
}

#else

static int output_video_frame(video_state *state)
{
    if (state->frame->width != state->width || state->frame->height != state->height ||
        state->frame->format != state->pix_fmt) {
        /* To handle this change, one could call av_image_alloc again and
         * decode the following frames into another rawvideo file. */
        fprintf(stderr, "Error: Width, height and pixel format have to be "
                "constant in a rawvideo file, but the width, height or "
                "pixel format of the input video changed:\n"
                "old: width = %d, height = %d, format = %s\n"
                "new: width = %d, height = %d, format = %s\n",
                state->width, state->height, av_get_pix_fmt_name(state->pix_fmt),
                state->frame->width, state->frame->height,
                av_get_pix_fmt_name(state->frame->format));
        return -1;
    }

    /* Calculate a reasonable time value that the caller can
     * use to display frames at the correct speed. */
    uint64_t pts = state->frame->best_effort_timestamp;
    state->usec = av_rescale_q ( pts,  state->video_stream->time_base, AV_TIME_BASE_Q );

#if 0
    msg(MSG_INFO, "video_frame n:%d coded_n:%d usec:%"PRId64"\n",
           state->video_frame_count++, state->frame->coded_picture_number, state->usec);
#endif

    if(state->data == NULL)
	    state->data = (unsigned char*) malloc(state->width*state->height*3);

    /* Convert from whatever colorspace the video is into 8-bit RGB.

       TODO: It would be significantly more efficient to do
       this conversion in a shader program.
    */
    uint8_t *outData[] = { (uint8_t*) state->data };
    const int destStride[] = {3*state->width};
    sws_scale(state->sws_ctx, (const uint8_t**) state->frame->data, state->frame->linesize, 0, state->height, outData, destStride);

    /* The image we get from FFMPEG is flipped vertically (if
     * we look at the data and expect 0,0 to be in the lower
     * left and the first pixel of data corresponds to that
     * lower left corner. However, flipping it on the CPU is
     * considerably slower than doing it on the
     * GPU. Therefore, the following is commented out. */
    //kuhl_flip_texture_array(state->data, state->width, state->height, 3);

    /* Indicate to the caller that we have retrieved a new video frame */
    state->has_new_video_frame = 1;


    return 0;
}



static int video_decode_packet(video_state *state)
{
	int ret = 0;

	// submit the packet to the decoder
	ret = avcodec_send_packet(state->video_dec_ctx, &(state->pkt));
	if(ret < 0)
	{
		msg(MSG_ERROR, "Error submitting paket for decoding (%s)", av_err2str(ret));
		return ret;
	}

	// get all the available frames from the decoder
	while (ret >= 0) {
        ret = avcodec_receive_frame(state->video_dec_ctx, state->frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        // write the frame data to output file
        if (state->video_dec_ctx->codec->type == AVMEDIA_TYPE_VIDEO)
        {
	        //printf("video packet\n");
            ret = output_video_frame(state);
        }
        else
        {
	        //printf("audio packet\n");
	        //ret = output_audio_frame(frame);
        }

        av_frame_unref(state->frame);
        if (ret < 0)
            return ret;
	}

    return 0;

}


static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type, const char* filename)
{
	int ret, stream_index;
	AVStream *st;
	const AVCodec *dec = NULL;

	ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if (ret < 0) {
		fprintf(stderr, "Could not find %s stream in input file '%s'\n",
		        av_get_media_type_string(type), filename);
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
		if(!*dec_ctx) {
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

        /* Init the decoders */
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }

		*stream_idx = stream_index;
	}

	return 0;
}




static video_state* video_init(const char *filename)
{
	msg(MSG_DEBUG, "video_init()");
	video_state *ret = calloc(sizeof(video_state), 1);
	strncpy(ret->filename, filename, 1024);
	ret->filename[1023] = '\0';

	/* Initialize video_state struct */
	ret->width = 0;
	ret->height = 0;
	ret->aspectRatio = 0.0f;
	ret->data = NULL;

	ret->video_stream_idx = -1;
	ret->sws_ctx = NULL;
	ret->frame = NULL;
	ret->fmt_ctx = NULL;
	ret->video_dec_ctx = NULL;
	ret->video_stream = NULL;
	ret->video_frame_count = 0;

	//av_register_all();

	/* open input file, and allocate format context */
	if (avformat_open_input(&(ret->fmt_ctx), filename, NULL, NULL) < 0) {
		msg(MSG_ERROR, "Could not open source file '%s'", filename);
		return NULL;
	}

	/* retrieve stream information */
	if (avformat_find_stream_info(ret->fmt_ctx, NULL) < 0) {
		msg(MSG_ERROR, "Could not find stream information in '%s'", filename);
		return NULL;
	}

	if (open_codec_context(&(ret->video_stream_idx),
	                       &(ret->video_dec_ctx),
	                       ret->fmt_ctx, AVMEDIA_TYPE_VIDEO, ret->filename) >= 0) {
		ret->video_stream = ret->fmt_ctx->streams[ret->video_stream_idx];

		/* allocate image where the decoded image will be put */
		ret->width = ret->video_dec_ctx->width;
		ret->height = ret->video_dec_ctx->height;
		ret->aspectRatio = ret->width/(float)ret->height;
		ret->pix_fmt = ret->video_dec_ctx->pix_fmt;
	}

	// Dump input information to stderr
	av_dump_format(ret->fmt_ctx, 0, filename, 0);

	if(!ret->video_stream)
	{
		msg(MSG_ERROR, "Could not find video stream in input for '%s', aborting", filename);
		video_cleanup(ret);
		return NULL;
	}

	AVFrame *frame = av_frame_alloc();
	if(!frame)
	{
		msg(MSG_ERROR, "Could not allocate frame for video '%s'", filename);
		video_cleanup(ret);
		return NULL;
	}
	ret->frame = frame;

	/* Create a swscontext to convert colorspace to RGB */
	ret->sws_ctx = sws_getContext(ret->width, ret->height,
	                              ret->pix_fmt, ret->width, ret->height,
	                              AV_PIX_FMT_RGB24, 0, 0, 0, 0);

	av_init_packet(&(ret->pkt));
	ret->pkt.data = NULL;
	ret->pkt.size = 0;
	return ret;
}

void video_cleanup(video_state *state)
{
	msg(MSG_DEBUG, "video_cleanup()");
	avcodec_close(state->video_dec_ctx);
	avformat_close_input(&(state->fmt_ctx));
	av_frame_free(&(state->frame));
	free(state);
}


video_state* video_get_next_frame(video_state *state, const char *filename)
{
#define VIDEO_LOG_DECODE_TIME 0

	if(state == NULL)
		state = video_init(filename);
	if(state == NULL)
	{
		msg(MSG_ERROR, "Failed to get next frame of video from file %s\n", filename);
		return NULL;
	}

	//msg(MSG_DEBUG, "video_get_next_frame()");

	long startDecodeTime = kuhl_microseconds();
	state->has_new_video_frame = 0;

	/* read frames from the file */
	while(av_read_frame(state->fmt_ctx, &(state->pkt)) >= 0)
	{
		if(state->pkt.stream_index == state->video_stream_idx)
			video_decode_packet(state);
		av_packet_unref(&(state->pkt));

		if(state->has_new_video_frame)
		{
			if(VIDEO_LOG_DECODE_TIME)
			{
				long elapsedTime = kuhl_microseconds() - startDecodeTime;
				msg(MSG_DEBUG, "Video frame decode time: %ld microseconds", elapsedTime);
			}
			return state;
		}
	}

	msg(MSG_FATAL, "Didn't find frame, exiting");
	exit(EXIT_FAILURE);
	return state;
}

#endif // HAVE_FFMPEG
