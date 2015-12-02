/*

  Copyright (C) 2013 xubinbin 徐彬彬 (Beijing China)

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

  
  filename: main.c 
  version : v1.1.0
  time    : 2015/09/21 16:20 
  author  : xubinbin ( ternence.hsu@foxmail.com )

*/

#include "fa_parseopt.h"
#include "fa_flv2es.h"

#define FLV_TAG_HEADER_SIZE             (11)
#define ADTS_HEADER_SIZE                (7)
#define FLV_FILE_HEADER_SIZE            (9)
#define ADTS_MAX_FRAME_BYTES            ((1 << 13) - 1)

flv_demux_struc flvdemux;

int flv_demux_init(flv_demux_struc *flvdemux)
{	
	memset(flvdemux, 0, sizeof(struct flv_demux_struc));

	flvdemux->enable_audio = 0;
	flvdemux->enable_video = 0;

	return 0;
}

int flv_demux_deinit(flv_demux_struc *flvdemux)
{
    memset(flvdemux, 0, sizeof(struct flv_demux_struc));
	return 0;
}


int parse_flv_file_header(char * header_buf)
{


    
    int header_length;

    if(header_buf[0] != 'F' || header_buf[1] != 'L' || header_buf[2] != 'V')
    {
        printf("This is not a FLV file !!!\n");
        return -1;
    }

    flvdemux.flv_version = header_buf[3];
    flvdemux.enable_audio = header_buf[4] & 0x4;
    flvdemux.enable_video = header_buf[4] & 0x1;

    header_length = (header_buf[5] & 0xFF) << 24
                  | (header_buf[6] & 0xFF) << 16
                  | (header_buf[7] & 0xFF) << 8
                  | (header_buf[8] & 0xFF);

    // 有时候 flv 头信息后面有其他的信息，这个值就不是 9 了
    if(header_length != FLV_FILE_HEADER_SIZE)
    {
        printf("FLV file header information , length error !!!\n");
        return -1;
    }

    return 0;
}

