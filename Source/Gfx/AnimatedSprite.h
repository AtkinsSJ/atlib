/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Assets/AssetRef.h>
#include <Gfx/Forward.h>

class SpriteAnimation final : public Asset {
    ASSET_SUBCLASS_METHODS(SpriteAnimation);

public:
    static OwnedRef<SpriteAnimation> make_placeholder();
    SpriteAnimation(String name, TypedAssetRef<SpriteGroup> group, Array<u32> frames, float seconds_per_frame);
    virtual ~SpriteAnimation() override;

    virtual void unload(AssetMetadata& metadata) override;

    AnimatedSprite play() const;
    Sprite const& get_sprite_at_time(float t) const;

private:
    // FIXME: Ideally, all assets would have a "get ref to me" method but for now we just want one here.
    String m_name;
    TypedAssetRef<SpriteGroup> m_group;
    Array<u32> m_frames;
    float m_seconds_per_frame;
};

class AnimatedSprite {
public:
    explicit AnimatedSprite(TypedAssetRef<SpriteAnimation>&&);
    Sprite animate(float delta_time);

private:
    TypedAssetRef<SpriteAnimation> m_animation;
    float m_t { 0 };
};
