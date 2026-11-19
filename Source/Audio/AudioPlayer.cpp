/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AudioPlayer.h"

#include <Util/Assert.h>
#include <Util/Log.h>
#include <Util/String.h>

namespace Audio {

static AudioPlayer* s_audio_player = nullptr;

bool AudioPlayer::init()
{
    auto* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mixer) {
        logCritical("Failed to create mixer device for AudioPlayer: {0}"_s, { String::from_null_terminated(SDL_GetError()) });
        return false;
    }

    s_audio_player = new AudioPlayer(*mixer);
    return true;
}

AudioPlayer& AudioPlayer::the()
{
    ASSERT(s_audio_player);
    return *s_audio_player;
}

AudioPlayer::~AudioPlayer()
{
    MIX_DestroyMixer(&m_mixer);
}

AudioPlayer::AudioPlayer(MIX_Mixer& mixer)
    : m_mixer(mixer)
{
}

}
