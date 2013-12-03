/*===============================================================================================
 CODEC_RAW.DLL
 Copyright (c), Firelight Technologies Pty, Ltd 2004-2011.

===============================================================================================*/

//#include <stdio.h>

#include <fmod.h>
#include <fmod_errors.h>

//#include <iostream>

#include "FSBFile.h"

FMOD_RESULT F_CALLBACK FSBOpen(FMOD_CODEC_STATE *codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo);
FMOD_RESULT F_CALLBACK FSBClose(FMOD_CODEC_STATE *codec);
FMOD_RESULT F_CALLBACK FSBRead(FMOD_CODEC_STATE *codec, void *buffer, unsigned int size, unsigned int *read);
FMOD_RESULT F_CALLBACK FSBSetPosition(FMOD_CODEC_STATE *codec, int subsound, unsigned int position, FMOD_TIMEUNIT postype);
FMOD_RESULT F_CALLBACK FSBGetFormat(FMOD_CODEC_STATE *codec_state, int index, FMOD_CODEC_WAVEFORMAT *waveformat);


FMOD_CODEC_DESCRIPTION FSBCodec =
{
    "FMOD League file plugin",   // Name.
    0x00010000,                         // Version 0xAAAABBBB   A = major, B = minor.
    0,                                  // Don't force everything using this codec to be a stream
    FMOD_TIMEUNIT_PCMBYTES,             // The time format we would like to accept into setposition/getposition.
    &FSBOpen,                           // Open callback.
    &FSBClose,                          // Close callback.
    &FSBRead,                           // Read callback.
    0,                                  // Getlength callback.  (If not specified FMOD return the length in FMOD_TIMEUNIT_PCM, FMOD_TIMEUNIT_MS or FMOD_TIMEUNIT_PCMBYTES units based on the lengthpcm member of the FMOD_CODEC structure).
    &FSBSetPosition,                    // Setposition callback.
    0,                                  // Getposition callback. (only used for timeunit types that are not FMOD_TIMEUNIT_PCM, FMOD_TIMEUNIT_MS and FMOD_TIMEUNIT_PCMBYTES).
    0,                                   // Sound create callback (don't need it)
    &FSBGetFormat
};


FMOD_RESULT F_CALLBACK FSBOpen(FMOD_CODEC_STATE *codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO *userexinfo)
{
    FSBFile* file = nullptr;
    try {
        file = new FSBFile(codec);
    }
    //catch (const std::runtime_error& error) {
    catch (const FSBException& error) {
        //std::cout << "Exception caught:" << std::endl;
        //std::cout << "   " << error.what() << std::endl;
        error.what();
        delete file;
        return FMOD_ERR_FORMAT;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK FSBClose(FMOD_CODEC_STATE *codec)
{
    delete (FSBFile*)codec->plugindata;
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK FSBGetFormat(FMOD_CODEC_STATE *codec, int index, FMOD_CODEC_WAVEFORMAT *waveformat) {
    auto file = (FSBFile*)codec->plugindata;
    return file->getFormat(index, waveformat);

}

FMOD_RESULT F_CALLBACK FSBRead(FMOD_CODEC_STATE *codec, void *buffer, unsigned int size, unsigned int *read)
{
    auto file = (FSBFile*)codec->plugindata;
    return file->read(buffer, size, read);
    //return codec->fileread(codec->filehandle, buffer, size, read, 0);
}

FMOD_RESULT F_CALLBACK FSBSetPosition(FMOD_CODEC_STATE *codec, int subsound, unsigned int position, FMOD_TIMEUNIT postype)
{
    auto file = (FSBFile*)codec->plugindata;
    return file->seekSound(subsound, position, postype);
    //return codec->fileseek(codec->filehandle, position, 0);
}

#include <Windows.h>
#include <fmod.hpp>

unsigned int registerLeagueCodec(FMOD::System* system, int prio) {
    unsigned int fsbCodecHandle;
    return system->registerCodec(&FSBCodec, &fsbCodecHandle, prio);
}

