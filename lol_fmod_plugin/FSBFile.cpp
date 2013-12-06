#include "FSBFile.h"

#include <fsbank.h>

//#include <iostream>

#define FMOD_FSB_SOURCE_MPEG_PADDED4    0x40

#define FSOUND_OGG                      0x8000000

FSBFile::FSBFile(FMOD_CODEC_STATE* codec)
:codec(codec), currentSound(-1)
{
    unsigned int bytesRead;
    unsigned int totalRead = 0;
    codec->fileread(codec->filehandle, &header, sizeof(header), &bytesRead, nullptr);
    totalRead += bytesRead;
    FSBenforce(bytesRead == sizeof(header), "Bad size read for FSOUND_FSB_HEADER_FSB4");
    FSBenforce(!memcmp(header.id, "FSB4", 4), "Bad magic header, wanted FSB4");
    FSBenforce(header.numSamples, "Expected file to contain at least 1 sample, got 0");
    FSBenforce(header.version == 0x40000, "Header version mismatch!");

    sampleHeaders.reserve(header.numSamples);
    
    FSOUND_FSB_SAMPLE_HEADER_3_1 sampleHeader;
    for (int idx = 0; idx < header.numSamples; idx++) {
        codec->fileread(codec->filehandle, &sampleHeader, sizeof(sampleHeader), &bytesRead, nullptr);
        totalRead += bytesRead;
        FSBenforce(bytesRead == sizeof(sampleHeader), "Read bad amount of bytes when reading sample header");
        auto extra = sampleHeader.size - sizeof(sampleHeader);
        if (extra) {
            FSBenforce(idx == 0, "Only expected extra-data in first sample header");
            FSBenforce(extra == 4, "Expected an int worth of extra-data");
            unsigned int stuff;
            codec->fileread(codec->filehandle, &stuff, 4, &bytesRead, nullptr);
            totalRead += bytesRead;
            FSBenforce(bytesRead == extra, "Bad read when reading extra stuff");
            //FSBenforce(stuff == 0x80000009, "Bad extra-data-verifying type of stuff. Uhm yeah.");
        }

        FSBenforce(sampleHeader.mode & FSOUND_OGG, "Non-ogg/celt file detected!");

        sampleHeaders.push_back(sampleHeader);
    }

    auto toHaveRead = sizeof(header)+header.sampleHeadersSize;
    //FSBenforce(totalRead == toHaveRead, "Bad things?"); // Seems it might not necessarrily be bad things after all.
    // Oh well.

    codec->plugindata = this;
    codec->numsubsounds = header.numSamples;
    codec->waveformat = nullptr;
}

void FSBFile::dispose() {
    // Close crap!
    endDecodeSound();
    codec->plugindata = nullptr;
}

FMOD_RESULT FSBFile::getFormat(int index, FMOD_CODEC_WAVEFORMAT* format) {
    FSBenforce(0 <= index && index < header.numSamples, "Bad subsound index!");

    const auto& header = sampleHeaders[index];
    strcpy_s(format->name, 30, header.name);
    format->format = FMOD_SOUND_FORMAT_PCM16;
    format->channels = header.numchannels;
    FSBenforce(header.numchannels <= 2, "Cant handle more than 2 channels");
    if (header.deffreq != 44100) {
        FSBenforce(header.deffreq == 44100, "Unexpected frequency encountered, investigate?");
        format->frequency = 44100; // Blindly ignore for now
        //TODO: Investigate :P
    }
    else {
        format->frequency = header.deffreq;
    }
    format->lengthbytes = header.lengthcompressedbytes; // If things go wrong change this to lengthsamples * channels * sizeof(short)

    format->lengthbytes = header.lengthsamples * header.numchannels * sizeof(short);

    format->lengthpcm = header.lengthsamples;
    format->blockalign = 512; // Possibly very wrong :P

    //format->blockalign = 512 * header.numchannels; // Possibly very wrong :P

    format->loopstart = header.loopstart;
    format->loopend = header.loopend;
    format->mode = header.mode;
    format->mode = 0;
    format->channelmask = 0;

    return FMOD_OK;
}

void FSBFile::readFrame() {
    if (currentFrame == frameCount) {
        return; //TODO: remove return :P
        FSBenforce(currentFrame != sampleHeaders[currentSound].lengthsamples / 512, "Uhm we read to far?");
    }

    int packetHeader[2];
    unsigned int bytesRead;
    codec->fileread(codec->filehandle, packetHeader, sizeof(packetHeader), &bytesRead, nullptr);
    FSBenforce(bytesRead == sizeof(packetHeader), "Bad number of bytes read when reading packet header");
    if (currentFrame == 0) {
        packetMagic = packetHeader[0];
        packetLength = packetHeader[1];
        packetBuffer.reserve(packetLength);
    }
    else {
        FSBenforce(packetMagic == packetHeader[0], "Maybe it isnt packet magick?");
        FSBenforce(packetLength == packetHeader[1], "Maybe it isnt constant thingy mahjingy?");
    }
    unsigned char* bufferPtr = (unsigned char*)packetBuffer.data();
    codec->fileread(codec->filehandle, bufferPtr, packetLength, &bytesRead, nullptr);
    if (bytesRead != packetLength) {
        FSBenforce(bytesRead == packetLength, "Bad number of bytes read when reading frame");
    }
    currentFrame++;
}

