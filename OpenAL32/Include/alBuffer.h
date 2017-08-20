#ifndef _AL_BUFFER_H_
#define _AL_BUFFER_H_

#include "alMain.h"

#ifdef __cplusplus
extern "C" {
#endif

/* User formats */
enum UserFmtType {
    UserFmtByte   = AL_BYTE_SOFT,
    UserFmtUByte  = AL_UNSIGNED_BYTE_SOFT,
    UserFmtShort  = AL_SHORT_SOFT,
    UserFmtUShort = AL_UNSIGNED_SHORT_SOFT,
    UserFmtFloat  = AL_FLOAT_SOFT
};
enum UserFmtChannels {
    UserFmtMono      = AL_MONO_SOFT,
    UserFmtStereo    = AL_STEREO_SOFT
};

ALsizei BytesFromUserFmt(enum UserFmtType type);
ALsizei ChannelsFromUserFmt(enum UserFmtChannels chans);
inline ALsizei FrameSizeFromUserFmt(enum UserFmtChannels chans, enum UserFmtType type)
{
    return ChannelsFromUserFmt(chans) * BytesFromUserFmt(type);
}


/* Storable formats */
enum FmtType {
    FmtByte  = UserFmtByte,
    FmtShort = UserFmtShort,
    FmtFloat = UserFmtFloat,
};
enum FmtChannels {
    FmtMono   = UserFmtMono,
    FmtStereo = UserFmtStereo
};
#define MAX_INPUT_CHANNELS  (8)

ALsizei BytesFromFmt(enum FmtType type);
ALsizei ChannelsFromFmt(enum FmtChannels chans);
inline ALsizei FrameSizeFromFmt(enum FmtChannels chans, enum FmtType type)
{
    return ChannelsFromFmt(chans) * BytesFromFmt(type);
}


typedef struct ALbuffer {
    ALvoid  *data;

    ALsizei  Frequency;
    ALenum   Format;
    ALsizei  SampleLen;

    enum FmtChannels FmtChannels;
    enum FmtType     FmtType;
    ALuint BytesAlloc;

    enum UserFmtChannels OriginalChannels;
    enum UserFmtType     OriginalType;
    ALsizei              OriginalSize;
    ALsizei              OriginalAlign;

    ALsizei LoopStart;
    ALsizei LoopEnd;

    ALsizei UnpackAlign;
    ALsizei PackAlign;

    /* Number of times buffer was attached to a source (deletion can only occur when 0) */
    unsigned int ref;

    /* Self ID */
    ALuint id;
} ALbuffer;

ALbuffer *NewBuffer(ALCcontext *context);
void DeleteBuffer(ALCdevice *device, ALbuffer *buffer);

ALenum LoadData(ALbuffer *buffer, ALuint freq, ALenum NewFormat, ALsizei frames, enum UserFmtChannels SrcChannels, enum UserFmtType SrcType, const ALvoid *data, ALsizei align, ALboolean storesrc);


inline struct ALbuffer *LookupBuffer(ALCdevice *device, ALuint id)
{ return (struct ALbuffer*)LookupUIntMapKeyNoLock(&device->BufferMap, id); }
inline struct ALbuffer *RemoveBuffer(ALCdevice *device, ALuint id)
{ return (struct ALbuffer*)RemoveUIntMapKeyNoLock(&device->BufferMap, id); }

ALvoid ReleaseALBuffers(ALCdevice *device);

#ifdef __cplusplus
}
#endif

#endif
