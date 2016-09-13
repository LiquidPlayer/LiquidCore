package org.liquidplayer.v8;

import org.junit.Before;
import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.*;
import static org.hamcrest.Matchers.*;

public class JSDateTest {

    private JSContext context;
    private JSDate ym, ymd, ymdh, ymdhm, ymdhms, ymdhmsm;

    @Before
    public void setUp() throws Exception {
        setUp(new JSContext());
    }

    public void setUp(JSContext context) throws Exception {
        this.context = context;
        ym = new JSDate(context,2016,7);
        ymd = new JSDate(context,2016,7,2);
        ymdh = new JSDate(context,2016,7,2,1);
        ymdhm = new JSDate(context,2016,7,2,1,10);
        ymdhms = new JSDate(context,2016,7,2,1,10,30);
        ymdhmsm = new JSDate(context,2016,7,2,1,10,30,500);
    }

    @Test
    public void testConstructors() throws Exception {
        JSDate now = new JSDate(context);
        JSDate javaNow = new JSDate(context,new Date());
        assertThat(javaNow.getTime(),greaterThanOrEqualTo(now.getTime()));
        assertThat(javaNow.getTime(),lessThan(now.getTime() + 2*1000));

        assertEquals(ym.getTime() + 2*24*60*60*1000, ymd.getTime().longValue());
        assertEquals(ymd.getTime() + 60*60*1000, ymdh.getTime().longValue());
        assertEquals(ymdh.getTime() + 10*60*1000, ymdhm.getTime().longValue());
        assertEquals(ymdhm.getTime() + 30*1000, ymdhms.getTime().longValue());
        assertEquals(ymdhms.getTime() + 500, ymdhmsm.getTime().longValue());
    }

    @Test
    public void testMethods() throws Exception {
        JSDate now1 = new JSDate(context);
        Long now2 = JSDate.now(context);
        assertThat(now2,greaterThanOrEqualTo(now1.getTime()));
        assertThat(now2,lessThan(now1.getTime() + 2*1000));

        assertEquals(1000L * (now1.getTime() / 1000L),JSDate.parse(context,now1.toString()).longValue());
    }

    @Test
    public void testGetters() throws Exception {
        assertEquals(2016,ym.getFullYear().intValue());
        assertEquals(6,ym.getMonth().intValue());
        assertEquals(2,ymd.getDate().intValue());
        assertEquals(1,ymdh.getHours().intValue());
        assertEquals(10,ymdhm.getMinutes().intValue());
        assertEquals(30,ymdhms.getSeconds().intValue());
        assertEquals(500,ymdhmsm.getMilliseconds().intValue());

        assertEquals(2,ymdhm.getDay().intValue()); // Tuesday
    }

    @Test
    public void testSetters() throws Exception {
        Integer ms = ymdhmsm.getMilliseconds();
        ymdhmsm.setMilliseconds(0);
        assertThat(ymdhmsm.getTime(),is(ymdhms.getTime()));
        ymdhmsm.setMilliseconds(ms);

        Integer s = ymdhms.getSeconds();
        ymdhms.setSeconds(0);
        assertThat(ymdhms.getTime(),is(ymdhm.getTime()));
        ymdhms.setSeconds(s);

        Integer m = ymdhm.getMinutes();
        ymdhm.setMinutes(0);
        assertThat(ymdhm.getTime(),is(ymdh.getTime()));
        ymdhm.setMinutes(m);

        Integer h = ymdh.getHours();
        ymdh.setHours(0);
        assertThat(ymdh.getTime(),is(ymd.getTime()));
        ymdh.setHours(h);

        Integer d = ymd.getDate();
        ymd.setDate(0);
        assertThat(ymd.getTime(),is(ym.getTime()));
        ymd.setDate(d);

        JSDate now = new JSDate(context);
        now.setFullYear(2005);
        assertThat(now.getFullYear(),is(2005));

        now.setMonth(5);
        assertThat(now.getMonth(),is(5));

        JSDate settime = new JSDate(context);
        settime.setTime(ymdhmsm.getTime());
        assertEquals(ymdhmsm.getTime(),settime.getTime());
    }

    @Test
    public void testUTC() throws Exception {
        Long Uym = JSDate.UTC(context,2016,7);
        Long Uymd = JSDate.UTC(context,2016,7,2);
        Long Uymdh = JSDate.UTC(context,2016,7,2,1);
        Long Uymdhm = JSDate.UTC(context,2016,7,2,1,10);
        Long Uymdhms = JSDate.UTC(context,2016,7,2,1,10,30);
        Long Uymdhmsm = JSDate.UTC(context,2016,7,2,1,10,30,500);
        assertEquals(Uym + 2*24*60*60*1000, Uymd.longValue());
        assertEquals(Uymd + 60*60*1000, Uymdh.longValue());
        assertEquals(Uymdh + 10*60*1000, Uymdhm.longValue());
        assertEquals(Uymdhm + 30*1000, Uymdhms.longValue());
        assertEquals(Uymdhms + 500, Uymdhmsm.longValue());

        assertEquals(Uym.longValue(), ym.getTime() - ym.getTimezoneOffset()*60*1000);

        JSDate utc = new JSDate(context,Uymdhmsm);
        assertEquals(utc.getTime().longValue(), ymdhmsm.getTime() - ymdhmsm.getTimezoneOffset()*60*1000);

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
        context.property("_date_",ymdhmsm);
        context.evaluateScript("var dateString = _date_.toDateString();");
        context.evaluateScript("var isoString = _date_.toISOString();");
        context.evaluateScript("var jsonString = _date_.toJSON();");
        context.evaluateScript("var timeString = _date_.toTimeString();");
        context.evaluateScript("var utcString = _date_.toUTCString();");

        String dateString = context.property("dateString").toString();
        String isoString  = context.property("isoString").toString();
        String jsonString = context.property("jsonString").toString();
        String timeString = context.property("timeString").toString();
        String utcString  = context.property("utcString").toString();

        assertEquals(dateString,ymdhmsm.toDateString());
        assertEquals(isoString,ymdhmsm.toISOString());
        assertEquals(jsonString,ymdhmsm.toJSON());
        assertEquals(timeString,ymdhmsm.toTimeString());
        assertEquals(utcString,ymdhmsm.toUTCString());
    }

    @org.junit.After
    public void shutDown() {
        Runtime.getRuntime().gc();
    }
}