//
//  shim.cpp
//  LiquidCoreiOSTests
//
//  Created by Eric Lange on 2/10/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <v8.h>
#include "include/v8-profiler.h"
#include "src/debug/debug-interface.h"
using namespace v8;



void CpuProfiler::Dispose() {}
void CpuProfiler::StartProfiling(Local<String> string, bool b) {}
CpuProfiler * CpuProfiler::New(Isolate* isolate) { return nullptr; }

void debug::SetDebugDelegate(Isolate* isolate, v8::debug::DebugDelegate* listener) {}

