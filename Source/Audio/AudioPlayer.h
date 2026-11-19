/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL3_mixer/SDL_mixer.h>

namespace Audio {

class AudioPlayer {
public:
    static bool init();
    static AudioPlayer& the();

    ~AudioPlayer();

private:
    explicit AudioPlayer(MIX_Mixer&);

    MIX_Mixer& m_mixer;
};

}
