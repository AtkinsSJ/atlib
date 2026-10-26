/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "App.h"
#include <App/Scene.h>
#include <UI/Window.h>
#include <Util/Log.h>
#include <Util/Random.h>
#include <Util/String.h>

static OwnedPtr<App> s_app = nullptr;

App& App::initialize(float seconds_per_frame, OwnedRef<Scene> scene)
{
    logInfo("Constructing app"_s);
    ASSERT(s_app == nullptr);
    s_app = OwnedPtr<App>::adopt(new App(seconds_per_frame, move(scene)));
    return *s_app;
}

void App::destruct()
{
    // FIXME: This is a stop-gap measure until everything is fixed up.
    //        We shouldn't need to close the app manually before the Renderer, but we currently do.
    s_app = nullptr;
}

App& App::the()
{
    ASSERT(s_app != nullptr);
    return *s_app;
}

App::App(float seconds_per_frame, OwnedRef<Scene> scene)
    : m_delta_time(seconds_per_frame)
    , m_cosmetic_random(Random::create())
    , m_scene(move(scene))
{
}

App::~App()
{
    logInfo("Destructing app"_s);
}

void App::switch_to_scene(OwnedRef<Scene> scene)
{
    m_next_scene = move(scene);
}

void App::transition_to_next_scene_if_needed()
{
    if (!m_next_scene)
        return;
    UI::closeAllWindows();
    m_scene = m_next_scene.release_nonnull();
}

Scene& App::scene() const
{
    return *m_scene;
}
