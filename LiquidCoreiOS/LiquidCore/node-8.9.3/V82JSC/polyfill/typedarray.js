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

/* In JavaScriptCore, a typed array that is constructed with an ArrayBuffer with non-integer
 * offset and/or length arguments will be treated as a zero-length array.  V8 takes the size
 * of the underlying ArrayBuffer.  We simulate that here.
 */

(function (global, factory) {
	typeof exports === 'object' && typeof module !== 'undefined' ? factory() :
	typeof define === 'function' && define.amd ? define(factory) :
	(factory());
}(this, (function () { 'use strict';
    const handler = {
        construct(target, args, newTarget) {
            if (typeof args[0] === 'object' &&
                args[0] instanceof ArrayBuffer &&
                args.length >= 3 &&
                (args[1] === undefined || args[1] === null) &&
                (args[2] === undefined || args[2] === null)) {

                return Reflect.construct(target, [args[0], 0, args[0].byteLength], newTarget);
            } else {
                return Reflect.construct(target, args, newTarget);
            }
        }
    };
    Uint8Array = new Proxy(Uint8Array, handler);
    Uint16Array = new Proxy(Uint16Array, handler);
    Uint32Array = new Proxy(Uint32Array, handler);
    Uint8ClampedArray = new Proxy(Uint8ClampedArray, handler);
    Int8Array = new Proxy(Int8Array, handler);
    Int16Array = new Proxy(Int16Array, handler);
    Int32Array = new Proxy(Int32Array, handler);
    Float32Array = new Proxy(Float32Array, handler);
    Float64Array = new Proxy(Float64Array, handler);
})));