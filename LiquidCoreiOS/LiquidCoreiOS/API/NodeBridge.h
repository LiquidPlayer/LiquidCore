//
//  NodeBridge.h
//  LiquidCoreiOS
//
//  Created by Eric Lange on 7/8/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#ifndef NodeBridge_h
#define NodeBridge_h

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

typedef void (*OnNodeStartedCallback)(void* data, JSContextRef ctx, JSContextGroupRef group);
typedef void (*OnNodeExitCallback)(void* data, int code);
typedef void (*ProcessThreadCallback)(void *);

EXTERNC void * process_start(OnNodeStartedCallback, OnNodeExitCallback, void*);
EXTERNC void process_dispose(void *token);
EXTERNC void process_set_filesystem(JSContextRef ctx, JSObjectRef fs);
EXTERNC void process_sync(void* token, ProcessThreadCallback runnable, void* data);
EXTERNC void process_async(void * token, ProcessThreadCallback runnable, void* data);
EXTERNC void * process_keep_alive(void *token);
EXTERNC void process_let_die(void *token, void* preserver);

#endif /* NodeBridge_h */
