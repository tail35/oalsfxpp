/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include "version.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>

#include "alMain.h"
#include "alSource.h"
#include "alSource.h"
#include "alBuffer.h"
#include "alAuxEffectSlot.h"
#include "bformatdec.h"
#include "alu.h"

#include "alstring.h"
#include "almalloc.h"


void InitSourceParams(ALsource *Source, ALsizei num_sends);
void DeinitSource(ALsource *source, ALsizei num_sends);


/************************************************
 * Global variables
 ************************************************/

/* Enumerated device names */
static const ALCchar alcDefaultName[] = "OpenAL Soft\0";

static al_string alcAllDevicesList;

/* Default is always the first in the list */
static ALCchar *alcDefaultAllDevicesSpecifier;

static ALCenum LastNullDeviceError = ALC_NO_ERROR;

/* Thread-local current context */
static ALCcontext* LocalContext;
/* Process-wide current context */
static ALCcontext* GlobalContext = NULL;

/* One-time configuration init control */
static ALCboolean alc_config_once = ALC_FALSE;

/* Default effect that applies to sources that don't have an effect on send 0 */
static ALeffect DefaultEffect;

/* Flag to specify if alcSuspendContext/alcProcessContext should defer/process
 * updates.
 */
static ALCboolean SuspendDefers = ALC_TRUE;


/************************************************
 * ALC information
 ************************************************/
static const ALCchar alcNoDeviceExtList[] =
    "ALC_ENUMERATE_ALL_EXT ALC_ENUMERATION_EXT ALC_EXT_CAPTURE "
    "ALC_EXT_thread_local_context ALC_SOFT_loopback";
static const ALCchar alcExtensionList[] =
    "ALC_ENUMERATE_ALL_EXT ALC_ENUMERATION_EXT ALC_EXT_CAPTURE "
    "ALC_EXT_DEDICATED ALC_EXT_disconnect ALC_EXT_EFX "
    "ALC_EXT_thread_local_context ALC_SOFTX_device_clock ALC_SOFT_HRTF "
    "ALC_SOFT_loopback ALC_SOFT_output_limiter ALC_SOFT_pause_device";
static const ALCint alcMajorVersion = 1;
static const ALCint alcMinorVersion = 1;

static const ALCint alcEFXMajorVersion = 1;
static const ALCint alcEFXMinorVersion = 0;


/************************************************
 * Device lists
 ************************************************/
static ALCdevice* DeviceList = NULL;


/************************************************
 * Library initialization
 ************************************************/
#if defined(_WIN32)
static void alc_init(void);
static void alc_deinit(void);
static void alc_deinit_safe(void);

#ifndef AL_LIBTYPE_STATIC
BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID lpReserved)
{
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            /* Pin the DLL so we won't get unloaded until the process terminates */
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                               (WCHAR*)hModule, &hModule);
            alc_init();
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            if(!lpReserved)
                alc_deinit();
            else
                alc_deinit_safe();
            break;
    }
    return TRUE;
}
#elif defined(_MSC_VER)
#pragma section(".CRT$XCU",read)
static void alc_constructor(void);
static void alc_destructor(void);
__declspec(allocate(".CRT$XCU")) void (__cdecl* alc_constructor_)(void) = alc_constructor;

static void alc_constructor(void)
{
    atexit(alc_destructor);
    alc_init();
}

static void alc_destructor(void)
{
    alc_deinit();
}
#elif defined(HAVE_GCC_DESTRUCTOR)
static void alc_init(void) __attribute__((constructor));
static void alc_deinit(void) __attribute__((destructor));
#else
#error "No static initialization available on this platform!"
#endif

#elif defined(HAVE_GCC_DESTRUCTOR)

static void alc_init(void) __attribute__((constructor));
static void alc_deinit(void) __attribute__((destructor));

#else
#error "No global initialization available on this platform!"
#endif

static void alc_init(void)
{
    AL_STRING_INIT(alcAllDevicesList);
}

