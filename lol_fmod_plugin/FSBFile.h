#pragma once

#include <fmod.h>
#include <stdexcept>
#include <vector>
#include <celt.h>

typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;

typedef struct {
    char        id[4];      /* 'FSB4' */
    int32_t     numSamples; /* number of samples in the file */
    int32_t     sampleHeadersSize;   /* size in bytes of all of the sample headers including extended information */
    int32_t     dataSize;   /* size in bytes of compressed sample data */
    uint32_t    version;    /* extended fsb version */
    uint32_t    mode;       /* flags that apply to all samples in the fsb */
    unsigned char        zero[8];    /* ??? */
    uint8_t     hash[16];   /* hash??? */
} FSOUND_FSB_HEADER_FSB4;

typedef struct {
    uint16_t    size;
    char        name[30];

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

    float     mindistance;
    float     maxdistance;
    int32_t     varfreq;
    uint16_t    varvol;
    int16_t     varpan;

} FSOUND_FSB_SAMPLE_HEADER_3_1;

class FSBException : public std::runtime_error {
public:
    FSBException(const std::string& msg) : runtime_error(msg){}
};

#define FSBenforce(X,Y) if(!(X)) { throw FSBException(Y); }

class FSBFile
{
    FMOD_CODEC_STATE* codec;
    FSOUND_FSB_HEADER_FSB4 header;

    std::vector<FSOUND_FSB_SAMPLE_HEADER_3_1> sampleHeaders;

    int packetMagic;
    int packetLength;
    std::vector<char> packetBuffer;

    short decodeBuffer[2 * 512];
    size_t decodeAvailable;

    int currentSound;
    size_t currentSoundOffset;

    size_t currentFrame;
    size_t frameCount;

    CELTMode* celtMode;
    CELTDecoder* celtDec;
    void startDecodeSound(int idx);
    void endDecodeSound();

    void readFrame();
public:
    FSBFile(FMOD_CODEC_STATE*);
    ~FSBFile() { dispose(); }

    FMOD_RESULT getFormat(int index, FMOD_CODEC_WAVEFORMAT* format);
    FMOD_RESULT read(void *buffer, unsigned int size, unsigned int *read);
    FMOD_RESULT seekSound(int subSound, unsigned int position, FMOD_TIMEUNIT postype);

    void dispose();
};

