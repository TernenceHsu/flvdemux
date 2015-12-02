/*

  Copyright (C) 2013 xubinbin ���� (Beijing China)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  
  filename: fa_ts2es.h 
  version : v1.1.0
  time    : 2013/08/02 16:20
  author  : xubinbin ( ternence.hsu@foxmail.com )
  code URL: http://code.google.com/p/tsdemux/
  
*/

#ifndef	__FA_FLV2ES_H__
#define __FA_FLV2ES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#pragma pack (1)

#define DATA_MAX_FRAME_SIZE         (1000*1024)

typedef struct adts_header_info
{
    unsigned syncword                           : 12;
    unsigned ID                                 : 1;
    unsigned layer                              : 2;
    unsigned protection_absent                  : 1;
    unsigned profile_ObjectType                 : 2;
    unsigned sampling_frequency_index           : 4;
    unsigned private_bit                        : 1;
    unsigned channel_configuration              : 3;
    unsigned original_copy                      : 1;
    unsigned home                               : 1;

    unsigned copyright_identification_bit       : 1;
    unsigned copyright_identification_start     : 1;
    unsigned aac_frame_length                   : 13;
    unsigned adts_buffer_fullness               : 11;
    unsigned number_of_raw_data_blocks_in_frame : 2;
    
} adts_header_info;


typedef enum
{
    Linear_PCM_P            = 0,
    ADPCM                   = 1,
    MP3                     = 2,
    Linear_PCM_L            = 3,
    Nellymoser_16K_mono     = 4,
    Nellymoser_8K_mono      = 5,
    Nellymoser              = 6,
    G711A                   = 7,
    G711U                   = 8,
    reserved                = 9,
    AAC                     = 10,
    SPEEX                   = 11,
    MP3_8K                  = 14,
    Device_specific_sound   = 15,
} FLV_SOUND_FORMAT;

typedef enum
{
    TAG_AUDIO               = 8,
    TAG_VIDEO               = 9,
    TAG_SCRIPT              = 18,
} FLV_TAG_TYPE;


typedef struct flv_audio_data
{

}flv_audio_data;

typedef struct flv_video_data
{
    int frame_type;
    int codec_id;
    int AVCPacketType;
    int CompositionTime;
    int valid_data_size;
}flv_video_data;

typedef struct flv_tag_struc
{
    int tag_type;
    int date_size;
    int timestamp;
    int timestamp_extended;
    int stream_id;
    char data[DATA_MAX_FRAME_SIZE];
    int previous_tag_size;

    flv_video_data video_data;
    flv_audio_data audio_data;
    
}flv_tag_struc;


typedef struct flv_demux_struc
{
    int flv_version;
    int enable_audio;
    int enable_video;
    flv_tag_struc flv_tag;
}flv_demux_struc;


extern flv_demux_struc flvdemux;

//#pragma pack ()

 #endif

