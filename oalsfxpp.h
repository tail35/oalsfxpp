#ifndef OALSFXPP_INCLUDED
#define OALSFXPP_INCLUDED


#include <array>
#include <memory>


enum class ChannelFormat
{
    none,
    mono,
    stereo,
    quad,
    five_point_one,
    five_point_one_rear,
    six_point_one,
    seven_point_one,
}; // ChannelFormat

enum class EffectType
{
    null,
    chorus,
    compressor,
    dedicated_dialog,
    dedicated_low_frequency,
    distortion,
    echo,
    equalizer,
    flanger,
    ring_modulator,
    reverb,
    eax_reverb,
}; // EffectType


union EffectProps
{
    using Pan = std::array<float, 3>;


    struct Chorus
    {
        static constexpr auto waveform_sinusoid = 0;
        static constexpr auto waveform_triangle = 1;

        static constexpr auto min_waveform = waveform_sinusoid;
        static constexpr auto max_waveform = waveform_triangle;
        static constexpr auto default_waveform = waveform_triangle;

        static constexpr auto min_phase = -180;
        static constexpr auto max_phase = 180;
        static constexpr auto default_phase = 90;

        static constexpr auto min_rate = 0.0F;
        static constexpr auto max_rate = 10.0F;
        static constexpr auto default_rate = 1.1F;

        static constexpr auto min_depth = 0.0F;
        static constexpr auto max_depth = 1.0F;
        static constexpr auto default_depth = 0.1F;

        static constexpr auto min_feedback = -1.0F;
        static constexpr auto max_feedback = 1.0F;
        static constexpr auto default_feedback = 0.25F;

        static constexpr auto min_delay = 0.0F;
        static constexpr auto max_delay = 0.016F;
        static constexpr auto default_delay = 0.016F;


        int waveform_;
        int phase_;
        float rate_;
        float depth_;
        float feedback_;
        float delay_;


        void set_defaults();

        void normalize();

        static bool are_equal(
            const Chorus& a,
            const Chorus& b);
    }; // Chorus

    struct Compressor
    {
        static constexpr auto min_on_off = false;
        static constexpr auto max_on_off = true;
        static constexpr auto default_on_off = true;


