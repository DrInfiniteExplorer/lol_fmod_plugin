/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2009 Xiph.Org Foundation
   Written by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "celt.h"
#include "arch.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "celt_header.h"

#define MAX_PACKET 1275

typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef float F_FLOAT;
#define FSOUND_FSB_NAMELEN 30

typedef struct {
    char        id[4];      /* 'FSB4' */
    int32_t     numsamples; /* number of samples in the file */
    int32_t     shdrsize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     datasize;   /* size in bytes of compressed sample data */
    uint32_t    version;    /* extended fsb version */
    uint32_t    mode;       /* flags that apply to all samples in the fsb */
    unsigned char        zero[8];    /* ??? */
    uint8_t     hash[16];   /* hash??? */
} FSOUND_FSB_HEADER_FSB4;


typedef struct {
    uint16_t    size;
    char        name[FSOUND_FSB_NAMELEN];

    uint32_t    lengthsamples;
    uint32_t    lengthcompressedbytes;
    uint32_t    loopstart;
    uint32_t    loopend;

    uint32_t    mode;
    int32_t     deffreq;
    uint16_t    defvol;
    int16_t     defpan;
    uint16_t    defpri;
    uint16_t    numchannels;

    F_FLOAT     mindistance;
    F_FLOAT     maxdistance;
    int32_t     varfreq;
    uint16_t    varvol;
    int16_t     varpan;

} FSOUND_FSB_SAMPLE_HEADER_3_1;

const char* celt_strerror(int) {
    return "a";
}

int main(int argc, char *argv[])
{
   int err = 0;
   char *inFile, *outFile;
   FILE *fin, *fout;
   CELTMode *mode=NULL;
   CELTDecoder *dec;
   int len;
   int frame_size, channels;
   //int bytes_per_packet;
//   unsigned char data[MAX_PACKET];
   int rate;
   //int complexity;
#if !(defined (FIXED_POINT) && defined(STATIC_MODES))
   //int i;
   //double rmsd = 0;
#endif
   int count = 0;
   int skip;
   //celt_int16 *in;
   short *out;
   
   rate = 44100;
   channels = 1;
   //frame_size = atoi(argv[3]);
   //mode = celt_mode_create(rate, frame_size, NULL);
   //celt_mode_info(mode, CELT_GET_LOOKAHEAD, &skip);
   
   /*
   if (mode == NULL)
   {
      fprintf(stderr, "failed to create a mode\n");
      return 1;
   }
   
   bytes_per_packet = atoi(argv[4]);
   if (bytes_per_packet < 0 || bytes_per_packet > MAX_PACKET)
   {
      fprintf (stderr, "bytes per packet must be between 0 and %d\n",
                        MAX_PACKET);
      return 1;
   }
   */

   //inFile = argv[argc-2];
   inFile = "lux_basicattack2_oc_2";
   fin = fopen(inFile, "rb");
   if (!fin)
   {
      fprintf (stderr, "Could not open input file %s\n", argv[argc-2]);
      return 1;
   }
   outFile = "outfile61_x64";
   fout = fopen(outFile, "wb+");
   if (!fout)
   {
      fprintf (stderr, "Could not open output file %s\n", argv[argc-1]);
      return 1;
   }

   

   frame_size = 1000;
   skip = 0;

   //in = (celt_int16*)malloc(frame_size*channels*sizeof(celt_int16));
   out = (short*)malloc(frame_size*channels*sizeof(short));

   char inData[11918];

   dec = NULL;



   
   auto luxFile = fopen("LoL_SFX_lux.fsb", "rb");
   //auto luxFile = fopen("untitled..sb", "rb");
   fseek(luxFile, 0, SEEK_END);
   auto luxSize = ftell(luxFile);
   fseek(luxFile, 0, SEEK_SET);

   auto luxData = new char[luxSize];
   fread(luxData, 1, luxSize, luxFile);
   fclose(luxFile);

   auto luxHeader = (FSOUND_FSB_HEADER_FSB4*)luxData;

   char* ptr = (char*)luxHeader + sizeof(*luxHeader);

   auto firstSampleHeader = (FSOUND_FSB_SAMPLE_HEADER_3_1*)ptr;
   char* luxFirstOffset = ptr + luxHeader->shdrsize;
   char* luxSecondOffset = luxFirstOffset + firstSampleHeader->lengthcompressedbytes;

   char* aOffset = luxFirstOffset;
   for (int i = 0; i < luxHeader->numsamples; i++) {
       auto sampleHeader = (FSOUND_FSB_SAMPLE_HEADER_3_1*)ptr;
       ptr += sampleHeader->size;
       int moreData = sampleHeader->size - sizeof(*sampleHeader);
       int x = 2;
       char arr[80];
       strcpy(arr, "ext_");
       strcat(arr, sampleHeader->name);
       strcat(arr, ".celt");
       if (i < 2) {
           FILE* _out = fopen(arr, "wb");
           fwrite(aOffset, 1, sampleHeader->lengthcompressedbytes, _out);
           fclose(_out);
       }
       aOffset += sampleHeader->lengthcompressedbytes;

   }
   ptrdiff_t measuredLen = aOffset - luxData;
   auto diff = luxSize - measuredLen;
   


   len = (int)fread(inData, sizeof(char), 11918, fin);

   frame_size = 512;
   int freq = 44100;
   mode = celt_mode_create(freq, firstSampleHeader->numchannels, frame_size, NULL);
   dec = celt_decoder_create(mode);

   celt_mode_info(mode, CELT_GET_LOOKAHEAD, &skip);

   if (err != 0)
   {
       fprintf(stderr, "Failed to create the decoder: %s\n", celt_strerror(err));
       return 1;
   }

   int idx = 0;
   unsigned char* decodePtr = (unsigned char*)luxFirstOffset;
   //unsigned char* decodePtr = (unsigned char*)luxSecondOffset;
   //auto decodePtr2 = (unsigned char*)inData;
   bool error = false;
   while (true)
   {
       int* ptr = (int*)decodePtr;
       decodePtr += 8;
       int len = ptr[1];
       //err = celt_decode(dec, decodePtr2, firstSampleHeader->lengthcompressedbytes, (celt_int16*)out, frame_size);
       err = celt_decode(dec, decodePtr, len, (short*)out);
       decodePtr += ptr[1];
       if (err != 0) {
           fprintf(stderr, "celt_decode() failed: %s\n", celt_strerror(err));
           error |= true;
       }
       else 
           printf("no error\n");

      fwrite(out, sizeof(short), frame_size*firstSampleHeader->numchannels, fout);
      idx++;
      if (idx == firstSampleHeader->lengthsamples / frame_size) {
          break;
      }
      //break;
   }
   if (error) {
       printf("\a");
   }
   PRINT_MIPS(stderr);
   
   celt_decoder_destroy(dec);
   fclose(fin);
   fclose(fout);
   celt_mode_destroy(mode);
   //free(in);
   free(out);
#ifdef RESYNTH
   if (rmsd > 0)
   {
      rmsd = sqrt(rmsd/(1.0*frame_size*channels*count));
      fprintf (stderr, "Error: encoder doesn't match decoder\n");
      fprintf (stderr, "RMS mismatch is %f\n", rmsd);
      return 1;
   } else {
      fprintf (stderr, "Encoder matches decoder!!\n");
   }
#endif
   return 0;
}

