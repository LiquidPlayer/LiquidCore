//
// sqlite3_shim.cpp
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
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <memory>
#include <malloc.h>
#include <android/log.h>
#include "sqlite3.h"
#include "jni.h"

static JavaVM *s_jvm;
static jobject s_sqlite3;
static jobject gClassLoader;
static jmethodID gFindClassMethod;

#define JNI_ENTER(env) \
    JNIEnv *env; \
    int __getEnvStat__ = s_jvm->GetEnv((void**)&env, JNI_VERSION_1_6); \
    if (__getEnvStat__ == JNI_EDETACHED) { \
        s_jvm->AttachCurrentThread(&env, NULL); \
    }

#define JNI_EXIT() \
    if (__getEnvStat__ == JNI_EDETACHED) { \
        s_jvm->DetachCurrentThread(); \
    }

#define SQLITE3_JNIRETURNOBJECT "org/liquidplayer/node/SQLite3Shim$JNIReturnObject"
#define ROBJ "L" SQLITE3_JNIRETURNOBJECT ";"

static jclass findClass(JNIEnv *env, const char* name) {
    return static_cast<jclass>(env->CallObjectMethod(gClassLoader,
        gFindClassMethod, env->NewStringUTF(name)));
}

typedef struct sqlite3 {
public:
    sqlite3(JNIEnv *env, jobject db) : m_db(env->NewGlobalRef(db)) {}
    virtual ~sqlite3() {
        JNI_ENTER(env)
            env->DeleteGlobalRef(m_db);
        JNI_EXIT()
    }
    jobject m_db;
    std::string m_errmsg;
} sqlite3;

class Mappable {
public:
    Mappable() {}
    virtual ~Mappable() {}
    virtual const void *data() = 0;
};

class JavaString : public Mappable {
public:
    JavaString() : m_java_string(0), m_string(0) {}
    JavaString(JNIEnv *env, jstring string) {
        m_java_string = (jstring) env->NewGlobalRef(string);
        m_string = env->GetStringUTFChars(m_java_string, NULL);
    }
    virtual ~JavaString() {
        if (m_string) {
            JNI_ENTER(env)
            env->ReleaseStringUTFChars(m_java_string, m_string);
            env->DeleteGlobalRef(m_java_string);
            JNI_EXIT()
        }
        m_string = 0;
        m_java_string = 0;
    }
    virtual const void *data() {
        return m_string;
    }
private:
    jstring m_java_string;
    const char *m_string;
};

class JavaBlob : public Mappable {
public:
    JavaBlob() : m_buffer_obj(0), m_blob(0), m_size(0) {}
    JavaBlob(JNIEnv *env, jbyteArray array) {
        m_buffer_obj = (jbyteArray) env->NewGlobalRef(array);
        m_blob = env->GetByteArrayElements(m_buffer_obj, NULL);
        m_size = env->GetArrayLength(m_buffer_obj);
    }
    virtual ~JavaBlob() {
        JNI_ENTER(env)
        env->ReleaseByteArrayElements(m_buffer_obj, m_blob, JNI_ABORT);
        env->DeleteGlobalRef(m_buffer_obj);
        JNI_EXIT()
    }
    virtual const void *data() {
        return m_blob;
    }
private:
    jbyteArray m_buffer_obj;
    jbyte *m_blob;
    jsize m_size;
};

typedef struct sqlite3_stmt {
public:
    sqlite3_stmt(JNIEnv *env, sqlite3* db, jobject stmt) :
        m_stmt(env->NewGlobalRef(stmt)), m_db(db), m_first_step(true) {}
    ~sqlite3_stmt() {
        JNI_ENTER(env)
            env->DeleteGlobalRef(m_stmt);
        JNI_EXIT()
    }
    jobject m_stmt;
    sqlite3* m_db;
    std::string m_errmsg;
    std::map<int,std::unique_ptr<JavaString>> m_column_names;
    std::vector<std::unique_ptr<Mappable>> m_column_values;
    bool m_first_step;
} sqlite3_stmt;

