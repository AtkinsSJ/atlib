/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AnimatedSprite.h"

#include <Gfx/Sprite.h>

OwnedRef<SpriteAnimation> SpriteAnimation::make_placeholder()
{
    auto sprite_animation = adopt_own(*new SpriteAnimation(
        {},
        asset_manager().get_placeholder_asset(SpriteGroup::asset_type()).get_ref<SpriteGroup>(),
        asset_manager().allocate_filled_array<u32>(1, 0),
        0.1f));

    return sprite_animation;
}

SpriteAnimation::SpriteAnimation(String name, TypedAssetRef<SpriteGroup> group, Array<u32> frames, float seconds_per_frame)
    : m_name(move(name))
    , m_group(move(group))
    , m_frames(move(frames))
    , m_seconds_per_frame(seconds_per_frame)
{
}

SpriteAnimation::~SpriteAnimation() = default;

void SpriteAnimation::unload(AssetMetadata&)
{
    asset_manager().deallocate(m_frames);
}

AnimatedSprite SpriteAnimation::play() const
{
    return AnimatedSprite { get_ref(m_name) };
}

Sprite const& SpriteAnimation::get_sprite_at_time(float t) const
{
    auto frame_index = floor_s32(t / m_seconds_per_frame) % m_frames.count();
    auto index_in_sprite_group = m_frames[frame_index];
    return m_group.get().get_sprite(index_in_sprite_group);
}

AnimatedSprite::AnimatedSprite(TypedAssetRef<SpriteAnimation>&& animation)
    : m_animation(animation)
{
}

Sprite AnimatedSprite::animate(float delta_time)
{
    m_t += delta_time;
    return m_animation.get().get_sprite_at_time(m_t);
}