        bool on_off_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Compressor& a,
            const Compressor& b);
    }; // Compressor

    struct Dedicated
    {
        static constexpr auto min_gain = 0.0F;
        static constexpr auto max_gain = 1.0F;
        static constexpr auto default_gain = 1.0F;


        float gain_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Dedicated& a,
            const Dedicated& b);
    }; // Dedicated

    struct Distortion
    {
        static constexpr auto min_edge = 0.0F;
        static constexpr auto max_edge = 1.0F;
        static constexpr auto default_edge = 0.2F;

        static constexpr auto min_gain = 0.01F;
        static constexpr auto max_gain = 1.0F;
        static constexpr auto default_gain = 0.05F;

        static constexpr auto min_low_pass_cutoff = 80.0F;
        static constexpr auto max_low_pass_cutoff = 24000.0F;
        static constexpr auto default_low_pass_cutoff = 8000.0F;

        static constexpr auto min_eq_center = 80.0F;
        static constexpr auto max_eq_center = 24000.0F;
        static constexpr auto default_eq_center = 3600.0F;

        static constexpr auto min_eq_bandwidth = 80.0F;
        static constexpr auto max_eq_bandwidth = 24000.0F;
        static constexpr auto default_eq_bandwidth = 3600.0F;


        float edge_;
        float gain_;
        float low_pass_cutoff_;
        float eq_center_;
        float eq_bandwidth_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Distortion& a,
            const Distortion& b);
    }; // Distortion

    struct Echo
    {
        static constexpr auto min_delay = 0.0F;
        static constexpr auto max_delay = 0.207F;
        static constexpr auto default_delay = 0.1F;

        static constexpr auto min_lr_delay = 0.0F;
        static constexpr auto max_lr_delay = 0.404F;
        static constexpr auto default_lr_delay = 0.1F;

        static constexpr auto min_damping = 0.0F;
        static constexpr auto max_damping = 0.99F;
        static constexpr auto default_damping = 0.5F;

        static constexpr auto min_feedback = 0.0F;
        static constexpr auto max_feedback = 1.0F;
        static constexpr auto default_feedback = 0.5F;

        static constexpr auto min_spread = -1.0F;
        static constexpr auto max_spread = 1.0F;
        static constexpr auto default_spread = -1.0F;


        float delay_;
        float lr_delay_;

        float damping_;
        float feedback_;

        float spread_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Echo& a,
            const Echo& b);
    }; // Echo

    struct Equalizer
    {
        static constexpr auto min_low_gain = 0.126F;
        static constexpr auto max_low_gain = 7.943F;
        static constexpr auto default_low_gain = 1.0F;

        static constexpr auto min_low_cutoff = 50.0F;
        static constexpr auto max_low_cutoff = 800.0F;
        static constexpr auto default_low_cutoff = 200.0F;

        static constexpr auto min_mid1_gain = 0.126F;
        static constexpr auto max_mid1_gain = 7.943F;
        static constexpr auto default_mid1_gain = 1.0F;

        static constexpr auto min_mid1_center = 200.0F;
        static constexpr auto max_mid1_center = 3000.0F;
        static constexpr auto default_mid1_center = 500.0F;

        static constexpr auto min_mid1_width = 0.01F;
        static constexpr auto max_mid1_width = 1.0F;
        static constexpr auto default_mid1_width = 1.0F;

        static constexpr auto min_mid2_gain = 0.126F;
        static constexpr auto max_mid2_gain = 7.943F;
        static constexpr auto default_mid2_gain = 1.0F;

        static constexpr auto min_mid2_center = 1000.0F;
        static constexpr auto max_mid2_center = 8000.0F;
        static constexpr auto default_mid2_center = 3000.0F;

        static constexpr auto min_mid2_width = 0.01F;
        static constexpr auto max_mid2_width = 1.0F;
        static constexpr auto default_mid2_width = 1.0F;

        static constexpr auto min_high_gain = 0.126F;
        static constexpr auto max_high_gain = 7.943F;
        static constexpr auto default_high_gain = 1.0F;

        static constexpr auto min_high_cutoff = 4000.0F;
        static constexpr auto max_high_cutoff = 16000.0F;
        static constexpr auto default_high_cutoff = 6000.0F;


        float low_cutoff_;
        float low_gain_;
        float mid1_center_;
        float mid1_gain_;
        float mid1_width_;
        float mid2_center_;
        float mid2_gain_;
        float mid2_width_;
        float high_cutoff_;
        float high_gain_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Equalizer& a,
            const Equalizer& b);
    }; // Equalizer

    struct Flanger
    {
        static constexpr auto waveform_sinusoid = 0;
        static constexpr auto waveform_triangle = 1;

        static constexpr auto min_waveform = waveform_sinusoid;
        static constexpr auto max_waveform = waveform_triangle;
        static constexpr auto default_waveform = waveform_triangle;

        static constexpr auto min_phase = -180;
        static constexpr auto max_phase = 180;
        static constexpr auto default_phase = 0;

        static constexpr auto min_rate = 0.0F;
        static constexpr auto max_rate = 10.0F;
        static constexpr auto default_rate = 0.27F;

        static constexpr auto min_depth = 0.0F;
        static constexpr auto max_depth = 1.0F;
        static constexpr auto default_depth = 1.0F;

        static constexpr auto min_feedback = -1.0F;
        static constexpr auto max_feedback = 1.0F;
        static constexpr auto default_feedback = -0.5F;

        static constexpr auto min_delay = 0.0F;
        static constexpr auto max_delay = 0.004F;
        static constexpr auto default_delay = 0.002F;


        int waveform_;
        int phase_;
        float rate_;
        float depth_;
        float feedback_;
        float delay_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Flanger& a,
            const Flanger& b);
    }; // Flanger

    struct Reverb
    {
        static constexpr auto min_density = 0.0F;
        static constexpr auto max_density = 1.0F;
        static constexpr auto default_density = 1.0F;

        static constexpr auto min_diffusion = 0.0F;
        static constexpr auto max_diffusion = 1.0F;
        static constexpr auto default_diffusion = 1.0F;

        static constexpr auto min_gain = 0.0F;
        static constexpr auto max_gain = 1.0F;
        static constexpr auto default_gain = 0.32F;

        static constexpr auto min_gain_hf = 0.0F;
        static constexpr auto max_gain_hf = 1.0F;
        static constexpr auto default_gain_hf = 0.89F;

        static constexpr auto min_gain_lf = 0.0F;
        static constexpr auto max_gain_lf = 1.0F;
        static constexpr auto default_gain_lf = 1.0F;

        static constexpr auto min_decay_time = 0.1F;
        static constexpr auto max_decay_time = 20.0F;
        static constexpr auto default_decay_time = 1.49F;

        static constexpr auto min_decay_hf_ratio = 0.1F;
        static constexpr auto max_decay_hf_ratio = 2.0F;
        static constexpr auto default_decay_hf_ratio = 0.83F;

        static constexpr auto min_decay_lf_ratio = 0.1F;
        static constexpr auto max_decay_lf_ratio = 2.0F;
        static constexpr auto default_decay_lf_ratio = 1.0F;

        static constexpr auto min_reflections_gain = 0.0F;
        static constexpr auto max_reflections_gain = 3.16F;
        static constexpr auto default_reflections_gain = 0.05F;

        static constexpr auto min_reflections_delay = 0.0F;
        static constexpr auto max_reflections_delay = 0.3F;
        static constexpr auto default_reflections_delay = 0.007F;

        static constexpr auto min_reflections_pan_xyz = -1.0F;
        static constexpr auto max_reflections_pan_xyz = 1.0F;
        static constexpr auto default_reflections_pan_xyz = 0.0F;

        static constexpr auto min_late_reverb_gain = 0.0F;
        static constexpr auto max_late_reverb_gain = 10.0F;
        static constexpr auto default_late_reverb_gain = 1.26F;

        static constexpr auto min_late_reverb_delay = 0.0F;
        static constexpr auto max_late_reverb_delay = 0.1F;
        static constexpr auto default_late_reverb_delay = 0.011F;

        static constexpr auto min_late_reverb_pan_xyz = -1.0F;
        static constexpr auto max_late_reverb_pan_xyz = 1.0F;
        static constexpr auto default_late_reverb_pan_xyz = 0.0F;

        static constexpr auto min_echo_time = 0.075F;
        static constexpr auto max_echo_time = 0.25F;
        static constexpr auto default_echo_time = 0.25F;

        static constexpr auto min_echo_depth = 0.0F;
        static constexpr auto max_echo_depth = 1.0F;
        static constexpr auto default_echo_depth = 0.0F;

        static constexpr auto min_modulation_time = 0.04F;
        static constexpr auto max_modulation_time = 4.0F;
        static constexpr auto default_modulation_time = 0.25F;

        static constexpr auto min_modulation_depth = 0.0F;
        static constexpr auto max_modulation_depth = 1.0F;
        static constexpr auto default_modulation_depth = 0.0F;

        static constexpr auto min_air_absorption_gain_hf = 0.892F;
        static constexpr auto max_air_absorption_gain_hf = 1.0F;
        static constexpr auto default_air_absorption_gain_hf = 0.994F;

        static constexpr auto min_hf_reference = 1000.0F;
        static constexpr auto max_hf_reference = 20000.0F;
        static constexpr auto default_hf_reference = 5000.0F;

        static constexpr auto min_lf_reference = 20.0F;
        static constexpr auto max_lf_reference = 1000.0F;
        static constexpr auto default_lf_reference = 250.0F;

        static constexpr auto min_room_rolloff_factor = 0.0F;
        static constexpr auto max_room_rolloff_factor = 10.0F;
        static constexpr auto default_room_rolloff_factor = 0.0F;

        static constexpr auto min_decay_hf_limit = false;
        static constexpr auto max_decay_hf_limit = true;
        static constexpr auto default_decay_hf_limit = true;


        // Shared reverb properties
        float density_;
        float diffusion_;
        float gain_;
        float gain_hf_;
        float decay_time_;
        float decay_hf_ratio_;
        float reflections_gain_;
        float reflections_delay_;
        float late_reverb_gain_;
        float late_reverb_delay_;
        float air_absorption_gain_hf_;
        float room_rolloff_factor_;
        bool decay_hf_limit_;

        // Additional EAX reverb properties
        float gain_lf_;
        float decay_lf_ratio_;
        Pan reflections_pan_;
        Pan late_reverb_pan_;
        float echo_time_;
        float echo_depth_;
        float modulation_time_;
        float modulation_depth_;
        float hf_reference_;
        float lf_reference_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const Reverb& a,
            const Reverb& b);
    }; // Reverb

    struct RingModulator
    {
        static constexpr auto min_frequency = 0.0F;
        static constexpr auto max_frequency = 8000.0F;
        static constexpr auto default_frequency = 440.0F;

        static constexpr auto min_high_pass_cutoff = 0.0F;
        static constexpr auto max_high_pass_cutoff = 24000.0F;
        static constexpr auto default_high_pass_cutoff = 800.0F;

        static constexpr auto waveform_sinusoid = 0;
        static constexpr auto waveform_sawtooth = 1;
        static constexpr auto waveform_square = 2;

        static constexpr auto min_waveform = waveform_sinusoid;
        static constexpr auto max_waveform = waveform_square;
        static constexpr auto default_waveform = waveform_sinusoid;


        float frequency_;
        float high_pass_cutoff_;
        int waveform_;


        void set_defaults();

        void normalize();


        static bool are_equal(
            const RingModulator& a,
            const RingModulator& b);
    }; // RingModulator


    Chorus chorus_;
    Compressor compressor_;
    Dedicated dedicated_;
    Distortion distortion_;
    Echo echo_;
    Equalizer equalizer_;
    Flanger flanger_;
    Reverb reverb_;
    RingModulator ring_modulator_;
}; // EffectProps