FMOD_RESULT FSBFile::read(void *buffer, unsigned int size, unsigned int *read) {
    //std::cout << "read " << size << std::endl;

    const auto& sampleHeader = sampleHeaders[currentSound];
    auto channels = sampleHeader.numchannels;
    short* outBufferPos = (short*)buffer;
    auto samplesPerFrame = 512 * channels;
    auto samplesToRead = size / sizeof(short);
    *read = 0;
    /*
    if (samplesToRead < samplesPerFrame) {
        return FMOD_OK;
    }
    FSBenforce(samplesToRead * sizeof(short) == size, "Ugh ugh!");
    FSBenforce(!(samplesToRead%samplesPerFrame), "bug bug!");
    */
    
    while (samplesToRead) {

        if (decodeAvailable) {
            // Read min(available, needed)
            auto toRead = decodeAvailable < samplesToRead ? decodeAvailable : samplesToRead;
            memcpy(outBufferPos, decodeBuffer + samplesPerFrame - decodeAvailable, toRead * sizeof(short));
            *read += (unsigned int)(toRead * sizeof(short));
            decodeAvailable -= toRead;
            samplesToRead -= toRead;
            outBufferPos += toRead;
        }
        else if (samplesToRead >= samplesPerFrame) { // Read directly to target if possible.
            readFrame();
            unsigned char* bufferPtr = (unsigned char*)packetBuffer.data();

            auto err = celt_decode(celtDec, bufferPtr, packetLength, outBufferPos);
            FSBenforce(!err, "Some error during the decoding!");
            *read += (unsigned int)(samplesPerFrame * sizeof(short));
            samplesToRead -= samplesPerFrame;
            outBufferPos += samplesPerFrame;
        }
        else {
            readFrame();
            unsigned char* bufferPtr = (unsigned char*)packetBuffer.data();

            auto err = celt_decode(celtDec, bufferPtr, packetLength, decodeBuffer);
            FSBenforce(!err, "Some error during the decoding!");
            decodeAvailable = samplesPerFrame;
        }
    }

    return FMOD_OK;
}

void FSBFile::startDecodeSound(int idx) {
    //std::cout << "startDecodeSound " << idx << std::endl;
    if (currentSound != -1) {
        endDecodeSound();
    }
    int frame_size = 512;
    int freq = 44100;
    int channels = sampleHeaders[idx].numchannels;
    celtMode = celt_mode_create(freq, channels, frame_size, NULL);
    celtDec = celt_decoder_create(celtMode);
    currentSound = idx;

    size_t soundStart = 0;
    soundStart += sizeof(header);
    soundStart += header.sampleHeadersSize;
    for (int i = 0; i < idx; i++) {
        const auto& sampleHeader = sampleHeaders[i];
        soundStart += sampleHeader.lengthcompressedbytes;
        if (header.mode & FMOD_FSB_SOURCE_MPEG_PADDED4) { // Align stuff?
            const int alignSize = 32;
            auto m = soundStart % alignSize;
            if (m) {
                soundStart += (alignSize - m);
            }
        }
    }
    currentSoundOffset = soundStart;
}

void FSBFile::endDecodeSound() {
    //std::cout << "endDecodeSound " << std::endl;
    if (currentSound == -1) {
        return;
    }
    celt_decoder_destroy(celtDec);
    celt_mode_destroy(celtMode);
    currentSound = -1;
}

#pragma comment(lib, "celt61.lib")

FMOD_RESULT FSBFile::seekSound(int subSound, unsigned int position, FMOD_TIMEUNIT postype) {
    //std::cout << "seekSound " << subSound << " " << position << std::endl;
    if (currentSound != subSound) {
        startDecodeSound(subSound);
    }
    if (position != 0) {
        //std::cout << position << std::endl;
        throw FSBException("Cant handle non-0 position yet! :S");
    }
    size_t soundAbsPos = position * 0; // Fix this lol :P
    size_t positionOffset = currentSoundOffset + soundAbsPos;
    codec->fileseek(codec->filehandle, (unsigned int)positionOffset, nullptr);

    currentFrame = 0; // Correlate this with position hoho
    frameCount = sampleHeaders[currentSound].lengthsamples / 512;
    decodeAvailable = 0;

    return FMOD_OK;
}
