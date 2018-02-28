//
//  Isolate.h
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Isolate_h
#define Isolate_h

#include <JavaScriptCore/JavaScript.h>
#include "include/v8-util.h"
#include "src/arguments.h"
#include "src/base/platform/platform.h"
#include "src/code-stubs.h"
#include "src/compilation-cache.h"
#include "src/debug/debug.h"
#include "src/execution.h"
#include "src/futex-emulation.h"
#include "src/heap/incremental-marking.h"
#include "src/api.h"
#include "src/lookup.h"
#include "src/objects-inl.h"
#include "src/parsing/preparse-data.h"
#include "src/profiler/cpu-profiler.h"
#include "src/unicode-inl.h"
#include "src/utils.h"
#include "src/vm-state.h"
#include "src/heap/heap.h"

#define DEF(T,V,F) \
    v8::internal::Object ** V;
struct Roots {
    STRONG_ROOT_LIST(DEF)
};

struct IsolateImpl {
    void *i0; // kHeapObjectMapOffset, kIsolateEmbedderDataOffset
    void *i1; // kForeignAddressOffset
    void *i2;
    void *i3;
    uint64_t i64_0; // kExternalMemoryOffset
    uint64_t i64_1; // kExternalMemoryLimitOffset
    uint64_t i64_2;
    void *i4;
    void *i5;
    struct Roots roots; // kIsolateRootsOffset
    JSContextGroupRef m_group;
    
    void EnterContext(v8::Context *ctx);
    void ExitContext(v8::Context *ctx);
    
    v8::Context *current_context;
};

#endif /* Isolate_h */

