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
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {


    ArrayList<ExpandableListItem> values;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        final int CELL_DEFAULT_HEIGHT = 200;
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

        if (savedInstanceState == null) {
            values = new ArrayList<>();
            values.add(new ExpandableListItem(
                    "The Elder Scrolls V Skyrim Trailer (2011)",
                    "magnet:?xt=urn:btih:617bb9d8a4449881e3d6018b177240f6db34364e&dn=The%20Elder%20Scrolls%20V%20Skyrim%20Trailer%20(2011)&tr=http://opentrackr.org:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=udp://tracker.pirateparty.gr:6969/announce",
                    CELL_DEFAULT_HEIGHT));
            values.add(new ExpandableListItem(
                    "New Mumiya (2017) Trailer Dub",
                    "magnet:?xt=urn:btih:bdce8abb6e16c2a1cb6ad2f049ccaa9fd64650e6&dn=New%20Mumiya%20(2017)%20Trailer%20Dub&tr=http://opentrackr.org:1337/announce&tr=http://p4p.arenabg.com:1337/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=udp://tracker.zer0day.to:1337/announce",
                    CELL_DEFAULT_HEIGHT));
            values.add(new ExpandableListItem(
                    "Alien Covenant Official Trailer",
                    "magnet:?xt=urn:btih:dc9d6c9d914602c9ec849ef5ad48c3e050432ddd&dn=Alien%20Covenant%20Official%20Trailer&tr=http://opentrackr.org:1337/announce&tr=http://p4p.arenabg.com:1337/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=udp://tracker.pirateparty.gr:6969/announce",
                    CELL_DEFAULT_HEIGHT));
        } else {
            values = savedInstanceState.getParcelableArrayList("list");
        }

        final ExpandingListView listView = (ExpandingListView) findViewById(R.id.list_view);
        final CustomArrayAdapter adapter = new CustomArrayAdapter(this, R.layout.list_item,
                values, listView);
        listView.setAdapter(adapter);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        outState.putParcelableArrayList("list", values);
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
