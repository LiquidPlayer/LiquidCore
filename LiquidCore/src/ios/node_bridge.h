/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef NodeBridge_h
#define NodeBridge_h

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

typedef void (*JSHeapFinalizer)(JSContextGroupRef, void *userData);

typedef void (*OnNodeStartedCallback)(void* data, JSContextRef ctx, JSContextGroupRef group);
typedef void (*OnNodeExitCallback)(void* data, int code);
typedef void (*ProcessThreadCallback)(void *);

EXTERNC void * process_start(OnNodeStartedCallback, OnNodeExitCallback, void*);
EXTERNC void process_run_in_thread(void *token);
EXTERNC void process_set_filesystem(JSContextRef ctx, JSObjectRef fs);
EXTERNC void process_sync(void* token, ProcessThreadCallback runnable, void* data);
EXTERNC void process_async(void * token, ProcessThreadCallback runnable, void* data);
EXTERNC void * process_keep_alive(void *token);
EXTERNC void process_let_die(void *token, void* preserver);
EXTERNC void expose_host_directory(JSContextRef ctx, const char* dir, int mediaAccessMask);

#endif /* NodeBridge_h */
