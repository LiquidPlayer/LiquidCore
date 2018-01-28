//
//  platform.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <v8.h>
#include <libplatform/libplatform.h>

using namespace v8;

Platform* platform::CreateDefaultPlatform(int thread_pool_size,
                                IdleTaskSupport idle_task_support,
                                InProcessStackDumping in_process_stack_dumping,
                                TracingController* tracing_controller)
{
    return nullptr;
}