static void alc_initconfig(void)
{
    aluInitMixer();

    InitEffect(&DefaultEffect);
}
void DO_INITCONFIG()
{
    if (!alc_config_once)
    {
        alc_config_once = ALC_TRUE;
        alc_initconfig();
    }
}


/************************************************
 * Library deinitialization
 ************************************************/
static void alc_cleanup(void)
{
    ALCdevice *dev;

    AL_STRING_DEINIT(alcAllDevicesList);

    free(alcDefaultAllDevicesSpecifier);
    alcDefaultAllDevicesSpecifier = NULL;

    dev = DeviceList;
    DeviceList = NULL;
}

static void alc_deinit_safe(void)
{
    alc_cleanup();
}

static void alc_deinit(void)
{
    alc_cleanup();
    alc_deinit_safe();
}


/************************************************
 * Miscellaneous ALC helpers
 ************************************************/

/* SetDefaultWFXChannelOrder
 *
 * Sets the default channel order used by WaveFormatEx.
 */
void SetDefaultWFXChannelOrder(ALCdevice *device)
{
    ALsizei i;

    for(i = 0;i < MAX_OUTPUT_CHANNELS;i++)
        device->RealOut.ChannelName[i] = InvalidChannel;

    switch(device->FmtChans)
    {
    case DevFmtMono:
        device->RealOut.ChannelName[0] = FrontCenter;
        break;
    case DevFmtStereo:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        break;
    case DevFmtQuad:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        device->RealOut.ChannelName[2] = BackLeft;
        device->RealOut.ChannelName[3] = BackRight;
        break;
    case DevFmtX51:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        device->RealOut.ChannelName[2] = FrontCenter;
        device->RealOut.ChannelName[3] = LFE;
        device->RealOut.ChannelName[4] = SideLeft;
        device->RealOut.ChannelName[5] = SideRight;
        break;
    case DevFmtX51Rear:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        device->RealOut.ChannelName[2] = FrontCenter;
        device->RealOut.ChannelName[3] = LFE;
        device->RealOut.ChannelName[4] = BackLeft;
        device->RealOut.ChannelName[5] = BackRight;
        break;
    case DevFmtX61:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        device->RealOut.ChannelName[2] = FrontCenter;
        device->RealOut.ChannelName[3] = LFE;
        device->RealOut.ChannelName[4] = BackCenter;
        device->RealOut.ChannelName[5] = SideLeft;
        device->RealOut.ChannelName[6] = SideRight;
        break;
    case DevFmtX71:
        device->RealOut.ChannelName[0] = FrontLeft;
        device->RealOut.ChannelName[1] = FrontRight;
        device->RealOut.ChannelName[2] = FrontCenter;
        device->RealOut.ChannelName[3] = LFE;
        device->RealOut.ChannelName[4] = BackLeft;
        device->RealOut.ChannelName[5] = BackRight;
        device->RealOut.ChannelName[6] = SideLeft;
        device->RealOut.ChannelName[7] = SideRight;
        break;
    }
}

extern inline ALint GetChannelIndex(const enum Channel names[MAX_OUTPUT_CHANNELS], enum Channel chan);


struct Compressor *CreateDeviceLimiter(const ALCdevice *device)
{
    return CompressorInit(0.0f, 0.0f, AL_FALSE, AL_TRUE, 0.0f, 0.0f, 0.5f, 2.0f,
                          0.0f, -3.0f, 3.0f, device->Frequency);
}


/* UpdateDeviceParams
 *
 * Updates device parameters according to the attribute list (caller is
 * responsible for holding the list lock).
 */
