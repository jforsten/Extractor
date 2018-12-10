// Extractor v0.4 - (c) Juha Forsten 29 Nov 2002 
// =============================================
//
// -----------------------------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -----------------------------------------------------------------------
// COMPILE:
//
// * Requires the libsnd -library!!
// 
// gcc extractor.c -L /usr/local/lib/ -lsndfile -o extractor 
//
// RUN:
//
// extractor 'input.wav' 'name_of_output(s)' 'Gain' 'Len_of_noise' 'Len_of_sample'
//
// Example:  extractor input_wave.wav wave 100 5000 100
//
//
//

#include        <stdio.h>
#include        <string.h>
#include        <ctype.h>
#include        <sndfile.h>

#define  BUFFER_LEN      1024

                                     
static void copy_data (SNDFILE *outfile, SNDFILE *infile, unsigned long start, unsigned long end)
{       
  static int    data [BUFFER_LEN];
  unsigned long len, bl;

  //  Find sample start
  sf_seek(infile, start, SEEK_SET);

  // Copy using buffer
  len = end - start;
  bl  = BUFFER_LEN / 2;
  while (len > bl) {
    sf_read_int (infile, data, BUFFER_LEN ) ;
    sf_write_int (outfile, data, BUFFER_LEN) ;
    len = len - bl;
  }

  sf_read_int (infile, data, len*2 ) ;
  sf_write_int (outfile, data, len*2) ;

  return;
  
} /* copy_data */   


///////////////////////////////
// MAIN 

int main (int argc, char *argv[])
{       
  char *infilename, *outfilename ;
  SNDFILE *infile, *outfile ;
  SF_INFO  sfinfo ;

  static  char    strbuffer [BUFFER_LEN] ; 

  unsigned long i;
  long gain, len_n, len_s, sample_begin, sample_end;
  long over, under, tmp_begin, tmp_end, sample_length;  
  int sample[2];

  unsigned int index, count;
  char s_index[5];
  char filename[80];

  tmp_begin = 0;
  tmp_end   = 0;
  index = 0;
  count = 0;
  
  // input file, output file, GAIN, Length of noise, length of sample
  if(argc != 6) {
    fprintf(stderr, "Usage: extractor 'input.wav' 'name_of_output(s)' 'Gain' 'Len_of_noise' 'Len_of_sample'\n\n");
    exit(1);
  }
  infilename  = argv [1] ;  
  outfilename = argv [2] ;  
  gain        = atol(argv[3]);
  len_n       = atol(argv[4]);
  len_s       = atol(argv[5]);

  printf("\nProcessing sample '%s'....\n", infilename);
  printf("Gain: %ld | Length of noise: %ld | Length of sample %ld\n\n", gain, len_n, len_s);

  // Open sound file
  if (! (infile = sf_open (infilename, SFM_READ,  &sfinfo))) {  
    printf ("Error : Not able to open input file %s.\n", infilename) ;
    sf_perror (NULL) ;
    //sf_get_header_info (NULL, strbuffer, BUFFER_LEN, 0) ;
    //printf (strbuffer) ;
    return  1 ;
  } 

  
  // Get length

  sample_length =  sfinfo.frames;

  printf ("Sample Rate : %d\n", sfinfo.samplerate) ;
  printf ("Frames      : %ld\n", (long) sfinfo.frames) ;
  printf ("Channels    : %d\n", sfinfo.channels) ;
  printf ("Format      : 0x%08X\n", sfinfo.format) ;
  printf ("Sections    : %d\n", sfinfo.sections) ;
  printf ("Seekable    : %s\n\n", (sfinfo.seekable ? "TRUE" : "FALSE")) ;
  //printf ("Duration    : %s\n", generate_duration_str (&sfinfo)) ;


  //printf("Total number of samples in input file: %ld\n\n", sample_length);


  sample_begin = 0;
  sample_end   = 0;
    

  // Extract samples until end

  while(1) {

    over         = 0;
    under        = 0;

    sf_seek(infile, sample_end, SEEK_SET);

    // Skip noice - find the sample start
    for (i = sample_end ; i < sample_length; i++) {
      count++;
      sf_read_int (infile, sample, 2);
      if ((abs(sample[0]) > gain) || (abs(sample[1]) > gain)){
	over++;
	if (over > len_s) {
	  sample_begin = tmp_begin;
	  break;
	}	
      } else {	
	over = 0;
	tmp_begin=i;
      }
    }
    
    over  = 0; under = 0;
    sample_end = sample_begin;
    
    // Sample start found - find sample end
    for ( i = sample_end; i < sample_length; i++) {
      count++;
      sf_read_int (infile, sample, 2);
      if ((abs(sample[0]) < gain) && (abs(sample[1]) < gain)){

	under++;	
	if (under > len_n) {
	  sample_end = tmp_end;
	  break;
	}	
      } else {
	under = 0;
	tmp_end = i;
      }
    }

    // If everything is processed, quit!
    if(count >= sample_length) break;

    printf("Sample found:  start=%ld, end=%ld\n",sample_begin,sample_end);
                                                                     
    sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16) ;
    
    // add number and extension to name
    strcpy(filename, outfilename);
    sprintf(s_index,"%d", index);
    strcat(filename, s_index);
    strcat(filename, ".wav");
    
    // Open output file
    if (! (outfile = sf_open (filename, SFM_WRITE, &sfinfo))) {
      printf ("ERROR: Can't open file %s.\n", outfilename) ;
      exit(1) ;
    }
    
    printf("Writing %s...\n\n", filename);
    copy_data (outfile, infile, sample_begin, sample_end) ;
    sf_close (outfile) ;   
    strcpy(filename,"");

    sample_end = sample_end + len_n;
    index++;


  }
  sf_close (infile) ;  
  printf("All done! Total of %d samples found!\n\n",index);
  exit(0);
}
  
  