int parse_flv_tag_header(char * header_buf)
{
    flvdemux.flv_tag.tag_type = header_buf[0];
    flvdemux.flv_tag.date_size  = (header_buf[1] & 0xFF) << 16
                                | (header_buf[2] & 0xFF) << 8
                                | (header_buf[3] & 0xFF);
    flvdemux.flv_tag.timestamp = (header_buf[4] & 0xFF) << 16
                               | (header_buf[5] & 0xFF) << 8
                               | (header_buf[6] & 0xFF);
    flvdemux.flv_tag.timestamp_extended = header_buf[7];
    flvdemux.flv_tag.stream_id = (header_buf[8] & 0xFF) << 16
                               | (header_buf[9] & 0xFF) << 8
                               | (header_buf[10] & 0xFF);

    if(flvdemux.flv_tag.stream_id != 0)
    {
        printf("The value of the StreamID should be 0 !!!\n");
    }
    return 0;
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

int main(int argc,char * argv[])
{
    int ret = 0;
	signed char buffer[1024];
    adts_header_info  adts_header_data;
    char adts_header_buf[ADTS_HEADER_SIZE];

	FILE * fp_sourcefile = NULL;
	FILE * fp_video_destfile = NULL;
	FILE * fp_audio_destfile = NULL;

    ret = fa_parseopt(argc, argv);
    if(ret) return -1;

	if ((fp_sourcefile = fopen(opt_inputfile, "rb")) == NULL) {
		printf("input file can not be opened;\n");
		return 0;
    }
	if(strlen(opt_video_outputfile) > 0) {
		if ((fp_video_destfile = fopen(opt_video_outputfile, "w+b")) == NULL) {
			printf("video output file can not be opened\n");
			return 0;
		}
	}
	if(strlen(opt_audio_outputfile) > 0) {
		if ((fp_audio_destfile = fopen(opt_audio_outputfile, "w+b")) == NULL) {
			printf("audio output file can not be opened\n");
			return 0;
		}
	}

	flv_demux_init(&flvdemux);

    if(fread(buffer,9,1,fp_sourcefile) != 1) {
    	printf("read inputfile error 001 !\n");
    	return 0;
    }

    ret = parse_flv_file_header(buffer);
    if(ret < 0)
        return(0);

    // PreviousTagSize0 UI32 Always 0
    if(fread(buffer,4,1,fp_sourcefile) != 1) {
    	printf("read inputfile error 002 !\n");
    	return 0;
    }


    // init adts_header_info
    adts_header_data.syncword = 0xfff;
    adts_header_data.ID = 0;
    adts_header_data.layer = 0;
    adts_header_data.protection_absent = 1;

    adts_header_data.private_bit = 0;
    adts_header_data.original_copy = 0;
    adts_header_data.home = 0;
    adts_header_data.copyright_identification_bit = 0;
    adts_header_data.copyright_identification_start = 0;
    adts_header_data.number_of_raw_data_blocks_in_frame = 0;

    //The dynamic parameters
    adts_header_data.profile_ObjectType = 1;
    adts_header_data.sampling_frequency_index = 4;
    adts_header_data.channel_configuration = 2;
    adts_header_data.adts_buffer_fullness = 0x7ff;

    while(1)
    {

        //read a tag
        if(fread(buffer,FLV_TAG_HEADER_SIZE,1,fp_sourcefile) != 1) {
        	printf("read inputfile error 003 !\n");
        	return 0;
        }

        ret = parse_flv_tag_header(buffer);
        if(ret < 0)
            return 0;
        
        //printf("tag_type = %d\n",flvdemux.flv_tag.tag_type);
        //printf("date_size = %d\n",flvdemux.flv_tag.date_size);

        // read flv tag data
        if(fread(flvdemux.flv_tag.data,flvdemux.flv_tag.date_size,1,fp_sourcefile) != 1) {
        	printf("read inputfile error 004 !\n");
        	return 0;
        }

        if(flvdemux.enable_audio && (flvdemux.flv_tag.tag_type == TAG_AUDIO))
        {
            if(flvdemux.flv_tag.data[1] == 0x01)  // 0x00 Synchronous data, 0x01 raw data
            {
                adts_header_data.aac_frame_length = flvdemux.flv_tag.date_size + 5;
                adts_write_frame_header_buf(adts_header_buf,&adts_header_data);
                fwrite(adts_header_buf,7,1,fp_audio_destfile);
                fwrite(flvdemux.flv_tag.data+2,flvdemux.flv_tag.date_size-2,1,fp_audio_destfile);
            }
        }
        else if(flvdemux.enable_video && (flvdemux.flv_tag.tag_type == TAG_VIDEO))
        {
            flvdemux.flv_tag.video_data.frame_type = flvdemux.flv_tag.data[0] >> 4;
            flvdemux.flv_tag.video_data.codec_id = flvdemux.flv_tag.data[0] & 0xF;
            flvdemux.flv_tag.video_data.AVCPacketType = flvdemux.flv_tag.data[1];
            flvdemux.flv_tag.video_data.CompositionTime = (flvdemux.flv_tag.data[2] & 0xFF) << 16
                                                        | (flvdemux.flv_tag.data[3] & 0xFF) << 8
                                                        | (flvdemux.flv_tag.data[4] & 0xFF);

            flvdemux.flv_tag.video_data.valid_data_size = (flvdemux.flv_tag.data[5] & 0xFF) << 24
                                                      | (flvdemux.flv_tag.data[6] & 0xFF) << 16
                                                      | (flvdemux.flv_tag.data[7] & 0xFF) << 8
                                                      | (flvdemux.flv_tag.data[8] & 0xFF);


            if(flvdemux.flv_tag.video_data.codec_id != 0x07 )
            {
                printf("not h264 data \n");
                return 0;
            }

            if(flvdemux.flv_tag.video_data.frame_type == 1 && 
                flvdemux.flv_tag.video_data.codec_id == 7 &&
                flvdemux.flv_tag.video_data.AVCPacketType == 0 &&
                flvdemux.flv_tag.video_data.CompositionTime == 0)
            {
                printf("test ####\n");


            }

            if(flvdemux.flv_tag.video_data.valid_data_size > 1000*1024){
                fwrite(flvdemux.flv_tag.data,flvdemux.flv_tag.date_size,1,fp_video_destfile);
            }else{
                flvdemux.flv_tag.data[5] = 0x00;
                flvdemux.flv_tag.data[6] = 0x00;
                flvdemux.flv_tag.data[7] = 0x00;
                flvdemux.flv_tag.data[8] = 0x01;
                fwrite(flvdemux.flv_tag.data+5,flvdemux.flv_tag.video_data.valid_data_size+4,1,fp_video_destfile);
            }
            
        }
        else if(flvdemux.flv_tag.tag_type == TAG_SCRIPT)
        {



        }

        // check previous tag size
        if(fread(buffer,4,1,fp_sourcefile) != 1) {
        	printf("read inputfile error 005 !\n");
        	return 0;
        }

        flvdemux.flv_tag.previous_tag_size = (buffer[0] & 0xFF) << 24
                                          | (buffer[1] & 0xFF) << 16
                                          | (buffer[2] & 0xFF) << 8
                                          | (buffer[3] & 0xFF);

        if(flvdemux.flv_tag.previous_tag_size != flvdemux.flv_tag.date_size + FLV_TAG_HEADER_SIZE)
        {
            printf("Check size of previous tag failure !!!");
            break;
        }

    }

    fclose(fp_sourcefile);
    if(strlen(opt_video_outputfile) > 0) {
    	fclose(fp_video_destfile);
    }
    if(strlen(opt_audio_outputfile) > 0) {
        fclose(fp_audio_destfile);
    }

	flv_demux_deinit(&flvdemux);

    printf("Done.\n");

    return 0;
}
