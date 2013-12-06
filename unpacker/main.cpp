
#include <Windows.h>
#include <mmreg.h>
#include <ShlObj.h>

#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <fmod.hpp>
#include <fmod_errors.h>
#include <set>

#include "lol_fmod_plugin.h"

#ifdef _WIN64
#pragma comment(lib, "fmodex64_vc.lib")
#else
#pragma comment(lib, "fmodex_vc.lib")
#endif

void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}

void SaveToWav(const std::string& outPath, FMOD::Sound *sound)
{
    FILE *fp;
    int             channels, bits;
    float           rate;
    unsigned int    lenbytes;

    if (!sound)
    {
        return;
    }

    sound->getFormat(0, 0, &channels, &bits);
    sound->getDefaults(&rate, 0, 0, 0);
    FMOD_SOUND_TYPE type;
    FMOD_SOUND_FORMAT format;
    int chan, bit;
    sound->getFormat(&type, &format, &chan, &bit);
    if (rate > 45000) {
        rate *= 2.f;
    }
    sound->getLength(&lenbytes, FMOD_TIMEUNIT_PCMBYTES);


    {
#if defined(WIN32) || defined(_WIN64) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
#pragma pack(1)
#endif
#define __PACKED

        /*
        WAV Structures
        */
        typedef struct
        {
            signed char id[4];
            int 		size;
        } RiffChunk;

        struct
        {
            RiffChunk       chunk           __PACKED;
            unsigned short	wFormatTag      __PACKED;    /* format type  */
            unsigned short	nChannels       __PACKED;    /* number of channels (i.e. mono, stereo...)  */
            unsigned int	nSamplesPerSec  __PACKED;    /* sample rate  */
            unsigned int	nAvgBytesPerSec __PACKED;    /* for buffer estimation  */
            unsigned short	nBlockAlign     __PACKED;    /* block size of data  */
            unsigned short	wBitsPerSample  __PACKED;    /* number of bits per sample of mono data */
        } FmtChunk = { { { 'f', 'm', 't', ' ' }, sizeof(FmtChunk)-sizeof(RiffChunk) }, 1, channels, (int)rate, (int)rate * channels * bits / 8, 1 * channels * bits / 8, bits } __PACKED;

        struct
        {
            RiffChunk   chunk;
        } DataChunk = { { { 'd', 'a', 't', 'a' }, lenbytes } };

        struct
        {
            RiffChunk   chunk;
            signed char rifftype[4];
        } WavHeader = { { { 'R', 'I', 'F', 'F' }, sizeof(FmtChunk)+sizeof(RiffChunk)+lenbytes }, { 'W', 'A', 'V', 'E' } };

#if defined(WIN32) || defined(_WIN64) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
#pragma pack()
#endif

        fopen_s(&fp, outPath.c_str(), "wb");

        /*
        Write out the WAV header.
        */
        fwrite(&WavHeader, sizeof(WavHeader), 1, fp);
        fwrite(&FmtChunk, sizeof(FmtChunk), 1, fp);
        fwrite(&DataChunk, sizeof(DataChunk), 1, fp);

        /*
        Write it to disk.
        */


        int total = 0;
        char buffer[4096];
        unsigned int didRead;
        do {
            int toRead = min(sizeof(buffer), lenbytes - total);
            sound->readData(buffer, toRead, &didRead);
            fwrite(buffer, 1, didRead, fp);
            total += didRead;
        } while (total != lenbytes);


        fclose(fp);
    }
}

void makePath(std::string path, bool hasFilePart = false) {

    if (path[1] != ':') {
        char arr[512];
        arr[GetCurrentDirectory(512, arr)] = 0;
        path = std::string(arr) + "\\" + path;
    }

    std::replace(path.begin(), path.end(), '/', '\\');
    if (hasFilePart) {
        path = path.substr(0, path.find_last_of('\\'));
    }

    std::cout << "Making dir" << std::endl;
    std::cout << "   " << path << std::endl;

    auto shError = SHCreateDirectoryEx(NULL, path.c_str(), NULL);
    if (shError == ERROR_SUCCESS || shError == ERROR_ALREADY_EXISTS) {
    }
    else {
        throw std::runtime_error("Error creating dir");
    }
}