static ALCenum UpdateDeviceParams(ALCdevice *device, const ALCint *attrList)
{
    ALCenum gainLimiter = device->Limiter ? ALC_TRUE : ALC_FALSE;
    const ALsizei old_sends = device->NumAuxSends;
    ALsizei new_sends = device->NumAuxSends;
    enum DevFmtChannels oldChans;
    enum DevFmtType oldType;
    ALboolean update_failed;
    ALCcontext *context;
    ALCuint oldFreq;
    size_t size;
    ALCsizei i;

    if((device->Flags&DEVICE_RUNNING))
        return ALC_NO_ERROR;

    al_free(device->ChannelDelay[0].Buffer);
    for(i = 0;i < MAX_OUTPUT_CHANNELS;i++)
    {
        device->ChannelDelay[i].Length = 0;
        device->ChannelDelay[i].Buffer = NULL;
    }

    al_free(device->Dry.Buffer);
    device->Dry.Buffer = NULL;
    device->Dry.NumChannels = 0;
    device->FOAOut.Buffer = NULL;
    device->FOAOut.NumChannels = 0;
    device->RealOut.Buffer = NULL;
    device->RealOut.NumChannels = 0;

    oldFreq  = device->Frequency;
    oldChans = device->FmtChans;
    oldType  = device->FmtType;

    if(device->FmtChans != oldChans && (device->Flags&DEVICE_CHANNELS_REQUEST))
    {
        device->Flags &= ~DEVICE_CHANNELS_REQUEST;
    }
    if(device->FmtType != oldType && (device->Flags&DEVICE_SAMPLE_TYPE_REQUEST))
    {
        device->Flags &= ~DEVICE_SAMPLE_TYPE_REQUEST;
    }
    if(device->Frequency != oldFreq && (device->Flags&DEVICE_FREQUENCY_REQUEST))
    {
        device->Flags &= ~DEVICE_FREQUENCY_REQUEST;
    }

    aluInitRenderer(device);

    /* Allocate extra channels for any post-filter output. */
    size = (device->Dry.NumChannels + device->FOAOut.NumChannels +
            device->RealOut.NumChannels)*sizeof(device->Dry.Buffer[0]);

    device->Dry.Buffer = al_calloc(16, size);
    if(!device->Dry.Buffer)
    {
        return ALC_INVALID_DEVICE;
    }

    if(device->RealOut.NumChannels != 0)
        device->RealOut.Buffer = device->Dry.Buffer + device->Dry.NumChannels +
                                 device->FOAOut.NumChannels;
    else
    {
        device->RealOut.Buffer = device->Dry.Buffer;
        device->RealOut.NumChannels = device->Dry.NumChannels;
    }

    if(device->FOAOut.NumChannels != 0)
        device->FOAOut.Buffer = device->Dry.Buffer + device->Dry.NumChannels;
    else
    {
        device->FOAOut.Buffer = device->Dry.Buffer;
        device->FOAOut.NumChannels = device->Dry.NumChannels;
    }

    device->NumAuxSends = new_sends;

    /* Valid values for gainLimiter are ALC_DONT_CARE_SOFT, ALC_TRUE, and
     * ALC_FALSE. We default to on, so ALC_DONT_CARE_SOFT is the same as
     * ALC_TRUE.
     */
    if(gainLimiter != ALC_FALSE)
    {
        if(!device->Limiter || device->Frequency != GetCompressorSampleRate(device->Limiter))
        {
            al_free(device->Limiter);
            device->Limiter = CreateDeviceLimiter(device);
        }
    }
    else
    {
        al_free(device->Limiter);
        device->Limiter = NULL;
    }

    /* Need to delay returning failure until replacement Send arrays have been
     * allocated with the appropriate size.
     */
    update_failed = AL_FALSE;
    context = device->ContextList;
    if(context)
    {
        ALsizei pos;

        {
            ALeffectslot *slot = device->effect_slot;
            ALeffectState *state = slot->Effect.State;

            state->OutBuffer = device->Dry.Buffer;
            state->OutChannels = device->Dry.NumChannels;
            if(V(state,deviceUpdate)(device) == AL_FALSE)
                update_failed = AL_TRUE;
            else
                UpdateEffectSlotProps(slot);
        }

        {
            ALsource *source = device->source;

            if(old_sends != device->NumAuxSends)
            {
                ALvoid *sends = al_calloc(16, device->NumAuxSends*sizeof(source->Send[0]));
                ALsizei s;

                memcpy(sends, source->Send,
                    mini(device->NumAuxSends, old_sends)*sizeof(source->Send[0])
                );
                for(s = device->NumAuxSends;s < old_sends;s++)
                {
                    if(source->Send[s].Slot)
                        source->Send[s].Slot->ref -= 1;
                    source->Send[s].Slot = NULL;
                }
                al_free(source->Send);
                source->Send = sends;
                for(s = old_sends;s < device->NumAuxSends;s++)
                {
                    source->Send[s].Slot = NULL;
                    source->Send[s].Gain = 1.0f;
                    source->Send[s].GainHF = 1.0f;
                    source->Send[s].HFReference = LOWPASSFREQREF;
                    source->Send[s].GainLF = 1.0f;
                    source->Send[s].LFReference = HIGHPASSFREQREF;
                }
            }
        }
        AllocateVoices(context, context->MaxVoices, old_sends);
        for(pos = 0;pos < context->VoiceCount;pos++)
        {
            ALvoice *voice = context->Voices[pos];
            struct ALvoiceProps *props;

            /* Clear any pre-existing voice property structs, in case the
             * number of auxiliary sends changed. Active sources will have
             * updates respecified in UpdateAllSourceProps.
             */
            props = voice->Update;
            voice->Update = NULL;
            al_free(props);

            props = voice->FreeList;
            voice->FreeList = NULL;
            while(props)
            {
                struct ALvoiceProps *next = props->next;
                al_free(props);
                props = next;
            }

            if(voice->Source == NULL)
                continue;
        }

        UpdateAllSourceProps(context);
    }
    if(update_failed)
        return ALC_INVALID_DEVICE;

    if(!(device->Flags&DEVICE_PAUSED))
    {
        device->Flags |= DEVICE_RUNNING;
    }

    return ALC_NO_ERROR;
}