struct Effect
{
    EffectType type_;
    EffectProps props_;


    void set_defaults();

    void set_type_and_defaults(
        const EffectType effect_type);

    void normalize();

    static bool are_equal(
        const Effect& a,
        const Effect& b);
}; // Effect

struct SendProps
{
    static constexpr auto lp_frequency_reference = 5'000.0F;
    static constexpr auto hp_frequency_reference = 250.0F;

    static constexpr auto min_gain = 0.0F;
    static constexpr auto max_gain = 1.0F;
    static constexpr auto default_gain = 1.0F;

    static constexpr auto min_gain_hf = 0.0F;
    static constexpr auto max_gain_hf = 1.0F;
    static constexpr auto default_gain_hf = 1.0F;

    static constexpr auto min_gain_lf = 0.0F;
    static constexpr auto max_gain_lf = 1.0F;
    static constexpr auto default_gain_lf = 1.0F;


    float gain_;
    float gain_hf_;
    float gain_lf_;


    void set_defaults();

    void normalize();


    static bool are_equal(
        const SendProps& a,
        const SendProps& b);
}; // SendProps


class Api
{
public:
    Api();

    Api(
        const Api& that) = delete;

    Api& operator=(
        const Api& that) = delete;

    ~Api();


    bool initialize(
        const ChannelFormat channel_format,
        const int sampling_rate,
        const int effect_count);

    bool is_initialized() const;

    bool set_effect_type(
        const int effect_index,
        const EffectType effect_type);

    bool set_effect_props(
        const int effect_index,
        const EffectProps& effect_props);

    bool set_effect(
        const int effect_index,
        const Effect& effect);

    bool set_send_props(
        const SendProps& send_props);

    bool set_send_props(
        const int effect_index,
        const SendProps& send_props);

    bool apply_changes();

    bool mix(
        const int sample_count,
        const float* src_samples,
        float* dst_samples);

    void uninitialize();


    static int get_min_sampling_rate();

    static int get_max_sampling_rate();

    static int get_min_effect_count();

    static int get_max_effect_count();

    static ChannelFormat channel_count_to_channel_format(
        const int channel_count);


private:
    class Impl;
    using ApiImplUPtr = std::unique_ptr<Impl>;


    ApiImplUPtr pimpl_;
}; // Api


#endif // OALSFXPP_INCLUDED
