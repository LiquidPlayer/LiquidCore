//
// JSDateTest.java
//
// AndroidJSCore project
// https://github.com/ericwlange/AndroidJSCore/
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2014-2016 Eric Lange. All rights reserved.

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
package org.liquidplayer.javascript;

import org.junit.Before;
import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSDateTest {

    protected JSContext getContext() {
        return new JSContext();
    }

    class DateTest {
        JSContext context;
        JSDate ym, ymd, ymdh, ymdhm, ymdhms, ymdhmsm;

        DateTest() throws Exception {
            this.context = getContext();
            ym = new JSDate(context,2016,7);
            ymd = new JSDate(context,2016,7,2);
            ymdh = new JSDate(context,2016,7,2,1);
            ymdhm = new JSDate(context,2016,7,2,1,10);
            ymdhms = new JSDate(context,2016,7,2,1,10,30);
            ymdhmsm = new JSDate(context,2016,7,2,1,10,30,500);
        }

        //Runtime.getRuntime().gc();
    }

    @Test
    public void testConstructors() throws Exception {
        DateTest dateTest = new DateTest();
        JSDate now = new JSDate(dateTest.context);
        JSDate javaNow = new JSDate(dateTest.context,new Date());
        assertThat(javaNow.getTime(),greaterThanOrEqualTo(now.getTime()));
        assertThat(javaNow.getTime(),lessThan(now.getTime() + 2*1000));

        assertEquals(dateTest.ym.getTime() + 2*24*60*60*1000, dateTest.ymd.getTime().longValue());
        assertEquals(dateTest.ymd.getTime() + 60*60*1000, dateTest.ymdh.getTime().longValue());
        assertEquals(dateTest.ymdh.getTime() + 10*60*1000, dateTest.ymdhm.getTime().longValue());
        assertEquals(dateTest.ymdhm.getTime() + 30*1000, dateTest.ymdhms.getTime().longValue());
        assertEquals(dateTest.ymdhms.getTime() + 500, dateTest.ymdhmsm.getTime().longValue());
    }

    @Test
    public void testMethods() throws Exception {
        DateTest dateTest = new DateTest();
        JSDate now1 = new JSDate(dateTest.context);
        Long now2 = JSDate.now(dateTest.context);
        assertThat(now2,greaterThanOrEqualTo(now1.getTime()));
        assertThat(now2,lessThan(now1.getTime() + 2*1000));

        assertEquals(1000L * (now1.getTime() / 1000L),JSDate.parse(dateTest.context,now1.toString()).longValue());
    }

    @Test
    public void testGetters() throws Exception {
        DateTest dateTest = new DateTest();
        assertEquals(2016,dateTest.ym.getFullYear().intValue());
        assertEquals(6,dateTest.ym.getMonth().intValue());
        assertEquals(2,dateTest.ymd.getDate().intValue());
        assertEquals(1,dateTest.ymdh.getHours().intValue());
        assertEquals(10,dateTest.ymdhm.getMinutes().intValue());
        assertEquals(30,dateTest.ymdhms.getSeconds().intValue());
        assertEquals(500,dateTest.ymdhmsm.getMilliseconds().intValue());

        assertEquals(2,dateTest.ymdhm.getDay().intValue()); // Tuesday
    }

    @Test
    public void testSetters() throws Exception {
        DateTest dateTest = new DateTest();
        Integer ms = dateTest.ymdhmsm.getMilliseconds();
        dateTest.ymdhmsm.setMilliseconds(0);
        assertThat(dateTest.ymdhmsm.getTime(),is(dateTest.ymdhms.getTime()));
        dateTest.ymdhmsm.setMilliseconds(ms);

        Integer s = dateTest.ymdhms.getSeconds();
        dateTest.ymdhms.setSeconds(0);
        assertThat(dateTest.ymdhms.getTime(),is(dateTest.ymdhm.getTime()));
        dateTest.ymdhms.setSeconds(s);

        Integer m = dateTest.ymdhm.getMinutes();
        dateTest.ymdhm.setMinutes(0);
        assertThat(dateTest.ymdhm.getTime(),is(dateTest.ymdh.getTime()));
        dateTest.ymdhm.setMinutes(m);

        Integer h = dateTest.ymdh.getHours();
        dateTest.ymdh.setHours(0);
        assertThat(dateTest.ymdh.getTime(),is(dateTest.ymd.getTime()));
        dateTest.ymdh.setHours(h);

        Integer d = dateTest.ymd.getDate();
        dateTest.ymd.setDate(0);
        assertThat(dateTest.ymd.getTime(),is(dateTest.ym.getTime()));
        dateTest.ymd.setDate(d);

        JSDate now = new JSDate(dateTest.context);
        now.setFullYear(2005);
        assertThat(now.getFullYear(),is(2005));

        now.setMonth(5);
        assertThat(now.getMonth(),is(5));

        JSDate settime = new JSDate(dateTest.context);
        settime.setTime(dateTest.ymdhmsm.getTime());
        assertEquals(dateTest.ymdhmsm.getTime(),settime.getTime());
    }

    @Test
    public void testUTC() throws Exception {
        DateTest dateTest = new DateTest();
        Long Uym = JSDate.UTC(dateTest.context,2016,7);
        Long Uymd = JSDate.UTC(dateTest.context,2016,7,2);
        Long Uymdh = JSDate.UTC(dateTest.context,2016,7,2,1);
        Long Uymdhm = JSDate.UTC(dateTest.context,2016,7,2,1,10);
        Long Uymdhms = JSDate.UTC(dateTest.context,2016,7,2,1,10,30);
        Long Uymdhmsm = JSDate.UTC(dateTest.context,2016,7,2,1,10,30,500);
        assertEquals(Uym + 2*24*60*60*1000, Uymd.longValue());
        assertEquals(Uymd + 60*60*1000, Uymdh.longValue());
        assertEquals(Uymdh + 10*60*1000, Uymdhm.longValue());
        assertEquals(Uymdhm + 30*1000, Uymdhms.longValue());
        assertEquals(Uymdhms + 500, Uymdhmsm.longValue());

        assertEquals(Uym.longValue(), dateTest.ym.getTime() - dateTest.ym.getTimezoneOffset()*60*1000);

        JSDate utc = new JSDate(dateTest.context,Uymdhmsm);
        assertEquals(utc.getTime().longValue(), dateTest.ymdhmsm.getTime() - dateTest.ymdhmsm.getTimezoneOffset()*60*1000);

        assertEquals(2,utc.getUTCDay().intValue()); // Tuesday

        assertThat(utc.getUTCMilliseconds(),is(500));
        utc.setUTCMilliseconds(0);
        assertThat(utc.getTime(),is(Uymdhms));

        assertThat(utc.getUTCSeconds(),is(30));
        utc.setUTCSeconds(0);
        assertThat(utc.getTime(),is(Uymdhm));

        assertThat(utc.getUTCMinutes(),is(10));
        utc.setUTCMinutes(0);
        assertThat(utc.getTime(),is(Uymdh));

        assertThat(utc.getUTCHours(),is(1));
        utc.setUTCHours(0);
        assertThat(utc.getTime(),is(Uymd));

        assertThat(utc.getUTCDate(),is(2));
        utc.setUTCDate(0);
        assertThat(utc.getTime(),is(Uym));

        assertThat(utc.getUTCMonth(),is(6));
        utc.setUTCMonth(0);
        assertThat(utc.getUTCMonth(),is(0));

        utc.setUTCFullYear(2005);
        assertThat(utc.getUTCFullYear(),is(2005));
    }

    @Test
    public void testConversionGetter() throws Exception {
        DateTest dateTest = new DateTest();
        dateTest.context.property("_date_",dateTest.ymdhmsm);
        dateTest.context.evaluateScript("var dateString = _date_.toDateString();");
        dateTest.context.evaluateScript("var isoString = _date_.toISOString();");
        dateTest.context.evaluateScript("var jsonString = _date_.toJSON();");
        dateTest.context.evaluateScript("var timeString = _date_.toTimeString();");
        dateTest.context.evaluateScript("var utcString = _date_.toUTCString();");

        String dateString = dateTest.context.property("dateString").toString();
        String isoString  = dateTest.context.property("isoString").toString();
        String jsonString = dateTest.context.property("jsonString").toString();
        String timeString = dateTest.context.property("timeString").toString();
        String utcString  = dateTest.context.property("utcString").toString();

        assertEquals(dateString,dateTest.ymdhmsm.toDateString());
        assertEquals(isoString,dateTest.ymdhmsm.toISOString());
        assertEquals(jsonString,dateTest.ymdhmsm.toJSON());
        assertEquals(timeString,dateTest.ymdhmsm.toTimeString());
        assertEquals(utcString,dateTest.ymdhmsm.toUTCString());
    }

}