/* FreeDevice
 *
 * Frees the device structure, and destroys any objects the app failed to
 * delete. Called once there's no more references on the device.
 */
static ALCvoid FreeDevice(ALCdevice *device)
{
    ALsizei i;

    al_free(device->effect);

    DeinitEffectSlot(device->effect_slot);
    al_free(device->effect_slot);

    DeinitSource(device->source, device->NumAuxSends);
    al_free(device->source);

    al_free(device->Limiter);
    device->Limiter = NULL;

    al_free(device->ChannelDelay[0].Buffer);
    for(i = 0;i < MAX_OUTPUT_CHANNELS;i++)
    {
        device->ChannelDelay[i].Gain   = 1.0f;
        device->ChannelDelay[i].Length = 0;
        device->ChannelDelay[i].Buffer = NULL;
    }

    al_free(device->Dry.Buffer);
    device->Dry.Buffer = NULL;
    device->Dry.NumChannels = 0;
    device->FOAOut.Buffer = NULL;
    device->FOAOut.NumChannels = 0;
    device->RealOut.Buffer = NULL;
    device->RealOut.NumChannels = 0;

    al_free(device);
}


void ALCdevice_IncRef(ALCdevice *device)
{
    unsigned int ref;
    ref = ++device->ref;
}

void ALCdevice_DecRef(ALCdevice *device)
{
    unsigned int ref;
    ref = --device->ref;
    if(ref == 0) FreeDevice(device);
}

/* VerifyDevice
 *
 * Checks if the device handle is valid, and increments its ref count if so.
 */
static ALCboolean VerifyDevice(ALCdevice **device)
{
    ALCdevice *tmpDevice;

    tmpDevice = DeviceList;

    if(tmpDevice)
    {
        if(tmpDevice == *device)
        {
            ALCdevice_IncRef(tmpDevice);
            return ALC_TRUE;
        }
    }

    *device = NULL;
    return ALC_FALSE;
}


/* InitContext
 *
 * Initializes context fields
 */
static ALvoid InitContext(ALCcontext *Context)
{
    struct ALeffectslotArray *auxslots;

    auxslots = al_calloc(DEF_ALIGN, FAM_SIZE(struct ALeffectslotArray, slot, 1));
    auxslots->count = 1;
    auxslots->slot[0] = Context->Device->effect_slot;

    Context->ActiveAuxSlots = auxslots;
}


