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

#define FLV_FILE_HEADER_SIZE    9
    
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


int main(int argc,char * argv[])
{

    int ret = 0;
	signed char buffer[1024];

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
            fwrite(flvdemux.flv_tag.data,flvdemux.flv_tag.date_size,1,fp_audio_destfile);
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
