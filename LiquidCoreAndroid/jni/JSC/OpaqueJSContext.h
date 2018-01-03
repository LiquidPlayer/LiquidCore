//
// OpaqueJSContext.h
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2018 Eric Lange. All rights reserved.

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
#ifndef LIQUIDCORE_OPAQUEJSCONTEXT_H_H
#define LIQUIDCORE_OPAQUEJSCONTEXT_H_H

#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSCRetainer.h"

class OpaqueJSContext : public JSCRetainer, public ManagedObject {
    public:
        static JSGlobalContextRef New(std::shared_ptr<JSContext> ctx);
        virtual ~OpaqueJSContext();
        virtual inline std::shared_ptr<JSContext> Context() const { return m_context; }
        virtual void MarkForCollection(JSValueRef value);
        virtual void MarkCollected(JSValueRef value);
        virtual void ForceGC();
        virtual void Dispose();

    private:
        OpaqueJSContext(std::shared_ptr<JSContext> ctx);
        virtual void GCCallback(GCType type, GCCallbackFlags flags);

        std::shared_ptr<JSContext> m_context;
        std::list<JSValueRef> m_collection;
        std::recursive_mutex m_gc_lock;
        bool m_isDefunct;

        static void StaticGCCallback(GCType type, GCCallbackFlags flags, void*data);
};

#endif //LIQUIDCORE_OPAQUEJSCONTEXT_H_H