/* FreeContext
 *
 * Cleans up the context, and destroys any remaining objects the app failed to
 * delete. Called once there's no more references on the context.
 */
static void FreeContext(ALCcontext *context)
{
    struct ALeffectslotArray *auxslots;
    size_t count;
    ALsizei i;

    auxslots = context->ActiveAuxSlots;
    context->ActiveAuxSlots = NULL;
    al_free(auxslots);

    for(i = 0;i < context->VoiceCount;i++)
        DeinitVoice(context->Voices[i]);
    al_free(context->Voices);
    context->Voices = NULL;
    context->VoiceCount = 0;
    context->MaxVoices = 0;

    count = 0;

    ALCdevice_DecRef(context->Device);
    context->Device = NULL;

    //Invalidate context
    memset(context, 0, sizeof(ALCcontext));
    al_free(context);
}

/* ReleaseContext
 *
 * Removes the context reference from the given device and removes it from
 * being current on the running thread or globally. Returns true if other
 * contexts still exist on the device.
 */
static bool ReleaseContext(ALCcontext *context, ALCdevice *device)
{
    ALCcontext *origctx, *newhead;
    bool ret = true;

    origctx = context;
    if(GlobalContext == origctx ? (GlobalContext = NULL, true) : (origctx = GlobalContext, false))
        ALCcontext_DecRef(context);

    origctx = context;
    newhead = NULL;
    if(device->ContextList == origctx ? (device->ContextList = newhead, true) : (origctx = device->ContextList, false))
    {
        ret = !!newhead;
    }

    ALCcontext_DecRef(context);
    return ret;
}

void ALCcontext_IncRef(ALCcontext *context)
{
    unsigned int ref = ++context->ref;
}

void ALCcontext_DecRef(ALCcontext *context)
{
    unsigned int ref = --context->ref;
    if(ref == 0) FreeContext(context);
}

/* VerifyContext
 *
 * Checks that the given context is valid, and increments its reference count.
 */
static ALCboolean VerifyContext(ALCcontext **context)
{
    ALCdevice *dev;

    dev = DeviceList;
    if(dev)
    {
        ALCcontext *ctx = dev->ContextList;
        if(ctx)
        {
            if(ctx == *context)
            {
                ALCcontext_IncRef(ctx);
                return ALC_TRUE;
            }
        }
    }

    *context = NULL;
    return ALC_FALSE;
}


/* GetContextRef
 *
 * Returns the currently active context for this thread, and adds a reference
 * without locking it.
 */
ALCcontext *GetContextRef(void)
{
    ALCcontext *context;

    context = LocalContext;
    if(context)
        ALCcontext_IncRef(context);
    else
    {
        context = GlobalContext;
        if(context)
            ALCcontext_IncRef(context);
    }

    return context;
}


