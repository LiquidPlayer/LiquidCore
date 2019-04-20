/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "SharedWrap.h"

Queue<SharedWrapBase*> SharedWrapBase::s_zombies;
static volatile bool stop = false;

void SharedWrapBase::FreeZombiesThread()
{
    while(!stop) {
        auto wrap = s_zombies.pop();
        delete wrap;
    };
}

static std::thread FreeZombies(SharedWrapBase::FreeZombiesThread);