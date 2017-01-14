//
// ExpandableListItem.java
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

import android.os.Parcel;
import android.os.Parcelable;

/**
 * This custom object is used to populate the list adapter. It contains a reference
 * to an image, title, and the extra text to be displayed. Furthermore, it keeps track
 * of the current state (collapsed/expanded) of the corresponding item in the list,
 * as well as store the height of the cell in its collapsed state.
 */
class ExpandableListItem implements OnSizeChangedListener, Parcelable {

    private String mTitle;
    private String mText;
    private String mUrl;
    private boolean mIsExpanded;
    private int mCollapsedHeight;
    private int mExpandedHeight;
    private String mFilename = null;
    private int mProgress;
    private boolean mIsDownloading;
    private Object data;

    ExpandableListItem(String title, String url, int collapsedHeight, String text) {
        mTitle = title;
        mCollapsedHeight = collapsedHeight;
        mIsExpanded = false;
        mText = text;
        mExpandedHeight = -1;
        mUrl = url;
        mProgress = 0;
        mIsDownloading = false;
    }

    private ExpandableListItem(Parcel in) {
        mTitle = in.readString();
        mText = in.readString();
        mUrl = in.readString();
        mIsExpanded = in.readInt() > 0;
        mCollapsedHeight = in.readInt();
        mExpandedHeight = in.readInt();
        mFilename = in.readString();
        mProgress = in.readInt();
        mIsDownloading = in.readInt() > 0;
    }

    String getUrl() {
        return mUrl;
    }
    String getTitle() {
        return mTitle;
    }

    void setExpanded(boolean isExpanded) {
        mIsExpanded = isExpanded;
    }
    boolean isExpanded() {
        return mIsExpanded;
    }

    public void setCollapsedHeight(int collapsedHeight) {
        mCollapsedHeight = collapsedHeight;
    }
    int getCollapsedHeight() {
        return mCollapsedHeight;
    }

    public String getText() {
        return mText;
    }
    public void setText(String text) {
        mText = text;
    }

    private void setExpandedHeight(int expandedHeight) {
        mExpandedHeight = expandedHeight;
    }
    int getExpandedHeight() {
        return mExpandedHeight;
    }

    void setFileName(String fname) {
        mFilename = fname;
    }
    String getFileName() {
        return mFilename;
    }

    public Object getData() {
        return data;
    }
    public void setData(Object data) {
        this.data = data;
    }

    void setProgress(int progress) {
        mProgress = progress;
    }
    int getProgress() {
        return mProgress;
    }

    void setDownloading(boolean isDownloading) {
        mIsDownloading = isDownloading;
    }
    boolean isDownloading() {
        return mIsDownloading;
    }


    @Override
    public void onSizeChanged(int newHeight) {
        setExpandedHeight(newHeight);
    }

    public static final Parcelable.Creator<ExpandableListItem> CREATOR =
            new Parcelable.Creator<ExpandableListItem>() {
        public ExpandableListItem createFromParcel(Parcel source) {
            return new ExpandableListItem(source);
        }

        public ExpandableListItem[] newArray(int size) {
            return new ExpandableListItem[size];
        }
    };

    @Override
    public int describeContents() {
        // hashCode() of this class
        return hashCode();
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeString(mTitle);
        out.writeString(mText);
        out.writeString(mUrl);
        out.writeInt(mIsExpanded ? 1 : 0);
        out.writeInt(mCollapsedHeight);
        out.writeInt(mExpandedHeight);
        out.writeString(mFilename);
        out.writeInt(mProgress);
        out.writeInt(mIsDownloading ? 1 : 0);
    }
}
