#include <stdio.h>


#include <Windows.h>

#include <iostream>

#include <fmod.hpp>
#include <fmod_errors.h>

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


int main(int argc, char *argv[])
{
    FMOD::System           *system;
    FMOD::Sound            *sound;
    FMOD::Channel          *channel = 0;
    FMOD_RESULT             result;
    FMOD_MODE               mode = FMOD_2D | FMOD_HARDWARE | FMOD_CREATESTREAM;
    unsigned int version;


    /*
    Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);

    FMOD::Debug_SetLevel(FMOD_DEBUG_TYPE_ALL);

    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    result = system->init(32, FMOD_INIT_NORMAL, 0);
    ERRCHECK(result);

    printf("Custom league fsb test :D\n");
    printf("\n");


    auto codecHandle = registerLeagueCodec(system, 0);

    int numsubsounds;
    FMOD::Sound *subsound = nullptr;
    char name[256];
    int soundNum = 0;
    try {
        //auto soundPath = "LoL_SFX_nasus_base.fsb";
        //auto soundPath = "LoL_SFX_ziggs.fsb";
        auto soundPath = "VOBank_en_US.fsb";        
        //auto soundPath = "LoL_SFX_lux.fsb";
        result = system->createSound(soundPath, mode, nullptr, &sound);
        //result = system->createStream(soundPath, mode, nullptr, &sound);
        ERRCHECK(result);

        printf("Press space to pause, Esc to quit\n");
        printf("\n");

        /*
        Play the sound.
        */


        result = sound->getNumSubSounds(&numsubsounds);
        ERRCHECK(result);

        for (int count = 0; count < 0 * numsubsounds; count++)
        {
            result = sound->getSubSound(count, &subsound);
            ERRCHECK(result);
            result = subsound->getName(name, 256);
            ERRCHECK(result);

            std::cout << name;
        }

        soundNum = min(numsubsounds / 2, 1260);
        soundNum = 6300;
        result = sound->getSubSound(soundNum, &subsound);
        ERRCHECK(result);
        result = subsound->getName(name, 256);
        ERRCHECK(result);

        std::cout << name;

        result = system->playSound(FMOD_CHANNEL_FREE, subsound, 0, &channel);
        ERRCHECK(result);
    }
    catch (const std::runtime_error& error) {
        std::cout << "Exception caught:" << std::endl;
        std::cout << "   " << error.what() << std::endl;
    }


    /*
    Main loop.
    */
    do
    {
        system->update();

        if (channel)
        {
            unsigned int ms;
            unsigned int lenms;
            bool         playing;
            bool         paused;

            channel->isPlaying(&playing);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
            {
                ERRCHECK(result);
            }

            result = channel->getPaused(&paused);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
            {
                ERRCHECK(result);
            }

            result = channel->getPosition(&ms, FMOD_TIMEUNIT_MS);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
            {
                ERRCHECK(result);
            }

            result = subsound->getLength(&lenms, FMOD_TIMEUNIT_MS);
            if ((result != FMOD_OK) && (result != FMOD_ERR_INVALID_HANDLE) && (result != FMOD_ERR_CHANNEL_STOLEN))
            {
                ERRCHECK(result);
            }

            printf("\rTime %02d:%02d:%02d/%02d:%02d:%02d : %s", ms / 1000 / 60, ms / 1000 % 60, ms / 10 % 100, lenms / 1000 / 60, lenms / 1000 % 60, lenms / 10 % 100, paused ? "Paused " : playing ? "Playing" : "Stopped\n");

            const std::string asd("Interactive3DClearNeutralCamp");
            if (!playing) {

                do {
                    std::cout << soundNum << "\r";
                    result = sound->getSubSound(soundNum++, &subsound);
                    ERRCHECK(result);
                    result = subsound->getName(name, 256);
                    ERRCHECK(result);
                } while (asd != name);
                printf("\n");
                std::cout << name << std::endl;
                result = system->playSound(FMOD_CHANNEL_FREE, subsound, 0, &channel);
            }
        }

        Sleep(20);

    } while (!GetAsyncKeyState(VK_ESCAPE) && !GetAsyncKeyState(VK_PAUSE));

    printf("\n");

    /*
    Shut down
    */
    result = sound->release();
    ERRCHECK(result);
    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

    return 0;
}


