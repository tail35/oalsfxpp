#ifndef _AL_EFFECT_H_
#define _AL_EFFECT_H_

#include <vector>
#include "alMain.h"


using EffectSampleBuffer = std::vector<ALfloat>;


union ALeffectProps
{
    struct Reverb
    {
        // Shared Reverb Properties
        ALfloat density;
        ALfloat diffusion;
        ALfloat gain;
        ALfloat gain_hf;
        ALfloat decay_time;
        ALfloat decay_hf_ratio;
        ALfloat reflections_gain;
        ALfloat reflections_delay;
        ALfloat late_reverb_gain;
        ALfloat late_reverb_delay;
        ALfloat air_absorption_gain_hf;
        ALfloat room_rolloff_factor;
        ALboolean decay_hf_limit;

        // Additional EAX Reverb Properties
        ALfloat gain_lf;
        ALfloat decay_lf_ratio;
        ALfloat reflections_pan[3];
        ALfloat late_reverb_pan[3];
        ALfloat echo_time;
        ALfloat echo_depth;
        ALfloat modulation_time;
        ALfloat modulation_depth;
        ALfloat hf_reference;
        ALfloat lf_reference;
    }; // Reverb

    struct Chorus
    {
        ALint waveform;
        ALint phase;
        ALfloat rate;
        ALfloat depth;
        ALfloat feedback;
        ALfloat delay;
    }; // Chorus

    struct Compressor
    {
        ALboolean on_off;
    }; // Compressor

    struct Distortion
    {
        ALfloat edge;
        ALfloat gain;
        ALfloat lowpass_cutoff;
        ALfloat eq_center;
        ALfloat eq_bandwidth;
    }; // Distortion

    struct Echo
    {
        ALfloat delay;
        ALfloat lr_delay;

        ALfloat damping;
        ALfloat feedback;

        ALfloat spread;
    }; // Echo

    struct Equalizer
    {
        ALfloat low_cutoff;
        ALfloat low_gain;
        ALfloat mid1_center;
        ALfloat mid1_gain;
        ALfloat mid1_width;
        ALfloat mid2_center;
        ALfloat mid2_gain;
        ALfloat mid2_width;
        ALfloat high_cutoff;
        ALfloat high_gain;
    }; // Equalizer

    struct Flanger
    {
        ALint waveform;
        ALint phase;
        ALfloat rate;
        ALfloat depth;
        ALfloat feedback;
        ALfloat delay;
    }; // Flanger

    struct Modulator
    {
        ALfloat frequency;
        ALfloat high_pass_cutoff;
        ALint waveform;
    }; // Modulator

    struct Dedicated
    {
        ALfloat gain;
    }; // Dedicated


    Reverb reverb;
    Chorus chorus;
    Compressor compressor;
    Distortion distortion;
    Echo echo;
    Equalizer equalizer;
    Flanger flanger;
    Modulator modulator;
    Dedicated dedicated;
}; // ALeffectProps

struct ALeffect
{
    // Effect type (AL_EFFECT_NULL, ...)
    ALenum type;

    ALeffectProps props;
}; // ALeffect


class IEffect
{
public:
    IEffect(
        const IEffect& that) = delete;

    IEffect& operator=(
        const IEffect& that) = delete;

    virtual ~IEffect();


    SampleBuffers* out_buffer;
    ALsizei out_channels;


    void construct();

    void destruct();

    ALboolean update_device(
        ALCdevice* device);

    void update(
        ALCdevice* device,
        const struct ALeffectslot* slot,
        const union ALeffectProps *props);

    void process(
        const ALsizei sample_count,
        const SampleBuffers& src_samples,
        SampleBuffers& dst_samples,
        const ALsizei channel_count);


protected:
    IEffect();


    virtual void do_construct() = 0;

    virtual void do_destruct() = 0;

    virtual ALboolean do_update_device(
        ALCdevice* device) = 0;

    virtual void do_update(
        ALCdevice* device,
        const struct ALeffectslot* slot,
        const union ALeffectProps *props) = 0;

    virtual void do_process(
        const ALsizei sample_count,
        const SampleBuffers& src_samples,
        SampleBuffers& dst_samples,
        const ALsizei channel_count) = 0;
}; // IEffect


ALenum InitEffect(ALeffect *effect);


#endif
