/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Texture.h"
#include <Assets/AssetManager.h>
#include <SDL3_image/SDL_image.h>

OwnedRef<Texture> Texture::make_placeholder()
{
    auto pixels = temp_arena().allocate_filled_array<u32>(2 * 2);
    pixels[0] = pixels[3] = 0xffff00ff;
    pixels[1] = pixels[2] = 0xff000000;
    auto* surface = SDL_CreateSurfaceFrom(2, 2, SDL_PIXELFORMAT_ABGR8888, pixels.raw_items(), 2 * sizeof(u32));

    return adopt_own(*new Texture(surface));
}

// FIXME: Duplicated in the Cursor loading code. Probably we want a single Image asset which Cursor and Texture both refer to.
static ErrorOr<SDL_Surface*> createSurfaceFromFileData(Blob fileData, String name)
{
    SDL_Surface* result = nullptr;

    ASSERT(fileData.size() > 0);      //, "Attempted to create a surface from an unloaded asset! ({0})", {name});
    ASSERT(fileData.size() < s32Max); //, "File '{0}' is too big for SDL's RWOps!", {name});

    SDL_IOStream* rw = SDL_IOFromConstMem(fileData.data(), truncate32(fileData.size()));
    if (rw) {
        result = IMG_Load_IO(rw, 0);

        if (result == nullptr) {
            return Error { myprintf("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(SDL_GetError()) }) };
        }

        SDL_CloseIO(rw);
    } else {
        return Error { myprintf("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(SDL_GetError()) }) };
    }

    return result;
}

ErrorOr<OwnedRef<Texture>> Texture::load(AssetMetadata& metadata, Blob file_data)
{
    auto surface_or_error = createSurfaceFromFileData(file_data, metadata.fullName);
    if (surface_or_error.is_error())
        return surface_or_error.release_error();

    auto* surface = surface_or_error.release_value();
    auto surface_format = SDL_GetPixelFormatDetails(surface->format);

    if (surface_format->bits_per_pixel != 32) {
        auto error = Error { myprintf("Texture asset '{0}' is not 32bit, which is all we support right now. (format = {1}bit)"_s, { metadata.shortName, formatInt(surface_format->bits_per_pixel) }) };
        SDL_DestroySurface(surface);
        return error;
    }

    // Premultiply alpha
    // NOTE: We always assume the data isn't premultiplied.
    u32 Rmask = surface_format->Rmask,
        Gmask = surface_format->Gmask,
        Bmask = surface_format->Bmask,
        Amask = surface_format->Amask;
    float rRmask = (float)Rmask,
          rGmask = (float)Gmask,
          rBmask = (float)Bmask,
          rAmask = (float)Amask;

    u32 pixelCount = surface->w * surface->h;
    for (u32 pixelIndex = 0;
        pixelIndex < pixelCount;
        pixelIndex++) {
        u32 pixel = ((u32*)surface->pixels)[pixelIndex];
        float rr = (float)(pixel & Rmask) / rRmask;
        float rg = (float)(pixel & Gmask) / rGmask;
        float rb = (float)(pixel & Bmask) / rBmask;
        float ra = (float)(pixel & Amask) / rAmask;

        u32 r = (u32)(rr * ra * rRmask) & Rmask;
        u32 g = (u32)(rg * ra * rGmask) & Gmask;
        u32 b = (u32)(rb * ra * rBmask) & Bmask;
        u32 a = (u32)(ra * rAmask) & Amask;

        ((u32*)surface->pixels)[pixelIndex] = r | g | b | a;
    }

    return adopt_own(*new Texture(surface));
}

Texture::Texture(SDL_Surface* surface)
    : surface(surface)
{
}

void Texture::unload(AssetMetadata&)
{
    if (surface != nullptr) {
        SDL_DestroySurface(surface);
        surface = nullptr;
    }
}
