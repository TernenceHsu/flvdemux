/*

  Copyright (C) 2013 xubinbin Ðì±ò±ò (Beijing China)

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

  
  filename: fa_ts2es.c
  version : v1.1.0
  time    : 2013/08/02 16:20 
  author  : xubinbin ( xubbwd@gmail.com )
  code URL: http://code.google.com/p/tsdemux/
  
*/

#include "fa_flv2es.h"

int script_type_parse(unsigned char *data);

union av_intfloat64 {
    uint64_t i;
    double f;
};

enum script_data_type {
    Number = 0,
    Boolean,
    String,
    Object,
    MovieClip,
    Null,
    Undefined,
    Reference,
    EcmaArray,
    ObjectEndMarker,
    StringArray,
    Date,
    LongString,
};
typedef enum script_data_type script_type_t;

struct script_string {
    char len[2];
};
typedef struct script_string String_t;

struct flv_header {
    unsigned char tag_f;
    unsigned char tag_l;
    unsigned char tag_v;
    unsigned char version;
    unsigned char audio_and_video;
    unsigned char DataOffset[4];
};
typedef struct flv_header header_t;

struct flv_tag {
    unsigned char tagtype;
    unsigned char size[3];
    unsigned char timestamp[3];
    unsigned char extent_timestamp;
    unsigned char stream_id[3];
};
typedef struct flv_tag tag_t;

struct flv_body {
    tag_t tag;
    // unsigned char PreviousTagSize[2];
};
typedef struct flv_body body_t;

struct object_name {
    unsigned short length;
    unsigned char name;
};
typedef struct object_name object_name_t;

struct script_object {
    object_name_t name;
};
typedef struct script_object_name script_object_t;


/**
 * Reinterpret a 64-bit integer as a double.
 */
static double int2double(uint64_t i)
{
    union av_intfloat64 v;
    v.i = i;
    return v.f;
}

/*
 * get_header_audio check if the flv have audio stream
 * info, a byte with video and audio bit flag
 *
 * return 0 or 1
*/
int get_header_audio(char info)
{
    return (info & 0x4) >> 2;
}

/*
 * get_header_video check if the flv have video stream
 * info, a byte with video and audio bit flag
 *
 * return 0 or 1
*/
int get_header_video(char info)
{
    return info & 0x1;
}

/*
 * flv_header_parse just parse the FLV VERSION tag
 * fd, file describe handle
 * return flv version
*/
int flv_header_parse(int fd)
{
    int ret = 0;
    header_t header;

    memset(&header, 0, sizeof(header));
    ret = read(fd, &header, sizeof(header));
    if (ret <= 0) {
        fprintf(stderr, "read header error, %d\n", -EINVAL);
        return -EINVAL;
    }


    fprintf(stdout, "flv = [%c %c %c] version = [%x]\n"
            "TypeFlagsAudio = [%x], TypeFlagsVideo = [%x], DataOffset = [%x %x %x %x]\n",
            header.tag_f, header.tag_l, header.tag_v, header.version,
            get_header_audio(header.audio_and_video), get_header_video(header.audio_and_video),
            header.DataOffset[0], header.DataOffset[1], header.DataOffset[2], header.DataOffset[3]);

    return (int)header.version;
}

/*
 * get_string_len get the key-value value string length
 * data, the data will be parsed
 *
 * return the String length by int
*/
int get_string_len(unsigned char *data)
{

    unsigned char *p = data;
    unsigned char len_char[8];
    int len_int = 0;

    memset(len_char, 0, sizeof(len_char));
    snprintf((char *)len_char, sizeof(len_char), "0x%x%x", data[0], data[1]);
    p = len_char;

    len_int = strtoul((char *)p, NULL, 16);

    return len_int;
}

/*
 * get_bool_value get bool type value (key-value value)
 * data, the data will be parsed
 *
 * return bool value 0 or 1
*/
int get_bool_value(unsigned char *data)
{
   return *data;
}

/*
 * get_key_len the key-value key string length unit: four bytes
 * data, the data will be parsed
 *
 * return the key length by int
*/
int get_key_len(unsigned char *data)
{
    char len_char[16];
    char *p = len_char;

    memset(len_char, 0, sizeof(len_char));
    snprintf(len_char, sizeof(len_char), "0x%x%x%x%x", data[0], data[1], data[2], data[3]);

    return strtoul(p, NULL, 16);
}

int process_ecma_array_end(unsigned char *data)
{
    return 0;
}

/*
 * process_ecma_array The onMetaData sub type named ECMA Array
 * data, the data will be parsed
 *
 * return the offset of the data
*/
int process_ecma_array(unsigned char *data)
{
    int ecma_array_len = 0;
    int keyname_len = 0;
    unsigned char keyname[32];
    unsigned char *p = data;
    int i = 0;

    ecma_array_len = get_key_len(p);
    p += 4;

    for (i = 0; i < ecma_array_len; i++) {
        keyname_len = get_string_len(p);
        p += 2;
        memset(keyname, 0, sizeof(keyname));
        strncpy((char *)keyname, (const char *)p, keyname_len);
        fprintf(stdout, "keyname = [%s]\n", keyname);
        p += keyname_len;
        p += script_type_parse(p);
    }

    if (*p == 0 && *(p + 1) == 0 && *(p + 2) == 9) {
        p += 3;
    }
    return p - data;
}