int main(int argc, char *argv[])
{
    FMOD::System           *system;
    FMOD::Sound            *sound;
    FMOD::Channel          *channel = 0;
    FMOD_RESULT             result;
    FMOD_MODE               mode = FMOD_2D | FMOD_HARDWARE | FMOD_CREATESTREAM;
    unsigned int version;

    if (argc != 3) {
        std::cout << "unpacker.exe fsbPath outdirPath" << std::endl;
        return 1;
    }

    auto fsbPath = std::string(argv[1]);
    auto outPath = std::string(argv[2]);

    //fsbPath = "LoL_SFX_ziggs.fsb";
    //fsbPath = "LoL_SFX_karma_base.fsb";
    //fsbPath = "LoL_SFX_fiddlesticks.fsb";


    result = FMOD::System_Create(&system);
    ERRCHECK(result);

    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    //system->setOutput(FMOD_OUTPUTTYPE_WAVWRITER);
    result = system->init(1, FMOD_INIT_NORMAL, 0);
    ERRCHECK(result);

    auto codecHandle = registerLeagueCodec(system, 50);

    int numsubsounds;
    FMOD::Sound *subSound = nullptr;
    char name[256];
    int soundNum = 0;

    try {
        result = system->createSound(fsbPath.c_str(), mode, nullptr, &sound);
        ERRCHECK(result);

        result = sound->getNumSubSounds(&numsubsounds);
        ERRCHECK(result);
        soundNum = 0;


        sound->getSubSound(0, &subSound);
        subSound->getName(name, 256);
        subSound->release();

        makePath(outPath.c_str());
        sound->release();
        system->close();
        system->release();

        std::set<std::string> writtenFiles;

        FMOD::Channel* channel;
        bool playing;
        for (int sndIdx = 0; sndIdx < numsubsounds; sndIdx++) {
            ERRCHECK(FMOD::System_Create(&system));
            ERRCHECK(system->getVersion(&version));
            system->setOutput(FMOD_OUTPUTTYPE_WAVWRITER_NRT);
            auto outFilePath = outPath + "\\" + std::string(name) + ".wav";
            if (writtenFiles.find(outFilePath) != writtenFiles.end()) {
                int cnt = 1;
                char arr[80];
                do {
                    _itoa_s(cnt, arr, 10);
                    outFilePath = outPath + "\\" + std::string(name) + "_" + std::string(arr) + ".wav";
                    cnt++;
                } while (writtenFiles.find(outFilePath) != writtenFiles.end());
            }
            writtenFiles.insert(outFilePath);
            ERRCHECK(system->init(1, FMOD_INIT_STREAM_FROM_UPDATE, (void*)outFilePath.c_str()));
            auto codecHandle = registerLeagueCodec(system, 50);
            system->createSound(fsbPath.c_str(), mode, nullptr, &sound);
            sound->getSubSound(sndIdx, &subSound);
            system->playSound(FMOD_CHANNEL_FREE, subSound, false, &channel);
            do {
                
                system->update();
                channel->isPlaying(&playing);
            } while (playing);
            subSound->release();


            if (sndIdx < numsubsounds - 1) {
                sound->getSubSound(sndIdx+1, &subSound);
                subSound->getName(name, 256);
                subSound->release();
                outFilePath = outPath + "\\" + std::string(name) + ".wav";
            }
            sound->release();
            system->close();
            system->release();
        }
    }
    catch (const std::runtime_error& error) {
        std::cout << "Exception caught:" << std::endl;
        std::cout << "   " << error.what() << std::endl;
    }

    return 0;
}


