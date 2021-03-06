/*

  Copyright (C) 2015 xubinbin ���� (Beijing China)

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

  
  filename: fa_parseopt.h 
  version : v1.1.0
  time    : 2015/12/04 16:20 
  author  : xubinbin ( ternence.hsu@foxmail.com )
  code URL: http://code.google.com/p/flvdemux/

*/

#ifndef _FA_GETOPT_H
#define _FA_GETOPT_H


/*
    Below macro maybe defined in fa_print, if you do not want to use fa_print,
    you can define any other printf functions which you like.  Moreover, if you
    dont want print anything (for the effienece purpose of the program), you 
    can also define null, and the program will print nothing when running.
*/
#ifndef FA_PRINT 
//#define FA_PRINT(...)       
#define FA_PRINT       printf
#define FA_PRINT_ERR   FA_PRINT
#define FA_PRINT_DBG   FA_PRINT
#endif


extern char  opt_inputfile[] ;
extern char  opt_video_outputfile[];
extern char  opt_audio_outputfile[];

int fa_parseopt(int argc, char *argv[]);

#endif
