//
//  Context.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include "Context.h"
#include "Isolate.h"

using namespace v8;

void Context::Enter()
{
    
}

void Context::Exit()
{
    
}

Local<Context> Context::New(Isolate* isolate, ExtensionConfiguration* extensions,
                            MaybeLocal<ObjectTemplate> global_template,
                            MaybeLocal<Value> global_object)
{
    ContextImpl * context = (ContextImpl *) malloc(sizeof (ContextImpl));
    memset(context, 0, sizeof(ContextImpl));
    
    context->m_context = JSGlobalContextCreateInGroup(((IsolateImpl *)isolate)->m_group, nullptr);
    
    return Local<Context>(context);
}