/*
 * get_double get the double number by long long type
 * data, the data will be parsed
 *
 * return the parse out number of long long
*/
unsigned long long get_double(unsigned char *data)
{
    unsigned char double_number[64];
    unsigned char *p = double_number;

    memset(double_number, 0, sizeof(double_number));
    snprintf((char *)double_number, sizeof(double_number), "0x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x%0.2x",
             *data, *(data + 1), *(data + 2), *(data + 3), *(data + 4), *(data + 5), *(data + 6), *(data + 7));

    return strtoull((char *)p, NULL, 16);

}

/*
 * script_type_parse parse the script object type and parse the data
 * data, the data will be parsed
 *
 * return the data offset
*/
int script_type_parse(unsigned char *data)
{
    int ret = 0;
    unsigned char *p = data;
    unsigned char string_output[512];
    unsigned long long number = 0;
    double double_number = 0.0;

    switch (*p) {
    case Number:
        p++;
        number = get_double(p);
        double_number = int2double(number);
        fprintf(stdout, "number = [%.2f]\n", double_number);
        p += 8;
        break;

    case Boolean:
        p++;
        ret = get_bool_value(p);
        p++;
        break;

    case String:
        p++;
        memset(string_output, 0, sizeof(string_output));
        ret = get_string_len(p);
        p += 2;
        strncpy((char *)string_output, (const char *)p, ret);
        fprintf(stdout, "String = [%s]\n", string_output);
        p += ret;
        break;

    case Object:
        p++;
        break;

    case MovieClip:
        p++;
        break;

    case Null:
        p++;
        break;

    case Undefined:
        p++;
        break;

    case Reference:
        p++;
        break;

    case EcmaArray:
        p++;
        ret = process_ecma_array(p);
        p += ret;
        break;

    case ObjectEndMarker:
        p++;
        break;

    case StringArray:
        p++;
        break;

    case Date:
        p++;
        break;

    case LongString:
        p++;
        break;

    default:

        break;
    }
    return p - data;
}

/*
 * do_tag_onMetaData parse the onMetaData tags
 * fd, file describe handle
 * size, will read size of the data, unit: byte
 *
 * return offset of the data
*/
//int do_tag_onMetaData(int fd, int size)
int do_tag_onMetaData(char * meta_buf, int size)
{
    int ret = 0;
    unsigned char *p = NULL;
    unsigned char *onMetaData = meta_buf;

    p = onMetaData;

    printf("\n\n\n script MetaData parse start :\n");

    while (p) {
        if (p - onMetaData >= size) {
            break;
        }
        ret = script_type_parse(p);
        fprintf(stdout, "in while 1 offset = [%d]\n", p - onMetaData);
        p += ret;
    }

    printf("\n end script MetaData parse !!! \n");
    
    return p - onMetaData;
}




int adts_write_frame_header_buf(char * buf,adts_header_info  *adts_header_data)
{
    int i = 0;

    if (adts_header_data->adts_buffer_fullness > ADTS_MAX_FRAME_BYTES)
    {
        printf("ADTS frame size too large: %u (max %d)\n",adts_header_data->adts_buffer_fullness,ADTS_MAX_FRAME_BYTES);
        return -1;
    }

    buf[i++] = adts_header_data->syncword >> 4 & 0xff;
    buf[i++] = (adts_header_data->syncword & 0xf) << 4 |
                (adts_header_data->ID &0x1) << 3 | 
                (adts_header_data->layer &0x3) << 1 | 
                (adts_header_data->protection_absent &0x1);
    buf[i++] = (adts_header_data->profile_ObjectType & 0x3) << 6 |
                (adts_header_data->sampling_frequency_index &0xf) << 2 | 
                (adts_header_data->private_bit &0x1) << 1 | 
                (adts_header_data->channel_configuration &0x11)  >> 2;
    buf[i++] = ((adts_header_data->channel_configuration & 0x3) << 6 |
                adts_header_data->original_copy << 5 | 
                adts_header_data->home << 4 | 
                adts_header_data->copyright_identification_bit << 3 |
                adts_header_data->copyright_identification_start << 2 |
                adts_header_data->aac_frame_length >> 11 ) &0xff;
    buf[i++] = adts_header_data->aac_frame_length >> 3 & 0xff;
    buf[i++] = (adts_header_data->aac_frame_length << 5 |
                adts_header_data->adts_buffer_fullness >> 6) & 0xff;
    buf[i++] = (adts_header_data->adts_buffer_fullness << 2 |
                adts_header_data->number_of_raw_data_blocks_in_frame ) & 0xff;

    return 0;
}

