//
//  NodeBridge.h
//  LiquidCoreiOS
//
/*
 Copyright (c) 2018 Eric Lange. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 - Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 - Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
