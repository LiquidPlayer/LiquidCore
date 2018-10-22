//
// SQLite3Statement.java
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

import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteMisuseException;
import android.database.sqlite.SQLiteStatement;
import android.util.SparseArray;

import java.nio.charset.Charset;
import java.util.ArrayList;

import static android.database.Cursor.FIELD_TYPE_BLOB;
import static android.database.Cursor.FIELD_TYPE_FLOAT;
import static android.database.Cursor.FIELD_TYPE_INTEGER;
import static android.database.Cursor.FIELD_TYPE_NULL;
import static android.database.Cursor.FIELD_TYPE_STRING;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_DONE;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_OK;
import static org.liquidplayer.node.SQLite3Shim.CODES.SQLITE_ROW;

@SuppressWarnings("unused")
class SQLite3Statement {
    private final String m_operation;
    private final SQLite3Database m_db;
    private final String m_sql;
    private final boolean m_isQuery;

    private SQLiteStatement m_stmt;
    private Cursor m_cursor;
    private SparseArray<String> m_query_bindings;
    private Long m_result;
    private ArrayList<String> m_parameters = null;

    SQLite3Statement(SQLite3Database db, String sql) throws SQLiteException {
        /* Android rather annoyingly doesn't allow you to use SQLStatement for all types of
         * operations.  You must use SQLStatement or SQLQuery, and with specific execute methods
         * depending on the operation.  So we simulate this here.
         */
        m_operation = sql.trim().split(" ")[0].toUpperCase();
        m_db = db;
        m_sql = sql;

        if ("SELECT".equals(m_operation)) {
            // Use a query
            m_query_bindings = new SparseArray<>();
            m_isQuery = true;
            // Check for errors only
            m_db.m_db.compileStatement(sql);
        } else {
            // Use a statement
            m_stmt = m_db.m_db.compileStatement(sql);
            m_isQuery = false;
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_bind_blob(int pos, byte [] blob) {
        try {
            if (m_isQuery) {
                throw new SQLiteMisuseException("Why are you querying a blob, son?");
            } else {
                m_stmt.bindBlob(pos, blob);
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_bind_double(int pos, double value) {
        try {
            if (m_isQuery) {
                m_query_bindings.setValueAt(pos, Double.toString(value));
            } else {
                m_stmt.bindDouble(pos, value);
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_bind_int(int pos, int value) {
        try {
            if (m_isQuery) {
                m_query_bindings.setValueAt(pos, Long.toString(value));
            } else {
                m_stmt.bindLong(pos, value);
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_bind_null(int pos) {
        try {
            if (m_isQuery) {
                m_query_bindings.setValueAt(pos, null);
            } else {
                m_stmt.bindNull(pos);
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }
    SQLite3Shim.JNIReturnObject sqlite3_bind_text(int pos, String text) {
        try {
            if (m_isQuery) {
                m_query_bindings.setValueAt(pos, text);
            } else {
                m_stmt.bindString(pos, text);
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    int sqlite3_bind_parameter_index(String zName) {
        // According to the sqlite3 documentation:
        //** ^Return the index of an SQL parameter given its name.  ^The
        //** index value returned is suitable for use as the second
        //** parameter to [sqlite3_bind_blob|sqlite3_bind()].  ^A zero
        //** is returned if no matching parameter is found.  ^The parameter
        //** name must be given in UTF-8 even if the original statement
        //** was prepared from UTF-16 text using [sqlite3_prepare16_v2()].

        if (m_parameters == null) {
            // First remove all string literals to avoid confusing the parser
            final String double_regex = "\"(?:\\\\\"|[^\"])*?\"";
            final String single_regex = "'(?:\\\\'|[^'])*?'";
            String clean_sql = m_sql.replaceAll(double_regex, "\"\"");
            clean_sql = clean_sql.replaceAll(single_regex, "\"\"");

            // Now split into "words"
            String[] words = clean_sql.split(" \t\\[\\]\\(\\)\n\\{\\}");

            // Remove any word that isn't a parameter ("?NNN" or ":AAA" or "@AAA" or "$AAA")
            m_parameters = new ArrayList<>();
            for (String word : words) {
                if (word.startsWith("?") || word.startsWith(":") || word.startsWith("@") || word.startsWith("$")) {
                    m_parameters.add(word);
                }
            }
        }

        int index = 1;
        for (String parameter : m_parameters) {
            if (parameter.equals(zName)) {
                return index;
            }
            index++;
        }
        return 0;
    }

    SQLite3Shim.JNIReturnObject sqlite3_clear_bindings() {
        try {
            if (m_isQuery) {
                m_query_bindings.clear();
            } else {
                m_stmt.clearBindings();
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    byte [] sqlite3_column_blob(int iCol) {
        try {
            if (m_cursor != null) {
                return m_cursor.getBlob(iCol);
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    int sqlite3_column_bytes(int iCol) {
        try {
            if (m_cursor != null && m_cursor.getType(iCol) == FIELD_TYPE_BLOB) {
                return m_cursor.getBlob(iCol).length;
            } else if (m_cursor != null && m_cursor.getType(iCol) == FIELD_TYPE_STRING) {
                return m_cursor.getString(iCol).getBytes(Charset.forName("UTF-8")).length;
            }
            return 0;
        } catch (Exception e) {
            return 0;
        }
    }

    double sqlite3_column_double(int iCol) {
        try {
            if (m_cursor != null) {
                return m_cursor.getDouble(iCol);
            } else if (m_result != null && iCol == 1) {
                return m_result.doubleValue();
            }
            return 0;
        } catch (Exception e) {
            return 0.0;
        }
    }

    long sqlite3_column_int64(int iCol) {
        try {
            if (m_cursor != null) {
                return m_cursor.getLong(iCol);
            } else if (m_result != null && iCol == 1) {
                return m_result;
            }
            return 0L;
        } catch (Exception e) {
            return 0L;
        }
    }

    String sqlite3_column_text(int iCol) {
        try {
            if (m_cursor != null) {
                return m_cursor.getString(iCol);
            } else if (m_result != null && iCol == 1) {
                return m_result.toString();
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    private static final int SQLITE_INTEGER = 1;
    private static final int SQLITE_FLOAT   = 2;
    private static final int SQLITE_TEXT    = 3;
    private static final int SQLITE_BLOB    = 4;
    private static final int SQLITE_NULL    = 5;

    int sqlite3_column_type(int iCol) {
        try {
            int type = 0;
            if (m_cursor != null) {
                switch(m_cursor.getType(iCol)) {
                    case FIELD_TYPE_INTEGER: type = SQLITE_INTEGER; break;
                    case FIELD_TYPE_FLOAT:   type = SQLITE_FLOAT;   break;
                    case FIELD_TYPE_STRING:  type = SQLITE_TEXT;    break;
                    case FIELD_TYPE_BLOB:    type = SQLITE_BLOB;    break;
                    case FIELD_TYPE_NULL:    type = SQLITE_NULL;    break;
                }
            }
            return type;
        } catch (Exception e) {
            return 0;
        }
    }

    int sqlite3_column_count() {
        try {
            if (m_cursor != null) {
                return m_cursor.getColumnCount();
            } else if (m_result != null) {
                return 1;
            }
            return 0;
        } catch (Exception e) {
            return 0;
        }
    }

    String sqlite3_column_name(int N) {
        try {
            if (m_cursor != null) {
                return m_cursor.getColumnName(N);
            }
            return null;
        } catch (Exception e) {
            return null;
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_finalize() {
        SQLite3Shim.JNIReturnObject reset = sqlite3_reset();
        if (reset.status != SQLITE_OK.errno) {
            return reset;
        }
        try {
            if (m_stmt != null) {
                m_stmt.close();
                m_stmt = null;
            }
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    @SuppressWarnings("WeakerAccess")
    SQLite3Shim.JNIReturnObject sqlite3_reset() {
        try {
            if (m_cursor != null) {
                m_cursor.close();
                m_cursor = null;
            }
            if (m_query_bindings != null) {
                m_query_bindings.clear();
            }
            if (m_stmt != null) {
                m_stmt.clearBindings();
            }
            m_result = null;
            return new SQLite3Shim.JNIReturnObject(SQLITE_OK);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }

    SQLite3Shim.JNIReturnObject sqlite3_step() {
        try {
            SQLite3Shim.CODES status;
            if (m_isQuery) {
                if (m_cursor == null) {
                    String [] selections = new String[m_query_bindings.size()];
                    for (int i=0; i<m_query_bindings.size(); i++) {
                        selections[i] = m_query_bindings.valueAt(i);
                    }
                    m_cursor = m_db.m_db.rawQuery(m_sql, selections, m_db.m_cancellationSignal);
                    m_cursor.moveToFirst();
                    status = m_cursor.isAfterLast() ? SQLITE_DONE : SQLITE_ROW;
                } else {
                    status = m_cursor.moveToNext() ? SQLITE_ROW : SQLITE_DONE;
                }
                m_result = null;
            } else if ("INSERT".equals(m_operation)) {
                if (m_result == null) {
                    m_result = m_stmt.executeInsert();
                    m_db.m_last_insert_rowId = m_result;
                    status = SQLITE_ROW;
                } else {
                    m_result = null;
                    status = SQLITE_DONE;
                }
            } else if ("UPDATE".equals(m_operation) || "DELETE".equals(m_operation)) {
                if (m_result == null) {
                    m_result = (long) m_stmt.executeUpdateDelete();
                    status = SQLITE_ROW;
                } else {
                    m_result = null;
                    status = SQLITE_DONE;
                }
            } else {
                m_stmt.execute();
                m_result = null;
                status = SQLITE_DONE;
            }
            return new SQLite3Shim.JNIReturnObject(status);
        } catch (SQLiteException exception) {
            return new SQLite3Shim.JNIReturnObject(exception);
        }
    }
}
