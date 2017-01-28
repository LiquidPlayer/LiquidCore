//
// ExpandableLayout.java
// LiquidPlayer Project
//
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
/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.liquidplayer.demoapp;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RelativeLayout;

/**
 * This layout is used to contain the extra information that will be displayed
 * when a certain cell is expanded. The custom relative layout is created in
 * order to achieve a fading affect of this layout's contents as it is being
 * expanded or collapsed as opposed to just fading the content in(out) after(before)
 * the cell expands(collapses).
 *
 * During expansion, layout takes place so the full contents of this layout can
 * be displayed. When the size changes to display the full contents of the layout,
 * its height is stored. When the view is collapsing, this layout's height becomes 0
 * since it is no longer in the visible part of the cell.By overriding onMeasure, and
 * setting the height back to its max height, it is still visible during the collapse
 * animation, and so, a fade out effect can be achieved.
 */
public class ExpandingLayout extends RelativeLayout {


    private OnSizeChangedListener mSizeChangedListener;
    private int mExpandedHeight = -1;

    public ExpandingLayout(Context context) {
        super(context);
    }

    public ExpandingLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ExpandingLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec) {
        if (mExpandedHeight > 0) {
            heightMeasureSpec = MeasureSpec.makeMeasureSpec(mExpandedHeight, MeasureSpec.AT_MOST);
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    protected void onSizeChanged (int w, int h, int oldw, int oldh) {
        mExpandedHeight = h;
        //Notifies the list data object corresponding to this layout that its size has changed.
        mSizeChangedListener.onSizeChanged(h);
    }

    public int getExpandedHeight() {
        return mExpandedHeight;
    }

    public void setExpandedHeight(int expandedHeight) {
        mExpandedHeight = expandedHeight;
    }

    public void setSizeChangedListener(OnSizeChangedListener listener) {
        mSizeChangedListener = listener;
    }
}
