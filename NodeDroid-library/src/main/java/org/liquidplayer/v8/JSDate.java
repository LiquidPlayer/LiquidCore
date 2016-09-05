//
// JSDate.java
// AndroidJSCore project
//
// https://github.com/ericwlange/AndroidJSCore/
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
package org.liquidplayer.v8;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;

/**
 * Convenience class for managing JavaScript date objects
 * @since 1.0
 */
public class JSDate extends JSObject {
    /**
     * Creates a new date object with the current date and time
     * @param ctx  The JSContext in which to create the date object
     * @since 1.0
     */
    public JSDate(JSContext ctx) {
        context = ctx;
        valueRef = makeDate(context.ctxRef(), new long[0]);
        context.persistObject(this);
    }
    /**
     * Creates a new date object, initialized with a Java timestamp
     * @param ctx  The JSContext in which to create the date object
     * @param date  The Date with which to initialize the object
     * @since 1.0
     */
    public JSDate(JSContext ctx, Date date) {
        context = ctx;
        long [] args = { date.getTime() };
        valueRef = makeDate(context.ctxRef(), args);
        context.persistObject(this);
    }
    /**
     * Creates a new date object, initialized with a Java timestamp
     * @param ctx  The JSContext in which to create the date object
     * @param epoch  Milliseconds since since 1 January 1970 00:00:00 UTC
     * @since 3.0
     */
    public JSDate(JSContext ctx, Long epoch) {
        context = ctx;
        long [] args = { epoch };
        valueRef = makeDate(context.ctxRef(), args);
        context.persistObject(this);
    }

    /**
     * Creates a new data object, initialized by date components
     * @param ctx  The JSContext in which to create the date object
     * @param params FullYear, Month[, Date[, Hours[, Minutes[, Seconds[, Milliseconds]]]]]
     * @since 3.0
     */
    public JSDate(JSContext ctx, Integer ... params) {
        context = ctx;
        Calendar calendar = Calendar.getInstance();
        final int fields[] = {
              Calendar.YEAR,
              Calendar.MONTH,
              Calendar.DAY_OF_MONTH,
              Calendar.HOUR_OF_DAY,
              Calendar.MINUTE,
              Calendar.SECOND,
              Calendar.MILLISECOND
        };
        for (int i=0; i<fields.length; i++) {
            if (i<params.length) calendar.set(fields[i],params[i]);
            else calendar.set(fields[i],0);
        }

        valueRef = makeDate(context.ctxRef(), new long[] { calendar.getTime().getTime() });
        context.persistObject(this);
    }

    /* Methods */

    /**
     * JavaScript Date.now(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/now
     * @param ctx The JavaScript context
     * @return Returns the numeric value corresponding to the current time - the number of milliseconds
     * elapsed since 1 January 1970 00:00:00 UTC.
     * @since 3.0
     */
    public static Long now(JSContext ctx) {
        return ctx.property("Date").toObject().property("now").toFunction().call().toNumber().longValue();
    }

    /**
     * JavaScript Date.parse(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/parse
     * Parses a string representation of a date and returns the number of milliseconds since 1
     * January, 1970, 00:00:00, UTC.
     * Note: Parsing of strings with Date.parse is strongly discouraged due to browser differences
     * and inconsistencies.
     * @param ctx The context into which to create the JavaScript date object
     * @param string String representation of the date
     * @return a new JavaScript date object
     * @since 3.0
     */
    public static Long parse(JSContext ctx, String string) {
        return ctx.property("Date").toObject().property("parse").toFunction().call(null,string)
                .toNumber().longValue();
    }

    /**
     * JavaScript Date.UTC(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/UTC
     * Accepts the same parameters as the longest form of the constructor (i.e. 2 to 7) and returns
     * the number of milliseconds since 1 January, 1970, 00:00:00 UTC.
     * @param ctx The context into which to create the JavaScript date object
     * @param params FullYear, Month[, Date[, Hours[, Minutes[, Seconds[, Milliseconds]]]]]
     * @return a new JavaScript date object
     * @since 3.0
     */
    public static Long UTC(JSContext ctx, Integer ... params) {
        ArrayList<Integer> p = new ArrayList<>();
        for (int i=0; i<7; i++) {
            if (i < params.length) p.add(params[i]);
            else p.add(0);
        }
        return ctx.property("Date").toObject().property("UTC").toFunction().apply(null,p.toArray())
                .toNumber().longValue();
    }

