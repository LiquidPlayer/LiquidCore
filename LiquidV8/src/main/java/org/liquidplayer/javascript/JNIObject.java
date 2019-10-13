/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

abstract class JNIObject {
    JNIObject(long reference) {
        this.reference = reference;
    }

    protected long reference;
}
