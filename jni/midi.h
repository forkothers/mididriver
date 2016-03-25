////////////////////////////////////////////////////////////////////////////////
//
//  MidiDriver - An Android Midi Driver.
//
//  Copyright (C) 2013	Bill Farmer
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
//  Bill Farmer	 william j farmer [at] yahoo [dot] co [dot] uk.
//
///////////////////////////////////////////////////////////////////////////////

#include <jni.h>
#include <dlfcn.h>
#include <assert.h>
#include <pthread.h>

#include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "org_billthefarmer_mididriver_MidiDriver.h"

// for EAS midi
#include "eas.h"
#include "eas_reverb.h"

#define LOG_TAG "MidiDriver"

#define LOG_D(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#define LOG_E(tag, ...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)
#define LOG_I(tag, ...) __android_log_print(ANDROID_LOG_INFO, tag, __VA_ARGS__)

// determines how many EAS buffers to fill a host buffer
#define NUM_BUFFERS 4

// EAS data
const S_EAS_LIB_CONFIG *pLibConfig;

// EAS function pointers
EAS_PUBLIC const S_EAS_LIB_CONFIG *(*pEAS_Config) (void);
EAS_PUBLIC EAS_RESULT (*pEAS_Init) (EAS_DATA_HANDLE *ppEASData);
EAS_PUBLIC EAS_RESULT (*pEAS_SetParameter) (EAS_DATA_HANDLE pEASData,
					    EAS_I32 module,
					    EAS_I32 param,
					    EAS_I32 value);
EAS_PUBLIC EAS_RESULT (*pEAS_OpenMIDIStream) (EAS_DATA_HANDLE pEASData,
					      EAS_HANDLE *pStreamHandle,
                                              EAS_HANDLE streamHandle);
EAS_PUBLIC EAS_RESULT (*pEAS_Shutdown) (EAS_DATA_HANDLE pEASData);
EAS_PUBLIC EAS_RESULT (*pEAS_Render) (EAS_DATA_HANDLE pEASData,
				      EAS_PCM *pOut,
				      EAS_I32 numRequested,
                                      EAS_I32 *pNumGenerated);
EAS_PUBLIC EAS_RESULT (*pEAS_WriteMIDIStream)(EAS_DATA_HANDLE pEASData,
					      EAS_HANDLE streamHandle,
					      EAS_U8 *pBuffer,
                                              EAS_I32 count);
EAS_PUBLIC EAS_RESULT (*pEAS_CloseMIDIStream) (EAS_DATA_HANDLE pEASData,
					       EAS_HANDLE streamHandle);

class MidiDriver
{
public:
    MidiDriver(jobject){}
    ~MidiDriver(){}

    jobject getJobject();
    SLresult createEngine();
    SLresult createBufferQueueAudioPlayer();
    void shutdownAudio();
    EAS_RESULT initEAS();
    void shutdownEAS();

private:
    jobject jobj;

    // mutex
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    // engine interfaces
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;

    // output mix interfaces
    SLObjectItf outputMixObject = NULL;

    // buffer queue player interfaces
    SLObjectItf bqPlayerObject = NULL;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

    // EAS data
    EAS_DATA_HANDLE pEASData;
    EAS_PCM *buffer;
    EAS_I32 bufferSize;
    EAS_HANDLE midiHandle;

    void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context);
}
