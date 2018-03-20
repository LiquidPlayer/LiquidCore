//
//  Locker.cpp
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/9/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "V82JSC.h"

using namespace v8;

Unlocker::~Unlocker()
{
}

Locker::~Locker()
{
    
}

void Unlocker::Initialize(Isolate* isolate)
{
    
}


/**
 * Returns whether or not the locker for a given isolate, is locked by the
 * current thread.
 */
bool Locker::IsLocked(Isolate* isolate)
{
    return false;
}

/**
 * Returns whether v8::Locker is being used by this V8 instance.
 */
bool Locker::IsActive()
{
    return false;
}

void Locker::Initialize(Isolate* isolate)
{
    
}