#define MUTEX_SIGNATURE 0xe71c1a08
typedef struct sqlite3_mutex {
    unsigned int signature;
public:
    sqlite3_mutex() : signature(MUTEX_SIGNATURE), mutex() {}
    virtual ~sqlite3_mutex() {}
    std::mutex mutex;
} sqlite3_mutex;

static jmethodID method(JNIEnv *env, jobject jobj, const char* name, const char *args) {
    jmethodID mid = nullptr;
    jclass cls = env->GetObjectClass(jobj);
    do {
        mid = env->GetMethodID(cls, name, args);

        if (!env->ExceptionCheck()) break;
        env->ExceptionClear();
        jclass super = env->GetSuperclass(cls);
        env->DeleteLocalRef(cls);
        if (super == NULL || env->ExceptionCheck()) {
            if (super != NULL) env->DeleteLocalRef(super);
            break;
        }
        cls = super;
    } while (true);

    if (mid) {
        env->DeleteLocalRef(cls);
    }
    return mid;
}

static int get_status(JNIEnv *env, sqlite3* db, jobject ret) {
    jclass cls = findClass(env, SQLITE3_JNIRETURNOBJECT);
    jfieldID fid = env->GetFieldID(cls, "status", "I");
    int status = env->GetIntField(ret, fid);

    fid = env->GetFieldID(cls, "error", "Ljava/lang/String;");
    jstring msg = (jstring) env->GetObjectField(ret, fid);
    if (msg) {
        const char *error = env->GetStringUTFChars(msg, NULL);
        db->m_errmsg = std::string(error);
        env->ReleaseStringUTFChars(msg, error);
    } else {
        db->m_errmsg = std::string();
    }
    return status;
}

extern "C" JNIEXPORT void JNICALL Java_org_liquidplayer_node_SQLite3Shim_initNative(JNIEnv* env, jobject thiz)
{
    env->GetJavaVM(&s_jvm);
    s_sqlite3 = env->NewWeakGlobalRef(thiz);
    auto randomClass = env->FindClass(SQLITE3_JNIRETURNOBJECT);
    jclass classClass = env->GetObjectClass(randomClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader",
                                             "()Ljava/lang/ClassLoader;");
    gClassLoader = env->NewGlobalRef(env->CallObjectMethod(randomClass, getClassLoaderMethod));
    gFindClassMethod = env->GetMethodID(classLoaderClass, "findClass",
                                    "(Ljava/lang/String;)Ljava/lang/Class;");
}

