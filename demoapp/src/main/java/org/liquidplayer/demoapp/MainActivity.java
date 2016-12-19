//
// MainActivty.java
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
package org.liquidplayer.demoapp;

import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ListView;

import java.util.Arrays;

public class MainActivity extends AppCompatActivity {

    private final int CELL_DEFAULT_HEIGHT = 200;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        /*
        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
        */

        ExpandableListItem[] values = new ExpandableListItem[] {
                new ExpandableListItem(
                        "Housefull 3 Official Trailer",
                        "magnet:?xt=urn:btih:2bdffd7dbf03e63b1ee2cf1c6580b49d720ac331&dn=Housefull%203%20Official%20Trailer%20with%20Subtitle%20Akshay%20Kumar%20Riteish%20Deshmukh%20Abhishek%20Bachchan&tr=http://opentrackr.org:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce",
                        CELL_DEFAULT_HEIGHT,
                        getResources().getString(R.string.short_lorem_ipsum)),
                new ExpandableListItem(
                        "Now You See Me 2 Official Trailer 1 (2016)",
                        "magnet:?xt=urn:btih:1027d40cde958bb7ad78a9980a2df9aab43b8a06&dn=Now%20You%20See%20Me%202%20Official%20Trailer%201%20(2016)%201080p%20mp4&tr=http://opentrackr.org:1337/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://tracker.zer0day.to:1337/announce",
                        CELL_DEFAULT_HEIGHT,
                        getResources().getString(R.string.medium_lorem_ipsum)),
                new ExpandableListItem(
                        "Sausage Party Official Trailer",
                        "magnet:?xt=urn:btih:502425a36a992da16b445e138cf0094cd3cce880&dn=Sausage%20Party%20Official%20Trailer&tr=http://retracker.krs-ix.ru/announce&tr=http://tracker.dler.org:6969/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.mg64.net:6881/announce&tr=udp://tracker.piratepublic.com:1337/announce",
                        CELL_DEFAULT_HEIGHT,
                        getResources().getString(R.string.long_lorem_ipsum)),
        };

        final ExpandingListView listView = (ExpandingListView) findViewById(R.id.list_view);
        final CustomArrayAdapter adapter = new CustomArrayAdapter(this, R.layout.list_item,
                Arrays.asList(values), listView);
        listView.setAdapter(adapter);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
