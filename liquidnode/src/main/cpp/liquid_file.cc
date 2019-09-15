/*
 * Copyright (c) 2016 - 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "node_buffer.h"
#include "node_internals.h"
#include "string_bytes.h"
#include "liquid_file.h"

using namespace v8;

namespace node {
namespace fs {

// This class is only used on sync fs calls.
// For async calls FSReqWrap is used.
class FSReqWrapSync {
public:
    FSReqWrapSync() {}
    ~FSReqWrapSync() { uv_fs_req_cleanup(&req); }
    uv_fs_t req;

private:
    DISALLOW_COPY_AND_ASSIGN(FSReqWrapSync);
};

  
v8::Local<v8::Value> fs_(node::Environment *env, v8::Local<v8::Value> path, int req_access)
{
  EscapableHandleScope handle_scope(env->isolate());
  Context::Scope context_scope(env->context());

  BufferValue p(env->isolate(), path);

  int access = 0;
  Local<v8::Private> privateKey = v8::Private::ForApi(env->isolate(),
                                                      String::NewFromUtf8(env->isolate(), "__fs"));
  Local<Object> globalObj = env->context()->Global();
  Maybe<bool> result = globalObj->HasPrivate(env->context(), privateKey);
  if (result.IsJust() && result.FromJust()) {
    Local<Value> fsVal;
    if(globalObj->GetPrivate(env->context(), privateKey).ToLocal(&fsVal)) {
      Local<Object> fsObj = fsVal->ToObject(env->context()).ToLocalChecked();

      Local<Value> cwd = fsObj->Get(String::NewFromUtf8(env->isolate(), "cwd"));
      BufferValue c(env->isolate(), cwd);
      char fullp[strlen(*c) + strlen(*p) + 8];

      const char *pth = *p;
      while (*pth == ' ' || *pth == '\t') pth++;
      if (*pth != '/') {
        strcpy(fullp, *c);
        strcpy(fullp + strlen(*c), "/");
        strcpy(fullp + strlen(*c) + 1, pth);
        pth = fullp;
      }

      FSReqWrapSync req_wrap;
      env->PrintSyncTrace();
      int err = uv_fs_realpath(env->event_loop(),
                               &req_wrap.req, pth, nullptr);
      const char* link_path;
      if (err != 0) {
        link_path = pth;
      } else {
        link_path = static_cast<const char*>(req_wrap.req.ptr);
      }

      Local<Value> error;
      Local<Value> rc = StringBytes::Encode(env->isolate(),
                                            link_path,
                                            UTF8,
                                            &error).ToLocalChecked();
      if (rc.IsEmpty()) {
        env->ThrowUVException(UV_EINVAL,
                              "realpath",
                              "Invalid character encoding for path",
                              pth);
        return rc;
      }
      Local<Object> test = fsObj->Get(env->context(), String::NewFromUtf8(env->isolate(), "fs"))
              .ToLocalChecked()->ToObject(env->context()).ToLocalChecked();
      MaybeLocal<Value> tuple = test->CallAsFunction(env->context(), fsObj, 1, &rc);
      if (tuple.IsEmpty()) {
        access = 0;
      } else {
        access = (int) tuple.ToLocalChecked()->ToObject(env->context()).ToLocalChecked()
                ->Get(env->context(), 0).ToLocalChecked()
                ->ToNumber(env->context()).ToLocalChecked()->Value();
        rc = tuple.ToLocalChecked()->ToObject(env->context()).ToLocalChecked()
                ->Get(env->context(), 1).ToLocalChecked();
      }

      if ((req_access & access) != req_access) {
        env->ThrowError("access denied (EACCES)");
        return handle_scope.Escape(path);
      }

      return handle_scope.Escape(rc);
    }
  }
  // FileSystem object not set up yet, so carry on as normal
  return handle_scope.Escape(path);
}

Local<Value> alias_(Environment *env, Local<Value> path)
{
  EscapableHandleScope handle_scope(env->isolate());
  Context::Scope context_scope(env->context());

  Local<v8::Private> privateKey = v8::Private::ForApi(env->isolate(),
                                                      String::NewFromUtf8(env->isolate(), "__fs"));
  Local<Object> globalObj = env->context()->Global();
  Maybe<bool> result = globalObj->HasPrivate(env->context(), privateKey);
  if (result.IsJust() && result.FromJust()) {
    Local<Value> fsVal;
    if(globalObj->GetPrivate(env->context(), privateKey).ToLocal(&fsVal)) {
      Local<Object> fsObj = fsVal->ToObject(env->context()).ToLocalChecked();

      Local<Object> alias = fsObj->Get(env->context(),String::NewFromUtf8(env->isolate(),"alias"))
              .ToLocalChecked()->ToObject(env->context()).ToLocalChecked();
      MaybeLocal<Value> aliased = alias->CallAsFunction(env->context(), fsObj, 1, &path);

      return handle_scope.Escape(aliased.ToLocalChecked());
    }
  }
  // FileSystem object not set up yet, so carry on as normal
  return path;
}

Local<Value> chdir_(Environment *env, Local<Value> path)
{
  EscapableHandleScope handle_scope(env->isolate());
  Context::Scope context_scope(env->context());

  path = fs_(env, path, _FS_ACCESS_RD);
  if (!path->IsUndefined()) {
    String::Utf8Value str(env->isolate(), path);

    Local<v8::Private> privateKey = v8::Private::ForApi(env->isolate(),
                                                        String::NewFromUtf8(env->isolate(), "__fs"));
    Local<Object> globalObj = env->context()->Global();
    Maybe<bool> result = globalObj->HasPrivate(env->context(), privateKey);
    if (result.IsJust() && result.FromJust()) {
      Local<Value> fsVal;
      if(globalObj->GetPrivate(env->context(), privateKey).ToLocal(&fsVal)) {
        Local<Object> fsObj = fsVal->ToObject(env->context()).ToLocalChecked();
        CHECK(fsObj->Set(env->context(), String::NewFromUtf8(env->isolate(), "cwd"), path).ToChecked());
      }
    }
  }
  return handle_scope.Escape(path);
}

Local<Value> cwd_(Environment *env)
{
  EscapableHandleScope handle_scope(env->isolate());
  Context::Scope context_scope(env->context());

  Local<v8::Private> privateKey = v8::Private::ForApi(env->isolate(),
                                                      String::NewFromUtf8(env->isolate(), "__fs"));
  Local<Object> globalObj = env->context()->Global();
  Maybe<bool> result = globalObj->HasPrivate(env->context(), privateKey);
  if (result.IsJust() && result.FromJust()) {
    Local<Value> fsVal;
    if(globalObj->GetPrivate(env->context(), privateKey).ToLocal(&fsVal)) {
      Local<Object> fsObj = fsVal->ToObject(env->context()).ToLocalChecked();
      return handle_scope.Escape(alias_(env, fsObj->Get(env->context(),
                                                        String::NewFromUtf8(env->isolate(), "cwd")).ToLocalChecked()));
    }
  }
  return handle_scope.Escape(Undefined(env->isolate()));
}

void Chdir(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  if (args.Length() != 1 || !args[0]->IsString()) {
      return env->ThrowTypeError("Bad argument.");
  }

  BufferValue path(args.GetIsolate(), fs_(env, args[0], _FS_ACCESS_RD));

  // Only using uv_chdir to validate path -- the default path should never be used
  if (*path) {
    int err = uv_chdir(*path);
    if (err) {
      return env->ThrowUVException(err, "uv_chdir");
    }

    // The real default path is held in the filesystem object
    chdir_(env, args[0]);
  }
}

void Cwd(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  Local<Value> aliased;

  aliased = cwd_(env);
  if (aliased->IsUndefined()) {
    char buf[PATH_MAX];

    size_t cwd_len = sizeof(buf);
    int err = uv_cwd(buf, &cwd_len);
    if (err) {
      return env->ThrowUVException(err, "uv_cwd");
    }

    Local<String> cwd = String::NewFromUtf8(env->isolate(),
                                            buf,
                                            String::kNormalString,
                                            cwd_len);
        aliased = alias_(env, cwd);
    }
    args.GetReturnValue().Set(aliased);
}

}

void Chdir(const FunctionCallbackInfo<Value>& args) {
  fs::Chdir(args);
}

}