SQLITE_API int sqlite3_open_v2(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
)
{
    JNI_ENTER(env)
        jstring jFilename = env->NewStringUTF(filename);
        jstring jVFS = env->NewStringUTF(zVfs);

        jmethodID mid = method(env, s_sqlite3,
            "sqlite3_open_v2", "(Ljava/lang/String;ILjava/lang/String;)" ROBJ);

        jobject ret = env->CallObjectMethod(s_sqlite3, mid, jFilename, (jint) flags, jVFS);

        jclass cls = findClass(env, SQLITE3_JNIRETURNOBJECT);
        jfieldID fid = env->GetFieldID(cls, "status", "I");
        int status = env->GetIntField(ret, fid);

        if (status == SQLITE_OK) {
            fid = env->GetFieldID(cls, "reference", "Ljava/lang/Object;");
            *ppDb = new sqlite3(env, env->GetObjectField(ret, fid));
            (*ppDb)->m_errmsg = std::string();
        } else {
            *ppDb = nullptr;
        }
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_busy_timeout(sqlite3* db, int ms)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_busy_timeout", "(I)" ROBJ);
        jobject ret = env->CallObjectMethod(db->m_db, mid, ms);
        int status = get_status(env, db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_changes(sqlite3* db)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_changes", "()I");
        int count = env->CallIntMethod(db->m_db, mid);
    JNI_EXIT()

    return count;
}
SQLITE_API int sqlite3_close(sqlite3* db)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_close", "()" ROBJ);
        jobject ret = env->CallObjectMethod(db->m_db, mid);
        int status = get_status(env, db, ret);
    JNI_EXIT()

    delete db;

    return status;
}
SQLITE_API sqlite3_mutex *sqlite3_db_mutex(sqlite3* db)
{
    return new sqlite3_mutex;
}
SQLITE_API int sqlite3_enable_load_extension(sqlite3 *db, int onoff)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_enable_load_extension", "(I)" ROBJ);
        jobject ret = env->CallObjectMethod(db->m_db, mid, onoff);
        int status = get_status(env, db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API const char *sqlite3_errmsg(sqlite3* db)
{
    return db->m_errmsg.length() ? db->m_errmsg.c_str() : "SQLITE_OK";
}
SQLITE_API int sqlite3_exec(
  sqlite3* db,                               /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *arg,                                 /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
)
{
    /* callback / arg / errmsg are ignored as they are not used by database.cc */
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_exec", "(Ljava/lang/String;)" ROBJ);
        jstring jsql = env->NewStringUTF(sql);
        jobject ret = env->CallObjectMethod(db->m_db, mid, jsql);
        int status = get_status(env, db, ret);
        if (errmsg) {
            if (db->m_errmsg.length()) {
                *errmsg = strdup(db->m_errmsg.c_str());
            } else {
                *errmsg = nullptr;
            }
        }
    JNI_EXIT()

    return status;
}
SQLITE_API void sqlite3_interrupt(sqlite3* db)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_interrupt", "()V");
        jobject ret = env->CallObjectMethod(db->m_db, mid);
    JNI_EXIT()
}
SQLITE_API sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_last_insert_rowid", "()J");
        jlong value = env->CallLongMethod(db->m_db, mid);
    JNI_EXIT()

    return value;
}
SQLITE_API int sqlite3_load_extension(
  sqlite3 *db,          /* Load the extension into this database connection */
  const char *zFile,    /* Name of the shared library containing extension */
  const char *zProc,    /* Entry point.  Derived from zFile if 0 */
  char **pzErrMsg       /* Put error message here if not 0 */
)
{
    const char *not_supported = "load_extension not supported";
    *pzErrMsg = strdup(not_supported);
    return SQLITE_MISUSE;
}
SQLITE_API int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
)
{
    /* Ignoring pzTail as it is not used by statement.cc */
    JNI_ENTER(env)
        jmethodID mid = method(env, db->m_db, "sqlite3_prepare_v2", "(Ljava/lang/String;I)" ROBJ);
        jstring jsql = env->NewStringUTF(zSql);
        jobject ret = env->CallObjectMethod(db->m_db, mid, jsql, (jint)nByte);
        int status = get_status(env, db, ret);
        if (status == SQLITE_OK) {
            jclass cls = findClass(env,SQLITE3_JNIRETURNOBJECT);
            jfieldID fid = env->GetFieldID(cls, "reference", "Ljava/lang/Object;");
            *ppStmt = new sqlite3_stmt(env, db, env->GetObjectField(ret, fid));
            (*ppStmt)->m_errmsg = std::string();
        } else {
            *ppStmt = nullptr;
        }
    JNI_EXIT()

    return status;
}
SQLITE_API SQLITE_DEPRECATED void *sqlite3_trace(sqlite3* db,
   void(*xTrace)(void*,const char*), void* data)
{
    return nullptr;
}
SQLITE_API SQLITE_DEPRECATED void *sqlite3_profile(sqlite3* db,
   void(*xProfile)(void*,const char*,sqlite3_uint64), void* data)
{
    return nullptr;
}
SQLITE_API void *sqlite3_update_hook(
  sqlite3* db,
  void(*callback)(void *,int ,char const *,char const *,sqlite3_int64),
  void* data
)
{
    return nullptr;
}

