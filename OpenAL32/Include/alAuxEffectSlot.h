#ifndef _AL_AUXEFFECTSLOT_H_
#define _AL_AUXEFFECTSLOT_H_

#include "alMain.h"
#include "alEffect.h"

#include "align.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ALeffectStateVtable;
struct ALeffectslot;

typedef struct ALeffectState {
    unsigned int ref;
    const struct ALeffectStateVtable *vtbl;

    ALfloat (*out_buffer)[BUFFERSIZE];
    ALsizei out_channels;
} ALeffectState;

void ALeffectState_Construct(ALeffectState *state);
void ALeffectState_Destruct(ALeffectState *state);

struct ALeffectStateVtable {
    void (*const destruct)(ALeffectState *state);

    ALboolean (*const device_update)(ALeffectState *state, ALCdevice *device);
    void (*const update)(ALeffectState *state, const ALCdevice *device, const struct ALeffectslot *slot, const union ALeffectProps *props);
    void (*const process)(ALeffectState *state, ALsizei samplesToDo, const ALfloat (*restrict samplesIn)[BUFFERSIZE], ALfloat (*restrict samplesOut)[BUFFERSIZE], ALsizei numChannels);

    void (*const delete1)(void *ptr);
};

#define DEFINE_ALEFFECTSTATE_VTABLE(T)                                        \
DECLARE_THUNK(T, ALeffectState, void, Destruct)                               \
DECLARE_THUNK1(T, ALeffectState, ALboolean, deviceUpdate, ALCdevice*)         \
DECLARE_THUNK3(T, ALeffectState, void, update, const ALCdevice*, const ALeffectslot*, const ALeffectProps*) \
DECLARE_THUNK4(T, ALeffectState, void, process, ALsizei, const ALfloatBUFFERSIZE*restrict, ALfloatBUFFERSIZE*restrict, ALsizei) \
static void T##_ALeffectState_Delete(void *ptr)                               \
{ return T##_Delete(STATIC_UPCAST(T, ALeffectState, (ALeffectState*)ptr)); }  \
                                                                              \
static const struct ALeffectStateVtable T##_ALeffectState_vtable = {          \
    T##_ALeffectState_Destruct,                                               \
                                                                              \
    T##_ALeffectState_deviceUpdate,                                           \
    T##_ALeffectState_update,                                                 \
    T##_ALeffectState_process,                                                \
                                                                              \
    T##_ALeffectState_Delete,                                                 \
}


struct ALeffectStateFactoryVtable;

typedef struct ALeffectStateFactory {
    const struct ALeffectStateFactoryVtable *vtbl;
} ALeffectStateFactory;

struct ALeffectStateFactoryVtable {
    ALeffectState *(*const create)(ALeffectStateFactory *factory);
};

#define DEFINE_ALEFFECTSTATEFACTORY_VTABLE(T)                                 \
DECLARE_THUNK(T, ALeffectStateFactory, ALeffectState*, create)                \
                                                                              \
static const struct ALeffectStateFactoryVtable T##_ALeffectStateFactory_vtable = { \
    T##_ALeffectStateFactory_create,                                          \
}


#define MAX_EFFECT_CHANNELS (4)


struct ALeffectslotArray {
    ALsizei count;
    struct ALeffectslot *slot[];
};


struct ALeffectslotProps {
    ALenum type;
    ALeffectProps props;

    ALeffectState *state;

    struct ALeffectslotProps* next;
};


typedef struct ALeffectslot {
    struct Effect {
        ALenum type;
        ALeffectProps props;

        ALeffectState *state;
    } effect;

    unsigned int ref;

    struct ALeffectslotProps* update;
    struct ALeffectslotProps* free_list;

    struct Params {
        ALenum effect_type;
        ALeffectState *effect_state;
    } params;

    ALsizei num_channels;
    BFChannelConfig chan_map[MAX_EFFECT_CHANNELS];
    /* Wet buffer configuration is ACN channel order with N3D scaling:
     * * Channel 0 is the unattenuated mono signal.
     * * Channel 1 is OpenAL -X
     * * Channel 2 is OpenAL Y
     * * Channel 3 is OpenAL -Z
     * Consequently, effects that only want to work with mono input can use
     * channel 0 by itself. Effects that want multichannel can process the
     * ambisonics signal and make a B-Format pan (ComputeFirstOrderGains) for
     * first-order device output (FOAOut).
     */
    alignas(16) ALfloat wet_buffer[MAX_EFFECT_CHANNELS][BUFFERSIZE];
} ALeffectslot;

ALenum InitEffectSlot(ALeffectslot *slot);
void DeinitEffectSlot(ALeffectslot *slot);
void UpdateEffectSlotProps(ALeffectslot *slot);
void UpdateAllEffectSlotProps(ALCcontext *context);


ALeffectStateFactory *ALnullStateFactory_getFactory(void);
ALeffectStateFactory *ALreverbStateFactory_getFactory(void);
ALeffectStateFactory *ALchorusStateFactory_getFactory(void);
ALeffectStateFactory *ALcompressorStateFactory_getFactory(void);
ALeffectStateFactory *ALdistortionStateFactory_getFactory(void);
ALeffectStateFactory *ALechoStateFactory_getFactory(void);
ALeffectStateFactory *ALequalizerStateFactory_getFactory(void);
ALeffectStateFactory *ALflangerStateFactory_getFactory(void);
ALeffectStateFactory *ALmodulatorStateFactory_getFactory(void);
ALeffectStateFactory *ALdedicatedStateFactory_getFactory(void);


ALenum InitializeEffect(ALCdevice *Device, ALeffectslot *EffectSlot, ALeffect *effect);


#ifdef __cplusplus
}
#endif

#endif
