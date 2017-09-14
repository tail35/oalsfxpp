/**
 * OpenAL cross platform audio library
 * Copyright (C) 2013 by Mike Gorchak
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


#include <algorithm>
#include <array>
#include <vector>
#include "config.h"
#include "alFilter.h"
#include "alAuxEffectSlot.h"


class ChorusEffectState :
    public EffectState
{
public:
    ChorusEffectState()
        :
        EffectState{},
        sample_buffers_{},
        buffer_length_{},
        offset_{},
        lfo_range_{},
        lfo_scale_{},
        lfo_disp_{},
        gains_{},
        waveform_{},
        delay_{},
        depth_{},
        feedback_{}
    {
    }

    virtual ~ChorusEffectState()
    {
    }


protected:
    void ChorusEffectState::do_construct() final
    {
        buffer_length_ = 0;

        for (auto& buffer : sample_buffers_)
        {
            buffer = SampleBuffer{};
        }

        offset_ = 0;
        lfo_range_ = 1;
        waveform_ = Waveform::triangle;
    }

    void ChorusEffectState::do_destruct() final
    {
        for (auto& buffer : sample_buffers_)
        {
            buffer = SampleBuffer{};
        }
    }

    void ChorusEffectState::do_update_device(
        ALCdevice* device) final
    {
        auto max_len = static_cast<int>(EffectProps::Chorus::max_delay * 2.0F * device->frequency_) + 1;

        max_len = next_power_of_2(max_len);

        if (max_len != buffer_length_)
        {
            sample_buffers_[0].resize(max_len);
            sample_buffers_[1].resize(max_len);

            buffer_length_ = max_len;
        }

        for (auto& buffer : sample_buffers_)
        {
            std::fill(buffer.begin(), buffer.end(), 0.0F);
        }
    }

    void ChorusEffectState::do_update(
        ALCdevice* device,
        const EffectSlot* slot,
        const EffectProps* props) final
    {
        const auto frequency = static_cast<float>(device->frequency_);

        switch (props->chorus_.waveform_)
        {
        case EffectProps::Chorus::waveform_triangle:
            waveform_ = Waveform::triangle;
            break;

        case EffectProps::Chorus::waveform_sinusoid:
            waveform_ = Waveform::sinusoid;
            break;
        }

        feedback_ = props->chorus_.feedback_;
        delay_ = static_cast<int>(props->chorus_.delay_ * frequency);

        // The LFO depth is scaled to be relative to the sample delay.
        depth_ = props->chorus_.depth_ * delay_;

        float coeffs[max_ambi_coeffs];

        // Gains for left and right sides
        Panning::calc_angle_coeffs(-Math::pi_2, 0.0F, 0.0F, coeffs);
        Panning::compute_panning_gains(device->channel_count_, device->dry_, coeffs, 1.0F, gains_[0].data());
        Panning::calc_angle_coeffs(Math::pi_2, 0.0F, 0.0F, coeffs);
        Panning::compute_panning_gains(device->channel_count_, device->dry_, coeffs, 1.0F, gains_[1].data());

        const auto phase = props->chorus_.phase_;
        const auto rate = props->chorus_.rate_;

        if (!(rate > 0.0F))
        {
            lfo_scale_ = 0.0F;
            lfo_range_ = 1;
            lfo_disp_ = 0;
        }
        else
        {
            // Calculate LFO coefficient
            lfo_range_ = static_cast<int>(frequency / rate + 0.5F);

            switch (waveform_)
            {
            case Waveform::triangle:
                lfo_scale_ = 4.0F / lfo_range_;
                break;

            case Waveform::sinusoid:
                lfo_scale_ = Math::tau / lfo_range_;
                break;
            }

            // Calculate lfo phase displacement
            if (phase >= 0)
            {
                lfo_disp_ = static_cast<int>(lfo_range_ * (phase / 360.0F));
            }
            else
            {
                lfo_disp_ = static_cast<int>(lfo_range_ * ((360 + phase) / 360.0F));
            }
        }
    }

    void ChorusEffectState::do_process(
        const int sample_count,
        const SampleBuffers& src_samples,
        SampleBuffers& dst_samples,
        const int channel_count) final
    {
        auto& left_buf = sample_buffers_[0];
        auto& right_buf = sample_buffers_[1];
        const auto buf_mask = buffer_length_ - 1;

        for (int base = 0; base < sample_count; )
        {
            float temps[128][2];
            int mod_delays[2][128];
            const auto todo = std::min(128, sample_count - base);

            switch (waveform_)
            {
            case Waveform::triangle:
                GetTriangleDelays(
                    mod_delays[0],
                    offset_ % lfo_range_,
                    lfo_range_,
                    lfo_scale_,
                    depth_,
                    delay_,
                    todo);

                GetTriangleDelays(
                    mod_delays[1],
                    (offset_ + lfo_disp_) % lfo_range_,
                    lfo_range_,
                    lfo_scale_,
                    depth_,
                    delay_,
                    todo);

                break;

            case Waveform::sinusoid:
                GetSinusoidDelays(
                    mod_delays[0],
                    offset_ % lfo_range_,
                    lfo_range_,
                    lfo_scale_,
                    depth_,
                    delay_,
                    todo);

                GetSinusoidDelays(
                    mod_delays[1],
                    (offset_ + lfo_disp_) % lfo_range_,
                    lfo_range_,
                    lfo_scale_,
                    depth_,
                    delay_,
                    todo);

                break;
            }

            for (int i = 0; i < todo; ++i)
            {
                left_buf[offset_ & buf_mask] = src_samples[0][base + i];
                temps[i][0] = left_buf[(offset_ - mod_delays[0][i]) & buf_mask] * feedback_;
                left_buf[offset_ & buf_mask] += temps[i][0];

                right_buf[offset_ & buf_mask] = src_samples[0][base + i];
                temps[i][1] = right_buf[(offset_ - mod_delays[1][i]) & buf_mask] * feedback_;
                right_buf[offset_ & buf_mask] += temps[i][1];

                offset_ += 1;
            }

            for (int c = 0; c < channel_count; ++c)
            {
                auto channel_gain = gains_[0][c];

                if (std::abs(channel_gain) > silence_threshold_gain)
                {
                    for (int i = 0; i < todo; ++i)
                    {
                        dst_samples[c][i + base] += temps[i][0] * channel_gain;
                    }
                }

                channel_gain = gains_[1][c];

                if (std::abs(channel_gain) > silence_threshold_gain)
                {
                    for (int i = 0; i < todo; ++i)
                    {
                        dst_samples[c][i + base] += temps[i][1] * channel_gain;
                    }
                }
            }

            base += todo;
        }
    }


private:
    enum class Waveform
    {
        triangle = EffectProps::Chorus::waveform_triangle,
        sinusoid = EffectProps::Chorus::waveform_sinusoid
    }; // Waveform


    using SampleBuffer = EffectSampleBuffer;
    using SampleBuffers = std::array<SampleBuffer, 2>;

    using Gains = MdArray<float, 2, max_channels>;


    SampleBuffers sample_buffers_;
    int buffer_length_;
    int offset_;
    int lfo_range_;
    float lfo_scale_;
    int lfo_disp_;

    // Gains for left and right sides
    Gains gains_;

    // effect parameters
    Waveform waveform_;
    int delay_;
    float depth_;
    float feedback_;


    static void GetTriangleDelays(
        int* delays,
        int offset,
        const int lfo_range,
        const float lfo_scale,
        const float depth,
        const int delay,
        const int todo)
    {
        for (int i = 0; i < todo; ++i)
        {
            delays[i] = static_cast<int>((1.0F - std::abs(2.0F - (lfo_scale * offset))) * depth) + delay;
            offset = (offset + 1) % lfo_range;
        }
    }

    static void GetSinusoidDelays(
        int* delays,
        int offset,
        const int lfo_range,
        const float lfo_scale,
        const float depth,
        const int delay,
        const int todo)
    {
        for (int i = 0; i < todo; ++i)
        {
            delays[i] = static_cast<int>(std::sin(lfo_scale * offset) * depth) + delay;
            offset = (offset + 1) % lfo_range;
        }
    }
}; // ChorusEffectState


EffectState* EffectStateFactory::create_chorus()
{
    return create<ChorusEffectState>();
}