SQLITE_API int sqlite3_bind_blob(sqlite3_stmt* stmt, int pos, const void* blob, int n,
    void(*destructor)(void*))
{
    /* destructor is always SQLITE_TRANSIENT in statement.cc */
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_blob", "(I[B)" ROBJ);
        jbyteArray blobArray;
        blobArray = env->NewByteArray(n);
        void *temp = env->GetPrimitiveArrayCritical((jarray)blobArray, 0);
        memcpy(temp, blob, n);
        env->ReleasePrimitiveArrayCritical(blobArray, temp, 0);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid, (jint)pos, blobArray);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_bind_double(sqlite3_stmt* stmt, int pos, double value)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_double", "(ID)" ROBJ);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid, (jint)pos, (jdouble)value);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_bind_int(sqlite3_stmt* stmt, int pos, int value)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_int", "(II)" ROBJ);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid, (jint)pos, (jint)value);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_bind_null(sqlite3_stmt* stmt, int pos)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_null", "(I)" ROBJ);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid, (jint)pos);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_bind_text(sqlite3_stmt* stmt, int pos, const char* text, int size,
    void(*destructor)(void*))
{
    /* destructor is always SQLITE_TRANSIENT in statement.cc */
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_text", "(ILjava/lang/String;)" ROBJ);
        char ctext[size+1];
        strncpy(ctext, text, size);
        ctext[size] = 0;
        jstring jtext = env->NewStringUTF(ctext);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid, (jint)pos, jtext);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API int sqlite3_bind_parameter_index(sqlite3_stmt* stmt, const char *zName)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_bind_parameter_index", "(Ljava/lang/String;)I");
        jstring jtext = env->NewStringUTF(zName);
        jint index = env->CallIntMethod(stmt->m_stmt, mid, jtext);
    JNI_EXIT()

    return index;
}
SQLITE_API int sqlite3_clear_bindings(sqlite3_stmt* stmt)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_clear_bindings", "()" ROBJ);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    return status;
}
SQLITE_API const void *sqlite3_column_blob(sqlite3_stmt* stmt, int iCol)
{
    // Lifecycle of a string or blob, according to the documentation:
    // ** ^The pointers returned are valid until a type conversion occurs as
    // ** described above, or until [sqlite3_step()] or [sqlite3_reset()] or
    // ** [sqlite3_finalize()] is called.  ^The memory space used to hold strings
    // ** and BLOBs is freed automatically.  Do <em>not</em> pass the pointers returned
    // ** from [sqlite3_column_blob()], [sqlite3_column_text()], etc. into
    // ** [sqlite3_free()].

    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_blob", "(I)[B");
        jbyteArray array = (jbyteArray) env->CallObjectMethod(stmt->m_stmt, mid, (jint)iCol);
        const void * blob = nullptr;
        if (array) {
            stmt->m_column_values.push_back(std::unique_ptr<JavaBlob>(new JavaBlob(env, array)));
            blob = stmt->m_column_values.back()->data();
        }
    JNI_EXIT()

    return blob;
}
SQLITE_API int sqlite3_column_bytes(sqlite3_stmt* stmt, int iCol)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_bytes", "(I)I");
        jint value = env->CallIntMethod(stmt->m_stmt, mid, (jint)iCol);
    JNI_EXIT()

    return value;
}
SQLITE_API double sqlite3_column_double(sqlite3_stmt* stmt, int iCol)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_double", "(I)D");
        jdouble value = env->CallDoubleMethod(stmt->m_stmt, mid, (jint)iCol);
    JNI_EXIT()

    return value;
}
SQLITE_API sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* stmt, int iCol)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_int64", "(I)J");
        jlong value = env->CallLongMethod(stmt->m_stmt, mid, (jint)iCol);
    JNI_EXIT()

    return value;
}
SQLITE_API const unsigned char *sqlite3_column_text(sqlite3_stmt* stmt, int iCol)
{
    // Lifecycle of a string or blob, according to the documentation:
    // ** ^The pointers returned are valid until a type conversion occurs as
    // ** described above, or until [sqlite3_step()] or [sqlite3_reset()] or
    // ** [sqlite3_finalize()] is called.  ^The memory space used to hold strings
    // ** and BLOBs is freed automatically.  Do <em>not</em> pass the pointers returned
    // ** from [sqlite3_column_blob()], [sqlite3_column_text()], etc. into
    // ** [sqlite3_free()].

    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_text", "(I)Ljava/lang/String;");
        jstring string = (jstring) env->CallObjectMethod(stmt->m_stmt, mid, (jint)iCol);
        const unsigned char * text = nullptr;
        if (string) {
            stmt->m_column_values.push_back(std::unique_ptr<JavaString>(new JavaString(env, string)));
            text = (const unsigned char*) stmt->m_column_values.back()->data();
        }
    JNI_EXIT()

    return text;
}
SQLITE_API int sqlite3_column_type(sqlite3_stmt* stmt, int iCol)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_type", "(I)I");
        jint value = env->CallIntMethod(stmt->m_stmt, mid, (jint)iCol);
    JNI_EXIT()

    return value;
}
SQLITE_API int sqlite3_column_count(sqlite3_stmt *pStmt)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, pStmt->m_stmt, "sqlite3_column_count", "()I");
        jint value = env->CallIntMethod(pStmt->m_stmt, mid);
    JNI_EXIT()

    return value;
}
SQLITE_API const char *sqlite3_column_name(sqlite3_stmt* stmt, int N)
{
    // Lifecycle of the column name, per the documentation:
    // ** ^The returned string pointer is valid until either the [prepared statement]
    // ** is destroyed by [sqlite3_finalize()] or until the statement is automatically
    // ** reprepared by the first call to [sqlite3_step()] for a particular run
    // ** or until the next call to
    // ** sqlite3_column_name() or sqlite3_column_name16() on the same column.

    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_column_name", "(I)Ljava/lang/String;");
        jstring string = (jstring) env->CallObjectMethod(stmt->m_stmt, mid, (jint)N);
        const char * name = nullptr;
        if (string) {
            stmt->m_column_names[N] = std::unique_ptr<JavaString>(new JavaString(env, string));
            name = (const char *) stmt->m_column_names[N]->data();
        }
    JNI_EXIT()

    return name;
}
SQLITE_API int sqlite3_finalize(sqlite3_stmt *pStmt)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, pStmt->m_stmt, "sqlite3_finalize", "()" ROBJ);
        jobject ret = env->CallObjectMethod(pStmt->m_stmt, mid);
        int status = get_status(env, pStmt->m_db, ret);
    JNI_EXIT()

    delete pStmt;

    return status;
}
SQLITE_API int sqlite3_reset(sqlite3_stmt *pStmt)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, pStmt->m_stmt, "sqlite3_reset", "()" ROBJ);
        jobject ret = env->CallObjectMethod(pStmt->m_stmt, mid);
        int status = get_status(env, pStmt->m_db, ret);
    JNI_EXIT()

    pStmt->m_first_step = true;
    pStmt->m_column_values.clear();

    return status;
}
SQLITE_API int sqlite3_step(sqlite3_stmt* stmt)
{
    JNI_ENTER(env)
        jmethodID mid = method(env, stmt->m_stmt, "sqlite3_step", "()" ROBJ);
        jobject ret = env->CallObjectMethod(stmt->m_stmt, mid);
        int status = get_status(env, stmt->m_db, ret);
    JNI_EXIT()

    if (stmt->m_first_step) {
        stmt->m_column_names.clear();
    }
    stmt->m_first_step = false;
    stmt->m_column_values.clear();

    return status;
}

SQLITE_API void sqlite3_free(void* ptr)
{
    if (ptr && *((unsigned int*)ptr) == MUTEX_SIGNATURE) {
        delete (sqlite3_mutex *) ptr;
    } else if (ptr) {
        free(ptr);
    }
}
SQLITE_API void sqlite3_mutex_enter(sqlite3_mutex* mutex)
{
    mutex->mutex.lock();
}
SQLITE_API void sqlite3_mutex_leave(sqlite3_mutex* mutex)
{
    mutex->mutex.unlock();
}
