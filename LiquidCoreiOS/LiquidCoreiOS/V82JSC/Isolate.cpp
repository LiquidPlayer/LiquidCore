//
//  Isolate.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Isolate.h"

using namespace v8;

void Isolate::Enter()
{
    
}

void Isolate::Exit()
{
    
}

void Isolate::Dispose()
{
    IsolateImpl *isolate = (IsolateImpl *)this;
    JSContextGroupRelease(isolate->m_group);
    
    free(isolate);
}

Isolate * Isolate::New(Isolate::CreateParams const&params)
{
    IsolateImpl * isolate = (IsolateImpl *) malloc(sizeof (IsolateImpl));
    memset(isolate, 0, sizeof(IsolateImpl));
    
    isolate->m_group = JSContextGroupCreate();
    
    return isolate;
}