void AllocateVoices(ALCcontext *context, ALsizei num_voices, ALsizei old_sends)
{
    ALCdevice *device = context->Device;
    ALsizei num_sends = device->NumAuxSends;
    struct ALvoiceProps *props;
    size_t sizeof_props;
    size_t sizeof_voice;
    ALvoice **voices;
    ALvoice *voice;
    ALsizei v = 0;
    size_t size;

    if(num_voices == context->MaxVoices && num_sends == old_sends)
        return;

    /* Allocate the voice pointers, voices, and the voices' stored source
     * property set (including the dynamically-sized Send[] array) in one
     * chunk.
     */
    sizeof_voice = RoundUp(FAM_SIZE(ALvoice, Send, num_sends), 16);
    sizeof_props = RoundUp(FAM_SIZE(struct ALvoiceProps, Send, num_sends), 16);
    size = sizeof(ALvoice*) + sizeof_voice + sizeof_props;

    voices = al_calloc(16, RoundUp(size*num_voices, 16));
    /* The voice and property objects are stored interleaved since they're
     * paired together.
     */
    voice = (ALvoice*)((char*)voices + RoundUp(num_voices*sizeof(ALvoice*), 16));
    props = (struct ALvoiceProps*)((char*)voice + sizeof_voice);

    if(context->Voices)
    {
        const ALsizei v_count = mini(context->VoiceCount, num_voices);
        const ALsizei s_count = mini(old_sends, num_sends);

        for(;v < v_count;v++)
        {
            ALvoice *old_voice = context->Voices[v];
            ALsizei i;

            /* Copy the old voice data and source property set to the new
             * storage.
             */
            *voice = *old_voice;
            for(i = 0;i < s_count;i++)
                voice->Send[i] = old_voice->Send[i];
            *props = *(old_voice->Props);
            for(i = 0;i < s_count;i++)
                props->Send[i] = old_voice->Props->Send[i];

            /* Set this voice's property set pointer and voice reference. */
            voice->Props = props;
            voices[v] = voice;

            /* Increment pointers to the next storage space. */
            voice = (ALvoice*)((char*)props + sizeof_props);
            props = (struct ALvoiceProps*)((char*)voice + sizeof_voice);
        }
        /* Deinit any left over voices that weren't copied over to the new
         * array. NOTE: If this does anything, v equals num_voices and
         * num_voices is less than VoiceCount, so the following loop won't do
         * anything.
         */
        for(;v < context->VoiceCount;v++)
            DeinitVoice(context->Voices[v]);
    }
    /* Finish setting the voices' property set pointers and references. */
    for(;v < num_voices;v++)
    {
        voice->Update = NULL;
        voice->FreeList = NULL;

        voice->Props = props;
        voices[v] = voice;

        voice = (ALvoice*)((char*)props + sizeof_props);
        props = (struct ALvoiceProps*)((char*)voice + sizeof_voice);
    }

    al_free(context->Voices);
    context->Voices = voices;
    context->MaxVoices = num_voices;
    context->VoiceCount = mini(context->VoiceCount, num_voices);
}


/************************************************
 * Standard ALC functions
 ************************************************/

/* alcCreateContext
 *
 * Create and attach a context to the given device.
 */
ALC_API ALCcontext* ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrList)
{
    ALCcontext *ALContext;
    ALCenum err;

    /* Explicitly hold the list lock while taking the BackendLock in case the
     * device is asynchronously destropyed, to ensure this new context is
     * properly cleaned up after being made.
     */
    if(!VerifyDevice(&device))
    {
        if(device) ALCdevice_DecRef(device);
        return NULL;
    }

    if (device->ContextList)
    {
        return NULL;
    }

    if(DefaultEffect.type != AL_EFFECT_NULL)
        ALContext = al_calloc(16, sizeof(ALCcontext)+sizeof(ALeffectslot));
    else
        ALContext = al_calloc(16, sizeof(ALCcontext));
    if(!ALContext)
    {
        ALCdevice_DecRef(device);
        return NULL;
    }

    ALContext->ref = 1;

    ALContext->Voices = NULL;
    ALContext->VoiceCount = 0;
    ALContext->MaxVoices = 0;
    ALContext->ActiveAuxSlots = NULL;
    ALContext->Device = device;

    if((err=UpdateDeviceParams(device, attrList)) != ALC_NO_ERROR)
    {
        al_free(ALContext);
        ALContext = NULL;

        if(err == ALC_INVALID_DEVICE)
        {
            aluHandleDisconnect(device);
        }
        ALCdevice_DecRef(device);
        return NULL;
    }
    AllocateVoices(ALContext, 1, device->NumAuxSends);

    ALCdevice_IncRef(ALContext->Device);
    InitContext(ALContext);

    device->ContextList = ALContext;

    ALCdevice_DecRef(device);

    return ALContext;
}

/* alcDestroyContext
 *
 * Remove a context from its device
 */
ALC_API ALCvoid ALC_APIENTRY alcDestroyContext(ALCcontext *context)
{
    ALCdevice *Device;

    if(!VerifyContext(&context))
    {
        return;
    }

    Device = context->Device;
    if(Device)
    {
        if(!ReleaseContext(context, Device))
        {
            Device->Flags &= ~DEVICE_RUNNING;
        }
    }

    ALCcontext_DecRef(context);
}

