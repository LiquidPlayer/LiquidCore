
#ifndef NODE_SQLITE3_SRC_DATABASE_H
#define NODE_SQLITE3_SRC_DATABASE_H


#include <string>
#include <queue>

#include <sqlite3.h>
#include <nan.h>

#include "async.h"
#include <map>

using namespace v8;

namespace node_sqlite3 {

class Database;


class Database : public Nan::ObjectWrap {
public:
    static NAN_MODULE_INIT(Init);
    static std::map<v8::Isolate *, Nan::Persistent<FunctionTemplate> *> constructor_templates;

    static inline bool HasInstance(Local<Value> val) {
        Nan::HandleScope scope;
        Nan::Persistent<FunctionTemplate> *constructor_template =
            constructor_templates[v8::Isolate::GetCurrent()];
        if (!constructor_template) return false;
        if (!val->IsObject()) return false;
        Local<Object> obj = val.As<Object>();
        return Nan::New(*constructor_template)->HasInstance(obj);
    }

    struct Baton {
        uv_work_t request;
        Database* db;
        Nan::Persistent<Function> callback;
        int status;
        std::string message;
        uv_loop_t *loop;

        Baton(Database* db_, Local<Function> cb_, uv_loop_t *loop_) :
                db(db_), status(SQLITE_OK), loop(loop_) {
            db->Ref();
            request.data = this;
            callback.Reset(cb_);
        }
        virtual ~Baton() {
            db->Unref();
            callback.Reset();
        }
    };

    struct OpenBaton : Baton {
        std::string filename;
        int mode;
        OpenBaton(Database* db_, Local<Function> cb_, const char* filename_, int mode_, uv_loop_t* loop_) :
            Baton(db_, cb_, loop_), filename(filename_), mode(mode_) {}
    };

    struct ExecBaton : Baton {
        std::string sql;
        ExecBaton(Database* db_, Local<Function> cb_, const char* sql_, uv_loop_t* loop_) :
            Baton(db_, cb_, loop_), sql(sql_) {}
    };

    struct LoadExtensionBaton : Baton {
        std::string filename;
        LoadExtensionBaton(Database* db_, Local<Function> cb_, const char* filename_, uv_loop_t* loop_) :
            Baton(db_, cb_, loop_), filename(filename_) {}
    };

    typedef void (*Work_Callback)(Baton* baton);

    struct Call {
        Call(Work_Callback cb_, Baton* baton_, bool exclusive_ = false) :
            callback(cb_), exclusive(exclusive_), baton(baton_) {};
        Work_Callback callback;
        bool exclusive;
        Baton* baton;
    };

    struct ProfileInfo {
        std::string sql;
        sqlite3_int64 nsecs;
    };

    struct UpdateInfo {
        int type;
        std::string database;
        std::string table;
        sqlite3_int64 rowid;
    };

    bool IsOpen() { return open; }
    bool IsLocked() { return locked; }

    typedef Async<std::string, Database> AsyncTrace;
    typedef Async<ProfileInfo, Database> AsyncProfile;
    typedef Async<UpdateInfo, Database> AsyncUpdate;

    friend class Statement;

protected:
    Database() : Nan::ObjectWrap(),
        _handle(NULL),
        open(false),
        closing(false),
        locked(false),
        pending(0),
        serialize(false),
        debug_trace(NULL),
        debug_profile(NULL),
        update_event(NULL) {
    }

    ~Database() {
        RemoveCallbacks();
        sqlite3_close(_handle);
        _handle = NULL;
        open = false;
    }

    static NAN_METHOD(New);
    static void Work_BeginOpen(Baton* baton);
    static void Work_Open(uv_work_t* req);
    static void Work_AfterOpen(uv_work_t* req);

    static NAN_GETTER(OpenGetter);

    void Schedule(Work_Callback callback, Baton* baton, bool exclusive = false);
    void Process();

    static NAN_METHOD(Exec);
    static void Work_BeginExec(Baton* baton);
    static void Work_Exec(uv_work_t* req);
    static void Work_AfterExec(uv_work_t* req);

    static NAN_METHOD(Wait);
    static void Work_Wait(Baton* baton);

    static NAN_METHOD(Close);
    static void Work_BeginClose(Baton* baton);
    static void Work_Close(uv_work_t* req);
    static void Work_AfterClose(uv_work_t* req);

    static NAN_METHOD(LoadExtension);
    static void Work_BeginLoadExtension(Baton* baton);
    static void Work_LoadExtension(uv_work_t* req);
    static void Work_AfterLoadExtension(uv_work_t* req);

    static NAN_METHOD(Serialize);
    static NAN_METHOD(Parallelize);

    static NAN_METHOD(Configure);

    static NAN_METHOD(Interrupt);

    static void SetBusyTimeout(Baton* baton);

    static void RegisterTraceCallback(Baton* baton);
    static void TraceCallback(void* db, const char* sql);
    static void TraceCallback(Database* db, std::string* sql);

    static void RegisterProfileCallback(Baton* baton);
    static void ProfileCallback(void* db, const char* sql, sqlite3_uint64 nsecs);
    static void ProfileCallback(Database* db, ProfileInfo* info);

    static void RegisterUpdateCallback(Baton* baton);
    static void UpdateCallback(void* db, int type, const char* database, const char* table, sqlite3_int64 rowid);
    static void UpdateCallback(Database* db, UpdateInfo* info);

    void RemoveCallbacks();

protected:
    sqlite3* _handle;

    bool open;
    bool closing;
    bool locked;
    unsigned int pending;

    bool serialize;

    std::queue<Call*> queue;

    AsyncTrace* debug_trace;
    AsyncProfile* debug_profile;
    AsyncUpdate* update_event;
};

}

#endif
