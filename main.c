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




int main(int argc,char * argv[])
{
    int ret = 0;
    int i = 0,num = 0;
	signed char buffer[1024];
    char sequence_buf[1024] = "";
    char adts_header_buf[ADTS_HEADER_SIZE];
    adts_header_info  adts_header_data;
    AudioTagHeader aac_config_data;
    AVCDecoderConfigurationRecord avc_config_data;
    

    char length_a,length_b;

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

    adts_header_data.profile_ObjectType = 1;  // aac_lc
    adts_header_data.adts_buffer_fullness = 0x7ff; // vbr

    while(1)
    {

        //read a tag
        if(fread(buffer,FLV_TAG_HEADER_SIZE,1,fp_sourcefile) != 1) {
        	printf("FLV file has been read : hrader !\n");
        	return 0;
        }

        ret = parse_flv_tag_header(buffer);
        if(ret < 0)
            return 0;

        // read flv tag data
        if(fread(flvdemux.flv_tag.data,flvdemux.flv_tag.date_size,1,fp_sourcefile) != 1) {
        	printf("FLV file has been read : data !\n");
        	return 0;
        }

        i = 0;
        if(flvdemux.enable_audio && (flvdemux.flv_tag.tag_type == FLV_TAG_TYPE_AUDIO))
        {
            aac_config_data.SoundFormat = (flvdemux.flv_tag.data[i] >> 4) & 0xf;
            aac_config_data.SoundRate = (flvdemux.flv_tag.data[i] >> 2) & 0x3;
            aac_config_data.SoundSize = (flvdemux.flv_tag.data[i] >> 1) & 0x1;
            aac_config_data.SoundType = flvdemux.flv_tag.data[i] & 0x1;
            aac_config_data.AACPacketType = flvdemux.flv_tag.data[++i];
            
            if((aac_config_data.SoundFormat == FLV_CODECID_AAC) && (aac_config_data.AACPacketType == 0x01))  // 0x00 Synchronous data, 0x01 raw data
            {
                // dynamic parameters
                switch (aac_config_data.SoundRate)
                {
                    case FLV_SAMPLERATE_44100HZ:
                        adts_header_data.sampling_frequency_index = 4;
                        break;
                    case FLV_SAMPLERATE_22050HZ:
                        adts_header_data.sampling_frequency_index = 7;
                        break;
                    case FLV_SAMPLERATE_11025HZ:
                        adts_header_data.sampling_frequency_index = 10;
                        break;
                    case FLV_SAMPLERATE_SPECIAL:
                        adts_header_data.sampling_frequency_index = 11; // 8kHz
                        break;
                    default:
                        adts_header_data.sampling_frequency_index = 4;
                        break;
                }

                switch (aac_config_data.SoundType)
                {
                    case FLV_MONO:
                        adts_header_data.channel_configuration = 1;
                        break;
                    case FLV_STEREO:
                        adts_header_data.channel_configuration = 2;
                        break;
                    default:
                        adts_header_data.channel_configuration = 2;
                        break;
                }

                adts_header_data.aac_frame_length = flvdemux.flv_tag.date_size + 5;
                adts_write_frame_header_buf(adts_header_buf,&adts_header_data);
                fwrite(adts_header_buf,7,1,fp_audio_destfile);
                fwrite(flvdemux.flv_tag.data+2,flvdemux.flv_tag.date_size-2,1,fp_audio_destfile);
            }
        }
        else if(flvdemux.enable_video && (flvdemux.flv_tag.tag_type == FLV_TAG_TYPE_VIDEO))
        {
            flvdemux.flv_tag.video_data.frame_type = flvdemux.flv_tag.data[i] >> 4;
            flvdemux.flv_tag.video_data.codec_id = flvdemux.flv_tag.data[i++] & 0xF;
            flvdemux.flv_tag.video_data.AVCPacketType = flvdemux.flv_tag.data[i++];
            flvdemux.flv_tag.video_data.CompositionTime = (flvdemux.flv_tag.data[i++] & 0xFF) << 16
                                                        | (flvdemux.flv_tag.data[i++] & 0xFF) << 8
                                                        | (flvdemux.flv_tag.data[i++] & 0xFF);
            // use i++ CompositionTime will error

            if(flvdemux.flv_tag.video_data.codec_id != FLV_CODECID_H264 )
            {
                printf("FLV format video coding is not avc, unable to properly parse !!! \n");
                return 0;
            }

            if(flvdemux.flv_tag.video_data.frame_type == 1 && 
                flvdemux.flv_tag.video_data.codec_id == 7 &&
                flvdemux.flv_tag.video_data.AVCPacketType == 0 &&
                flvdemux.flv_tag.video_data.CompositionTime == 0)
            {
                //AVC sequence header , sps and pps
                avc_config_data.configurationVersion = flvdemux.flv_tag.data[i++];
                avc_config_data.AVCProfileIndication = flvdemux.flv_tag.data[i++];
                avc_config_data.profile_compatibility = flvdemux.flv_tag.data[i++];
                avc_config_data.AVCLevelIndication = flvdemux.flv_tag.data[i++];
                avc_config_data.lengthSizeMinusOne = flvdemux.flv_tag.data[i++] & 0x3;

                avc_config_data.numOfSequenceParameterSets = flvdemux.flv_tag.data[i++] & 0x1f;
                for(num = 0;num < avc_config_data.numOfSequenceParameterSets;num++)
                {
                    length_a = flvdemux.flv_tag.data[i++];
                    length_b = flvdemux.flv_tag.data[i++];
                    avc_config_data.sequenceParameterSetLength = (length_a << 8 ) | length_b;
                    sequence_buf[0] = 0x00;
                    sequence_buf[1] = 0x00;
                    sequence_buf[2] = 0x00;
                    sequence_buf[3] = 0x01;
                    memcpy(sequence_buf+4,flvdemux.flv_tag.data+i,avc_config_data.sequenceParameterSetLength);
                    fwrite(sequence_buf,avc_config_data.sequenceParameterSetLength+4,1,fp_video_destfile);
                    i+=avc_config_data.sequenceParameterSetLength;
                }

                avc_config_data.numOfPictureParameterSets = flvdemux.flv_tag.data[i++] & 0xff;
                for(num = 0;num < avc_config_data.numOfPictureParameterSets;num++)
                {
                    length_a = flvdemux.flv_tag.data[i++];
                    length_b = flvdemux.flv_tag.data[i++];
                    avc_config_data.pictureParameterSetLength = (length_a << 8 ) | length_b;
                    sequence_buf[0] = 0x00;
                    sequence_buf[1] = 0x00;
                    sequence_buf[2] = 0x00;
                    sequence_buf[3] = 0x01;
                    memcpy(sequence_buf+4,flvdemux.flv_tag.data+i,avc_config_data.pictureParameterSetLength);
                    fwrite(sequence_buf,avc_config_data.pictureParameterSetLength+4,1,fp_video_destfile);
                    i+=avc_config_data.sequenceParameterSetLength;
                }
            }
            //else if(flvdemux.flv_tag.video_data.frame_type == 1 && 
            //    flvdemux.flv_tag.video_data.codec_id == 7)
            else
            {
                flvdemux.flv_tag.video_data.valid_data_size = (flvdemux.flv_tag.data[5] & 0xFF) << 24
                                                          | (flvdemux.flv_tag.data[6] & 0xFF) << 16
                                                          | (flvdemux.flv_tag.data[7] & 0xFF) << 8
                                                          | (flvdemux.flv_tag.data[8] & 0xFF);
                
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
        }
        else if(flvdemux.flv_tag.tag_type == FLV_TAG_TYPE_META)
        {
            do_tag_onMetaData(flvdemux.flv_tag.data,flvdemux.flv_tag.date_size);
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
