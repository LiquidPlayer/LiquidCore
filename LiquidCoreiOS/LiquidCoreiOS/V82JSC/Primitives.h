//
//  Primitives.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 2/6/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef Primitives_h
#define Primitives_h

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

struct OddballImpl {
    double toRawNumber;
    void *toString;
    void *toNumber;
    void *type;
    void *kind;
    void *size;
};

struct UndefinedImpl {
    union {
        v8::internal::Map *pMap;
        v8::internal::Oddball oddball;
        unsigned char filler_[256];
    };
    
    union {
        v8::internal::Map map;
        unsigned char filler1_[256];
    };
    JSValueRef m_undefined;
    
    static v8::Primitive * New(v8::Isolate *isolate);
};


#endif /* Primitives_h */
