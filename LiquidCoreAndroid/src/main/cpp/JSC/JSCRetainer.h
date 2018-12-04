/*
 * Copyright (c) 2017 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef LIQUIDCORE_JSCRETAINER_H
#define LIQUIDCORE_JSCRETAINER_H

#include "Common/Common.h"
#include "JSC/Macros.h"

class JSCRetainer {
public:
    JSCRetainer() : m_count(1) {
    }
    virtual ~JSCRetainer() {};
    inline void retain() {
        m_count++;
    }
    inline void release() {
        ASSERTJSC(m_count);
        if (--m_count==0) {
            delete this;
        }
    }
protected:
    int m_count;
};

#endif //LIQUIDCORE_JSCRETAINER_H