/* alcMakeContextCurrent
 *
 * Makes the given context the active process-wide context, and removes the
 * thread-local context for the calling thread.
 */
ALC_API ALCboolean ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context)
{
    ALCcontext* temp_context;

    /* context must be valid or NULL */
    if(context && !VerifyContext(&context))
    {
        return ALC_FALSE;
    }
    /* context's reference count is already incremented */
    temp_context = context;
    context = GlobalContext;
    GlobalContext = temp_context;
    if(context) ALCcontext_DecRef(context);

    if((context=LocalContext) != NULL)
    {
        LocalContext = NULL;
        ALCcontext_DecRef(context);
    }

    return ALC_TRUE;
}

/* alcOpenDevice
 *
 * Opens the named device.
 */
ALC_API ALCdevice* ALC_APIENTRY alcOpenDevice(const ALCchar *deviceName)
{
    ALCdevice *device;
    ALCsizei i;

    DO_INITCONFIG();

    if (DeviceList)
    {
        return NULL;
    }

    if(deviceName && (!deviceName[0] || strcasecmp(deviceName, alcDefaultName) == 0 || strcasecmp(deviceName, "openal-soft") == 0
#ifdef _WIN32
        /* Some old Windows apps hardcode these expecting OpenAL to use a
         * specific audio API, even when they're not enumerated. Creative's
         * router effectively ignores them too.
         */
        || strcasecmp(deviceName, "DirectSound3D") == 0 || strcasecmp(deviceName, "DirectSound") == 0
        || strcasecmp(deviceName, "MMSYSTEM") == 0
#endif
    ))
        deviceName = NULL;

    device = al_calloc(16, sizeof(ALCdevice));
    if(!device)
    {
        return NULL;
    }

    //Validate device
    device->ref = 1;

    device->Flags = 0;
    device->Dry.Buffer = NULL;
    device->Dry.NumChannels = 0;
    device->FOAOut.Buffer = NULL;
    device->FOAOut.NumChannels = 0;
    device->RealOut.Buffer = NULL;
    device->RealOut.NumChannels = 0;
    device->Limiter = NULL;

    device->ContextList = NULL;

    device->AuxiliaryEffectSlotMax = 64;
    device->NumAuxSends = DEFAULT_SENDS;

    for(i = 0;i < MAX_OUTPUT_CHANNELS;i++)
    {
        device->ChannelDelay[i].Gain   = 1.0f;
        device->ChannelDelay[i].Length = 0;
        device->ChannelDelay[i].Buffer = NULL;
    }

    //Set output format
    device->FmtChans = DevFmtChannelsDefault;
    device->FmtType = DevFmtTypeDefault;
    device->Frequency = DEFAULT_OUTPUT_RATE;
    device->UpdateSize = clampu(1024, 64, 8192);

    if(device->AuxiliaryEffectSlotMax == 0) device->AuxiliaryEffectSlotMax = 64;

    device->Limiter = CreateDeviceLimiter(device);

    device->source = al_calloc(16, sizeof(ALsource));
    InitSourceParams(device->source, device->NumAuxSends);

    device->effect_slot = al_calloc(16, sizeof(ALeffectslot));
    InitEffectSlot(device->effect_slot);
    aluInitEffectPanning(device->effect_slot);

    device->effect = al_calloc(16, sizeof(ALeffect));
    InitEffect(device->effect);

    DeviceList = device;

    return device;
}

/* alcCloseDevice
 *
 * Closes the given device.
 */
ALC_API ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device)
{
    ALCdevice *iter;
    ALCcontext *ctx;

    iter = DeviceList;
    if(!iter)
    {
        return ALC_FALSE;
    }

    ctx = device->ContextList;
    if(ctx != NULL)
    {
        ReleaseContext(ctx, device);
    }
    device->Flags &= ~DEVICE_RUNNING;

    ALCdevice_DecRef(device);

    return ALC_TRUE;
}
