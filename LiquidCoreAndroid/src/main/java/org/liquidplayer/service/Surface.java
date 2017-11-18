//
// Surface.java
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2016 Eric Lange. All rights reserved.

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
package org.liquidplayer.service;

import android.view.View;

import org.liquidplayer.javascript.JSContext;

import java.util.concurrent.Semaphore;

/**
 * A Surface is a UI interaction layer with a MicroService.
 */
public interface Surface {
    /**
     * Binds a MicroService to the surface.  This will get called after the micro service
     * is started, but before the service javascript is executed.  This gives the surface
     * an opportunity to bind any native functions required by the module.
     * @param service The microservice to bind
     */
    void bind(MicroService service, JSContext context, Synchronizer synchronizer);

    /**
     * Attaches a MicroService to the UI.  For most Surfaces, this should be done before the
     * MicroService is started so any interaction code with the Surface can be loaded and exposed
     * to the MicroService before the JavaScript is executed.
     * @param service  The MicroService to attach
     * @param onAttached A runnable to be called after the UI is active
     */
    View attach(MicroService service, Runnable onAttached);

    /**
     * Detaches any currently attached MicroService.  The Surface may then be ready to be discarded
     * or reattached to another MicroService.
     */
    void detach();

    /**
     * Detaches and clears the Surface.  This is the preferred method when the Surface is to be
     * reused.  detach() may leave behind some UI state, as it expects that the MicroService may
     * later be re-attached.  reset() clears any UI state back to a fresh state.
     */
    void reset();
}
