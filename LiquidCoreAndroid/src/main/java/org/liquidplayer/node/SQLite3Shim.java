/*
 * Copyright (c) 2017 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.node;

import android.database.sqlite.SQLiteAbortException;
import android.database.sqlite.SQLiteAccessPermException;
import android.database.sqlite.SQLiteBindOrColumnIndexOutOfRangeException;
import android.database.sqlite.SQLiteBlobTooBigException;
import android.database.sqlite.SQLiteCantOpenDatabaseException;
import android.database.sqlite.SQLiteConstraintException;
import android.database.sqlite.SQLiteDatabaseCorruptException;
import android.database.sqlite.SQLiteDatabaseLockedException;
import android.database.sqlite.SQLiteDatatypeMismatchException;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteDoneException;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteFullException;
import android.database.sqlite.SQLiteMisuseException;
import android.database.sqlite.SQLiteOutOfMemoryException;
import android.database.sqlite.SQLiteReadOnlyDatabaseException;
import android.database.sqlite.SQLiteTableLockedException;
import android.support.annotation.Keep;

import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_ERROR;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_OK;

class SQLite3Shim {

    static class JNIReturnObject {
        JNIReturnObject(CODES code) {
            this.status = code.errno;
            this.reference = null;
            this.error = null;
        }
        JNIReturnObject(CODES code, String error) {
            this.status = code.errno;
            this.reference = null;
            this.error = error;
        }
        JNIReturnObject(SQLiteException exception) {
            SQLite3Shim.CODES ecode = null;
            for (SQLite3Shim.CODES code : SQLite3Shim.CODES.values()) {
                if (code.claz == exception.getClass()) {
                    ecode = code;
                    break;
                }
            }
            if (ecode == null) {
                ecode = SQLITE_ERROR;
            }
            status = ecode.errno;
            error = exception.getMessage();
            reference = null;
        }
        JNIReturnObject(Object reference) {
            this.status = SQLITE_OK.errno;
            this.error = null;
            this.reference = reference;
        }
        final int status;
        final Object reference;
        final String error;
    }

    enum CODES {

        SQLITE_OK        ("SQLITE_OK",           0, null),                                            /* Successful result */
        SQLITE_ERROR     ("SQLITE_ERROR",        1, SQLiteException.class),                           /* SQL error or missing database */
        SQLITE_INTERNAL  ("SQLITE_INTERNAL",     2, null),                                            /* Internal logic error in SQLite */
        SQLITE_PERM      ("SQLITE_PERM",         3, SQLiteAccessPermException.class),                 /* Access permission denied */
        SQLITE_ABORT     ("SQLITE_ABORT",        4, SQLiteAbortException.class),                      /* Callback routine requested an abort */
        SQLITE_BUSY      ("SQLITE_BUSY",         5, SQLiteDatabaseLockedException.class),             /* The database file is locked */
        SQLITE_LOCKED    ("SQLITE_LOCKED",       6, SQLiteTableLockedException.class),                /* A table in the database is locked */
        SQLITE_NOMEM     ("SQLITE_NOMEM",        7, SQLiteOutOfMemoryException.class),                /* A malloc() failed */
        SQLITE_READONLY  ("SQLITE_READONLY",     8, SQLiteReadOnlyDatabaseException.class),           /* Attempt to write a readonly database */
        SQLITE_INTERRUPT ("SQLITE_INTERRUPT",    9, null),                                            /* Operation terminated by sqlite3_interrupt()*/
        SQLITE_IOERR     ("SQLITE_IOERR",       10, SQLiteDiskIOException.class),                     /* Some kind of disk I/O error occurred */
        SQLITE_CORRUPT   ("SQLITE_CORRUPT",     11, SQLiteDatabaseCorruptException.class),            /* The database disk image is malformed */
        SQLITE_NOTFOUND  ("SQLITE_NOTFOUND",    12, null),                                            /* Unknown opcode in sqlite3_file_control() */
        SQLITE_FULL      ("SQLITE_FULL",        13, SQLiteFullException.class),                       /* Insertion failed because database is full */
        SQLITE_CANTOPEN  ("SQLITE_CANTOPEN",    14, SQLiteCantOpenDatabaseException.class),           /* Unable to open the database file */
        SQLITE_PROTOCOL  ("SQLITE_PROTOCOL",    15, null),                                            /* Database lock protocol error */
        SQLITE_EMPTY     ("SQLITE_EMPTY",       16, null),                                            /* Database is empty */
        SQLITE_SCHEMA    ("SQLITE_SCHEMA",      17, null),                                            /* The database schema changed */
        SQLITE_TOOBIG    ("SQLITE_TOOBIG",      18, SQLiteBlobTooBigException.class),                 /* String or BLOB exceeds size limit */
        SQLITE_CONSTRAINT("SQLITE_CONSTRAINT",  19, SQLiteConstraintException.class),                 /* Abort due to constraint violation */
        SQLITE_MISMATCH  ("SQLITE_MISMATCH",    20, SQLiteDatatypeMismatchException.class),           /* Data type mismatch */
        SQLITE_MISUSE    ("SQLITE_MISUSE",      21, SQLiteMisuseException.class),                     /* Library used incorrectly */
        SQLITE_NOLFS     ("SQLITE_NOLFS",       22, null),                                            /* Uses OS features not supported on host */
        SQLITE_AUTH      ("SQLITE_AUTH",        23, null),                                            /* Authorization denied */
        SQLITE_FORMAT    ("SQLITE_FORMAT",      24, null),                                            /* Auxiliary database format error */
        SQLITE_RANGE     ("SQLITE_RANGE",       25, SQLiteBindOrColumnIndexOutOfRangeException.class),/* 2nd parameter to sqlite3_bind out of range */
        SQLITE_NOTADB    ("SQLITE_NOTADB",      26, null),                                            /* File opened that is not a database file */
        SQLITE_NOTICE    ("SQLITE_NOTICE",      27, null),                                            /* Notifications from sqlite3_log() */
        SQLITE_WARNING   ("SQLITE_WARNING",     28, null),                                            /* Warnings from sqlite3_log() */
        SQLITE_ROW       ("SQLITE_ROW",         100,null),                                            /* sqlite3_step() has another row ready */
        SQLITE_DONE      ("SQLITE_DONE",        101,SQLiteDoneException.class),                       /* sqlite3_step() has finished executing */
        ;

        final int errno;
        final String code;
        final Class<? extends SQLiteException> claz;

        CODES(String code, int errno, Class <? extends SQLiteException> e) {
            this.code = code;
            this.errno = errno;
            this.claz = e;
        }
    }

    private static SQLite3Shim sShim;

    private native void initNative();

    private SQLite3Shim() {
        initNative();
    }

    static void init() {
        if (sShim == null) {
            sShim = new SQLite3Shim();
        }
    }

    @SuppressWarnings("unused")
    @Keep
    JNIReturnObject sqlite3_open_v2(String path, int flags, String jVFS) {
        try {
            SQLite3Database db = new SQLite3Database(path, flags, jVFS);
            return new JNIReturnObject(db);
        } catch (SQLiteException exception) {
            return new JNIReturnObject(exception);
        }
    }

}
