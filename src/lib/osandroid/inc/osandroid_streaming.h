/*
Copyright (c) 2023, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OSANDROID_STREAMING_H_INCLUDED
#define OSANDROID_STREAMING_H_INCLUDED

typedef enum
{
    STREAMING_STATE_NONE = 0,
    STREAMING_STATE_STOPPED = 1,
    STREAMING_STATE_PAUSED = 2,
    STREAMING_STATE_PLAYING = 3,
    STREAMING_STATE_FAST_FORWARDING = 4,
    STREAMING_STATE_REWINDING = 5,
    STREAMING_STATE_BUFFERING = 6,
    STREAMING_STATE_ERROR = 7,
    STREAMING_STATE_CONNECTING = 8,
    STREAMING_STATE_SKIPPING_TO_PREVIOUS = 9,
    STREAMING_STATE_SKIPPING_TO_NEXT = 10,
    STREAMING_STATE_SKIPPING_TO_QUEUE_ITEM = 11,
    STREAMING_STATE_PTSJUMP = 12,
} streaming_playback_state_e;

typedef struct streaming_playback_info
{
    /* Playback duration (ms) */
    uint64_t duration;
    /* Volume 1 ~ 100% */
    uint8_t volume;
    /* Video playback speed, eg: 1.25 */
    float play_speed;
    streaming_playback_state_e state;
} streaming_playback_info_t;

typedef struct streaming_video_info
{
    /* Title/Channel */
    char title_channel[1024];
    /* Total duration (ms) */
    uint64_t total_duration;
    /* Video Resolution */
    uint32_t resol_width;
    uint32_t resol_height;
    /* Video frame rate */
    uint32_t frame_rate;
    /* Frames Count */
    uint64_t frames_count;
    /* Dropped Frames */
    uint64_t dropped_frames;
    /* Error Frames */
    uint64_t error_frames;
    /* Render Time (ms) */
    uint32_t render_time;
    /* Video & Audio bit rate */
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;
    /* Audio Sample Rate */
    uint32_t sample_rate_hz;
    /* Codec */
    char codec[64];
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    /* Time (ms) between the play request and the stream starts (exclude pre-roll time) */
    uint64_t start_time;
    /* Connection Speed Bit per Second */
    uint64_t connection_speed;
    /* Video Buffering time (ms) */
    uint64_t buffering_time;
} streaming_video_info_t;

typedef struct streaming_info
{
    char app_name[512];
    streaming_video_info_t video_info;
    streaming_playback_info_t playback_info;
    char error[512];
} streaming_info_t;

bool osandroid_streaming_get(streaming_info_t *info);
bool osandroid_streaming_json_parse(streaming_info_t *info, const char *rep_buf);

#endif /* OSANDROID_STREAMING_H_INCLUDED */
