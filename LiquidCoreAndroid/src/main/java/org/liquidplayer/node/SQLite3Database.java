//
// SQLite3Database.java
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Created by Eric Lange
//
/*
 Copyright (c) 2017 Eric Lange. All rights reserved.

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
package org.liquidplayer.node;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteStatement;
import android.os.CancellationSignal;

import static android.database.sqlite.SQLiteDatabase.CREATE_IF_NECESSARY;
import static android.database.sqlite.SQLiteDatabase.ENABLE_WRITE_AHEAD_LOGGING;
import static android.database.sqlite.SQLiteDatabase.OPEN_READONLY;
import static android.database.sqlite.SQLiteDatabase.OPEN_READWRITE;
import static android.database.sqlite.SQLiteDatabase.openDatabase;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_MISUSE;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_OK;

@SuppressWarnings("unused")
class SQLite3Database {
    final SQLiteDatabase m_db;
    Long m_last_insert_rowId = 0L;
    CancellationSignal m_cancellationSignal = new CancellationSignal();

    private static final int SQLITE_OPEN_READONLY      =0x00000001;  /* Ok for sqlite3_open_v2() */
    private static final int SQLITE_OPEN_READWRITE     =0x00000002;  /* Ok for sqlite3_open_v2() */
    private static final int SQLITE_OPEN_CREATE        =0x00000004;  /* Ok for sqlite3_open_v2() */
    private static final int SQLITE_OPEN_WAL           =0x00080000;  /* VFS only */

    SQLite3Database(String path, int flags, String jVFS) throws SQLiteException {
        int mode = 0;
        if ((flags & SQLITE_OPEN_CREATE) != 0)    mode |= CREATE_IF_NECESSARY;
        if ((flags & SQLITE_OPEN_WAL) != 0)       mode |= ENABLE_WRITE_AHEAD_LOGGING;
        if ((flags & SQLITE_OPEN_READONLY) != 0)  mode |= OPEN_READONLY;
        if ((flags & SQLITE_OPEN_READWRITE) != 0) mode |= OPEN_READWRITE;

        m_db = openDatabase(path, null, mode, null);
    }

    SQLite3Shim.JNIReturnObject sqlite3_busy_timeout(int ms) {
        /* There is no way to override busy timeout on Android, so just ignore silently */
        return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
    }

    int sqlite3_changes() {
        SQLiteStatement statement = m_db.compileStatement("SELECT changes()");
        return (int) statement.simpleQueryForLong();
    }

    SQLite3Shim.JNIReturnObject sqlite3_close() {
        try {
            m_db.close();
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_enable_load_extension(int onoff) {
        if (onoff != 0) {
            return new SQLite3Shim.JNIReturnObject(SQLITE_MISUSE,
                    "sqlite3_enable_load_extension not supported");
        }
        return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
    }

    SQLite3Shim.JNIReturnObject sqlite3_exec(String sql) {
        try {
            m_db.execSQL(sql);
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    void sqlite3_interrupt() {
        m_cancellationSignal.cancel();
    }

    Long sqlite3_last_insert_rowid() {
        return m_last_insert_rowId;
    }

    SQLite3Shim.JNIReturnObject sqlite3_prepare_v2(String sql, int nByte) {
        try {
            SQLite3Statement stmt = new SQLite3Statement(this, sql);
            return new SQLite3Shim.JNIReturnObject(stmt);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }
}