    /* Date.prototype Methods */

    /* Getter */

    /**
     * JavaScript Date.prototype.getDate(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getDate
     * @return Returns the day of the month (1-31) for the specified date according to local time.
     * @since 3.0
     */
    public Integer getDate() {
        return property("getDate").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getDay(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getDay
     * @return Returns the day of the week (0-6) for the specified date according to local time.
     * @since 3.0
     */
    public Integer getDay() {
        return property("getDay").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getFullYear(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getFullYear
     * @return Returns the year (4 digits for 4-digit years) of the specified date according to local time.
     * @since 3.0
     */
    public Integer getFullYear() {
        return property("getFullYear").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getHours(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getHours
     * @return Returns the hour (0-23) in the specified date according to local time.
     * @since 3.0
     */
    public Integer getHours() {
        return property("getHours").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getMilliseconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getMilliseconds
     * @return Returns the milliseconds (0-999) in the specified date according to local time.
     * @since 3.0
     */
    public Integer getMilliseconds() {
        return property("getMilliseconds").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getMinutes(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getMinutes
     * @return Returns the minutes (0-59) in the specified date according to local time.
     * @since 3.0
     */
    public Integer getMinutes() {
        return property("getMinutes").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getMonth(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getMonth
     * @return Returns the month (0-11) in the specified date according to local time.
     * @since 3.0
     */
    public Integer getMonth() {
        return property("getMonth").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getSeconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getSeconds
     * @return Returns the seconds (0-59) in the specified date according to local time.
     * @since 3.0
     */
    public Integer getSeconds() {
        return property("getSeconds").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getTime(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getTime
     * @return Returns the numeric value of the specified date as the number of milliseconds
     * since January 1, 1970, 00:00:00 UTC (negative for prior times).
     * @since 3.0
     */
    public Long getTime() {
        return property("getTime").toFunction().call(this).toNumber().longValue();
    }

    /**
     * JavaScript Date.prototype.getTimezoneOffset(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getTimezoneOffset
     * @return Returns the time-zone offset in minutes for the current locale.
     * @since 3.0
     */
    public Integer getTimezoneOffset() {
        return property("getTimezoneOffset").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCDate(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCDate
     * @return Returns the day (date) of the month (1-31) in the specified date according to
     * universal time.
     * @since 3.0
     */
    public Integer getUTCDate() {
        return property("getUTCDate").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCDay(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCDay
     * @return Returns the day of the week (0-6) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCDay() {
        return property("getUTCDay").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCFullYear(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCFullYear
     * @return Returns the year (4 digits for 4-digit years) in the specified date according to
     * universal time.
     * @since 3.0
     */
    public Integer getUTCFullYear() {
        return property("getUTCFullYear").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCHours(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCHours
     * @return Returns the hours (0-23) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCHours() {
        return property("getUTCHours").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCMilliseconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCMilliseconds
     * @return Returns the milliseconds (0-999) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCMilliseconds() {
        return property("getUTCMilliseconds").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCMinutes(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCMinutes
     * @return Returns the minutes (0-59) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCMinutes() {
        return property("getUTCMinutes").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCMonth(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCMonth
     * @return Returns the month (0-11) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCMonth() {
        return property("getUTCMonth").toFunction().call(this).toNumber().intValue();
    }

    /**
     * JavaScript Date.prototype.getUTCSeconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/getUTCSeconds
     * @return Returns the seconds (0-59) in the specified date according to universal time.
     * @since 3.0
     */
    public Integer getUTCSeconds() {
        return property("getUTCSeconds").toFunction().call(this).toNumber().intValue();
    }

    /* Setter */

    /**
     * JavaScript Date.prototype.setDate(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setDate
     * @param value Sets the day of the month for a specified date according to local time.
     * @since 3.0
     */
    public void setDate(Integer value) {
        property("setDate").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setFullYear(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setFullYear
     * @param value Sets the full year (e.g. 4 digits for 4-digit years) for a specified date
     *              according to local time.
     * @since 3.0
     */
    public void setFullYear(Integer value) {
        property("setFullYear").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setHours(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setHours
     * @param value Sets the hours for a specified date according to local time.
     * @since 3.0
     */
    public void setHours(Integer value) {
        property("setHours").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setMilliseconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setMilliseconds
     * @param value Sets the milliseconds for a specified date according to local time.
     * @since 3.0
     */
    public void setMilliseconds(Integer value) {
        property("setMilliseconds").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setMinutes(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setMinutes
     * @param value Sets the minutes for a specified date according to local time.
     * @since 3.0
     */
    public void setMinutes(Integer value) {
        property("setMinutes").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setMonth(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setMonth
     * @param value Sets the month for a specified date according to local time.
     * @since 3.0
     */
    public void setMonth(Integer value) {
        property("setMonth").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setSeconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setSeconds
     * @param value Sets the seconds for a specified date according to local time.
     * @since 3.0
     */
    public void setSeconds(Integer value) {
        property("setSeconds").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setTime(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setTime
     * @param value Sets the Date object to the time represented by a number of milliseconds since
     *              January 1, 1970, 00:00:00 UTC, allowing for negative numbers for times prior.
     * @since 3.0
     */
    public void setTime(Long value) {
        property("setTime").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCDate(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCDate
     * @param value Sets the day of the month for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCDate(Integer value) {
        property("setUTCDate").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCFullYear(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCFullYear
     * @param value Sets the full year (e.g. 4 digits for 4-digit years) for a specified date
     *              according to universal time.
     * @since 3.0
     */
    public void setUTCFullYear(Integer value) {
        property("setUTCFullYear").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCHours(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCHours
     * @param value Sets the hour for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCHours(Integer value) {
        property("setUTCHours").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCMilliseconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCMilliseconds
     * @param value Sets the milliseconds for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCMilliseconds(Integer value) {
        property("setUTCMilliseconds").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCMinutes(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCMinutes
     * @param value Sets the minutes for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCMinutes(Integer value) {
        property("setUTCMinutes").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCMonth(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCMonth
     * @param value Sets the month for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCMonth(Integer value) {
        property("setUTCMonth").toFunction().call(this,value);
    }

    /**
     * JavaScript Date.prototype.setUTCSeconds(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/setUTCSeconds
     * @param value Sets the seconds for a specified date according to universal time.
     * @since 3.0
     */
    public void setUTCSeconds(Integer value) {
        property("setUTCSeconds").toFunction().call(this,value);
    }

    /* Conversion getter */

    /**
     * JavaScript Date.prototype.toDateString(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toDateString
     * @return Returns the "date" portion of the Date as a human - readable string.
     * @since 3.0
     */
    public String toDateString() {
        return property("toDateString").toFunction().call(this).toString();
    }

    /**
     * JavaScript Date.prototype.toISOString(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toISOString
     * @return Converts a date to a string following the ISO 8601 Extended Format.
     * @since 3.0
     */
    public String toISOString() {
        return property("toISOString").toFunction().call(this).toString();
    }

    /**
     * JavaScript Date.prototype.toJSON(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toJSON
     * @return Returns a string representing the Date using toISOString(). Intended for use
     * by JSON.stringify().
     * @since 3.0
     */
    public String toJSON() {
        return property("toJSON").toFunction().call(this).toString();
    }

    /**
     * JavaScript Date.prototype.toTimeString(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toTimeString
     * @return Returns the "time" portion of the Date as a human - readable string.
     * @since 3.0
     */
    public String toTimeString() {
        return property("toTimeString").toFunction().call(this).toString();
    }

    /**
     * JavaScript Date.prototype.toUTCString(), see:
     * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date/toUTCString
     * @return Converts a date to a string using the UTC timezone.
     * @since 3.0
     */
    public String toUTCString() {
        return property("toUTCString").toFunction().call(this).toString();
    }
}
