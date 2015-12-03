/*

  Copyright (C) 2015 xubinbin Ðì±ò±ò (Beijing China)

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
  time    : 2015/12/03 16:20
  author  : xubinbin ( ternence.hsu@foxmail.com )
  code URL: http://code.google.com/p/flvdemux/
  
*/

#ifndef	__FA_FLV2ES_H__
#define __FA_FLV2ES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define DATA_MAX_FRAME_SIZE         (1000*1024)
#define FLV_TAG_HEADER_SIZE             (11)
#define ADTS_HEADER_SIZE                (7)
#define FLV_FILE_HEADER_SIZE            (9)
#define ADTS_MAX_FRAME_BYTES            ((1 << 13) - 1)

typedef unsigned long long          uint64_t;

enum {
    FLV_HEADER_FLAG_HASVIDEO = 1,
    FLV_HEADER_FLAG_HASAUDIO = 4,
};

enum {
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};

enum {
    FLV_STREAM_TYPE_VIDEO,
    FLV_STREAM_TYPE_AUDIO,
    FLV_STREAM_TYPE_DATA,
    FLV_STREAM_TYPE_NB,
};

enum {
    FLV_MONO   = 0,
    FLV_STEREO = 1,
};

enum {
    FLV_SAMPLESSIZE_8BIT  = 0,
    FLV_SAMPLESSIZE_16BIT = 1,
};

enum {
    FLV_SAMPLERATE_SPECIAL = 0, /**< signifies 5512Hz and 8000Hz in the case of NELLYMOSER */
    FLV_SAMPLERATE_11025HZ = 1,
    FLV_SAMPLERATE_22050HZ = 2,
    FLV_SAMPLERATE_44100HZ = 3,
};

enum {
    FLV_CODECID_PCM                  = 0,
    FLV_CODECID_ADPCM                = 1,
    FLV_CODECID_MP3                  = 2,
    FLV_CODECID_PCM_LE               = 3,
    FLV_CODECID_NELLYMOSER_16KHZ_MONO = 4,
    FLV_CODECID_NELLYMOSER_8KHZ_MONO = 5,
    FLV_CODECID_NELLYMOSER           = 6,
    FLV_CODECID_PCM_ALAW             = 7,
    FLV_CODECID_PCM_MULAW            = 8,
    FLV_CODECID_AAC                  = 10,
    FLV_CODECID_SPEEX                = 11,
};

enum {
    FLV_CODECID_H263    = 2,
    FLV_CODECID_SCREEN  = 3,
    FLV_CODECID_VP6     = 4,
    FLV_CODECID_VP6A    = 5,
    FLV_CODECID_SCREEN2 = 6,
    FLV_CODECID_H264    = 7,
    FLV_CODECID_REALH263= 8,
    FLV_CODECID_MPEG4   = 9,
};

enum {
    FLV_FRAME_KEY            = 1, ///< key frame (for AVC, a seekable frame)
    FLV_FRAME_INTER          = 2, ///< inter frame (for AVC, a non-seekable frame)
    FLV_FRAME_DISP_INTER     = 3, ///< disposable inter frame (H.263 only)
    FLV_FRAME_GENERATED_KEY  = 4, ///< generated key frame (reserved for server use only)
    FLV_FRAME_VIDEO_INFO_CMD = 5, ///< video info/command frame
};

typedef enum {
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
} AMFDataType;

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


typedef struct AudioTagHeader
{
    int SoundFormat;
    int SoundRate;
    int SoundSize;
    int SoundType;
    int AACPacketType;
}AudioTagHeader;

typedef struct flv_video_data
{
    int frame_type;
    int codec_id;
    int AVCPacketType;
    int CompositionTime;
    int valid_data_size;
}flv_video_data;


/*
 * In the ISO/IEC 14496-15 avc encapsulation are defined
 * AVC decoder configuration record
 */
typedef struct AVCDecoderConfigurationRecord
{
    int configurationVersion;
    int AVCProfileIndication;
    int profile_compatibility;
    int AVCLevelIndication;
    int lengthSizeMinusOne;
    int numOfSequenceParameterSets;
    int sequenceParameterSetLength;
    int numOfPictureParameterSets;
    int pictureParameterSetLength;
}AVCDecoderConfigurationRecord;



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
    AudioTagHeader audio_data;
    
}flv_tag_struc;


typedef struct flv_demux_struc
{
    int flv_version;
    int enable_audio;
    int enable_video;
    flv_tag_struc flv_tag;
}flv_demux_struc;


extern flv_demux_struc flvdemux;

int do_tag_onMetaData(char * meta_buf, int size);
int adts_write_frame_header_buf(char * buf,adts_header_info  *adts_header_data);

#endif
