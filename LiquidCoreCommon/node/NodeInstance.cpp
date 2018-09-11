//
// NodeInstance.cpp
//
// LiquidPlayer project
// https://github.com/LiquidPlayer
//
// Edited by Eric Lange
//
// Based on node.cc in node/src/node.cc
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
#ifdef __APPLE__
# include <JavaScriptCore/JavaScript.h>
#endif
#include "NodeInstance.h"
#include "nodedroid_file.h"
#include "os_dependent.h"

#include "node_buffer.h"
#include "node_constants.h"
#include "node_javascript.h"
#include "node_platform.h"
#include "node_version.h"
#include "node_internals.h"
#include "node_revert.h"
#include "node_debug_options.h"
#include "node_perf.h"

#if defined HAVE_PERFCTR
#include "node_counters.h"
#endif

#if HAVE_OPENSSL
#include "node_crypto.h"
#endif

#if defined(NODE_HAVE_I18N_SUPPORT)
#include "node_i18n.h"
#endif

#if defined HAVE_DTRACE || defined HAVE_ETW
#include "node_dtrace.h"
#endif

#if defined HAVE_LTTNG
#include "node_lttng.h"
#endif

#include <sys/resource.h>  // getrlimit, setrlimit

#include "node_perf.h"
#include "v8-profiler.h"

#ifdef __ANDROID__
# include "JSC/JSC.h"
# include "JNI/JNI.h"
#endif

#ifdef __ANDROID__
NodeInstance::NodeInstance(JNIEnv* env, jobject thiz) {
    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewGlobalRef(thiz);

    node_main_thread = new std::thread(node_main_task,reinterpret_cast<void*>(this));
    on_start = nullptr;
    callback_data = nullptr;
}
#endif

#ifdef __ANDROID__
# define DEBUG_LOGX(m, f) __android_log_print(ANDROID_LOG_DEBUG, m, f);
# define DEBUG_LOG(m, f, ...) __android_log_print(ANDROID_LOG_DEBUG, m, f, __VA_ARGS__);
# define ERROR_LOG(m, f, ...) __android_log_print(ANDROID_LOG_ERROR, m, f, __VA_ARGS__);
#else
# define DEBUG_LOGX(m, f) printf("%s: " f "\n", m)
# define DEBUG_LOG(m, f, ...) printf("%s: " f "\n", m, __VA_ARGS__)
# define ERROR_LOG(m, f, ...) fprintf(stderr, "%s: " f "\n", m, __VA_ARGS__)
#endif

NodeInstance::NodeInstance(OnNodeStartedCallback onStart, OnNodeExitCallback onExit, void* data)
{
#ifdef __ANDROID__
    m_jvm = nullptr;
    m_JavaThis = nullptr;
#endif
    node_main_thread = new std::thread(node_main_task,reinterpret_cast<void*>(this));
    on_start = onStart;
    on_exit = onExit;
    callback_data = data;
}

NodeInstance::~NodeInstance() {
    node_main_thread->join();
    delete node_main_thread;
}

void NodeInstance::spawnedThread()
{
    enum { kMaxArgs = 64 };
    char cmd[60];

    setenv("NODE_PATH", "/home/node_modules", true);
    strcpy(cmd, "node -e global.__nodedroid_onLoad();");

    int argc = 0;
    char *argv[kMaxArgs];

    char *p2 = strtok(cmd, " ");
    while (p2 && argc < kMaxArgs-1)
      {
        argv[argc++] = p2;
        p2 = strtok(0, " ");
      }
    argv[argc] = 0;

    int ret = StartInstance(argc, argv);

#ifdef __ANDROID__
    if (m_jvm) {
        JNIEnv *env;
        int getEnvStat = m_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            m_jvm->AttachCurrentThread(&env, NULL);
        }

        jclass cls = env->GetObjectClass(m_JavaThis);
        jmethodID mid;
        do {
            mid = env->GetMethodID(cls,"onNodeExit","(J)V");
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                if (getEnvStat == JNI_EDETACHED) {
                    m_jvm->DetachCurrentThread();
                }
                return;
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(m_JavaThis, mid, (jlong)ret);

        env->DeleteGlobalRef(m_JavaThis);

        if (getEnvStat == JNI_EDETACHED) {
            m_jvm->DetachCurrentThread();
        }
    }
#else
    if (on_exit) {
        on_exit(callback_data, ret);
    }
#endif
}

void NodeInstance::node_main_task(void *inst) {
    reinterpret_cast<NodeInstance*>(inst)->spawnedThread();
}

void NodeInstance::WaitForInspectorDisconnect(Environment* env) {
#if HAVE_INSPECTOR
  if (env->inspector_agent()->IsConnected()) {
    // Restore signal dispositions, the app is done and is no longer
    // capable of handling signals.
    env->inspector_agent()->WaitForDisconnect();
  }
#endif
}

bool NodeInstance::DomainHasErrorHandler(const Environment* env,
                                  const Local<Object>& domain) {
  HandleScope scope(env->isolate());

  Local<Value> domain_event_listeners_v = domain->Get(env->events_string());
  if (!domain_event_listeners_v->IsObject())
    return false;

  Local<Object> domain_event_listeners_o =
      domain_event_listeners_v.As<Object>();

  Local<Value> domain_error_listeners_v =
      domain_event_listeners_o->Get(env->error_string());

  if (domain_error_listeners_v->IsFunction() ||
      (domain_error_listeners_v->IsArray() &&
      domain_error_listeners_v.As<Array>()->Length() > 0))
    return true;

  return false;
}

bool NodeInstance::DomainsStackHasErrorHandler(const Environment* env) {
  HandleScope scope(env->isolate());

  if (!env->using_domains())
    return false;

  Local<Array> domains_stack_array = env->domains_stack_array().As<Array>();
  if (domains_stack_array->Length() == 0)
    return false;

  uint32_t domains_stack_length = domains_stack_array->Length();
  for (uint32_t i = domains_stack_length; i > 0; --i) {
    Local<Value> domain_v = domains_stack_array->Get(i - 1);
    if (!domain_v->IsObject())
      return false;

    Local<Object> domain = domain_v.As<Object>();
    if (DomainHasErrorHandler(env, domain))
      return true;
  }

  return false;
}

bool NodeInstance::ShouldAbortOnUncaughtException(Isolate* isolate) {
  HandleScope scope(isolate);

  Environment* env = Environment::GetCurrent(isolate);
  Local<Object> process_object = env->process_object();
  Local<String> emitting_top_level_domain_error_key =
    env->emitting_top_level_domain_error_string();
  bool isEmittingTopLevelDomainError =
      process_object->Get(emitting_top_level_domain_error_key)->BooleanValue();

  return isEmittingTopLevelDomainError || !DomainsStackHasErrorHandler(env);
}

void NodeInstance::Chdir(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  if (args.Length() != 1 || !args[0]->IsString()) {
    return env->ThrowTypeError("Bad argument.");
  }

  BufferValue path(args.GetIsolate(), nodedroid::fs_(env, args[0], _FS_ACCESS_RD));

  // Only using uv_chdir to validate path -- the default path should never be used
  if (*path) {
      int err = uv_chdir(*path);
      if (err) {
        return env->ThrowUVException(err, "uv_chdir");
      }

      // The real default path is held in the filesystem object
      nodedroid::chdir_(env, args[0]);
  }
}

void NodeInstance::Exit(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  DEBUG_LOG("NodeInstance", "exit(%d) called", (int) args[0]->Int32Value());

  WaitForInspectorDisconnect(Environment::GetCurrent(args));

  uv_walk(env->event_loop(), [](uv_handle_t* h, void* arg) {
    uv_unref(h);
  }, nullptr);
  uv_stop(env->event_loop());
}

void NodeInstance::Abort(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  WaitForInspectorDisconnect(Environment::GetCurrent(args));

  uv_walk(env->event_loop(), [](uv_handle_t* h, void* arg) {
    uv_unref(h);
  }, nullptr);
  uv_stop(env->event_loop());
}

// FIXME: Not sure if we should allow clients to do this
void NodeInstance::Kill(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);

  DEBUG_LOGX("NodeInstance", "kill called");

  if (args.Length() != 2) {
    return env->ThrowError("Bad argument.");
  }

  int pid = args[0]->Int32Value();
  int sig = args[1]->Int32Value();
  int err = uv_kill(pid, sig);
  args.GetReturnValue().Set(err);
}


void NodeInstance::Cwd(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  Local<Value> aliased;

  aliased = nodedroid::cwd_(env);
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
      aliased = nodedroid::alias_(env, cwd);
  }
  args.GetReturnValue().Set(aliased);
}

void NodeInstance::OnFatalError(const char* location, const char* message) {
  if (location) {
    ERROR_LOG( "NodeInstance",
        "FATAL ERROR: %s %s", location, message);
  } else {
    ERROR_LOG( "NodeInstance",
        "FATAL ERROR: %s", message);
  }
}

static struct {
  void Initialize(int thread_pool_size, uv_loop_t* loop) {}
  void Dispose() {}
  void DrainVMTasks() {}
  bool StartInspector(Environment *env, const char* script_path,
                      const node::DebugOptions& options) {
    env->ThrowError("Node compiled with NODE_USE_V8_PLATFORM=0");
    return true;
  }

  void StartTracingAgent() {
    fprintf(stderr, "Node compiled with NODE_USE_V8_PLATFORM=0, "
                    "so event tracing is not available.\n");
  }
  void StopTracingAgent() {}
  bool InspectorStarted(Environment *env) {
    return false;
  }
} v8_platform;

static void PrintErrorString(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
#ifdef _WIN32
  HANDLE stderr_handle = GetStdHandle(STD_ERROR_HANDLE);

  // Check if stderr is something other than a tty/console
  if (stderr_handle == INVALID_HANDLE_VALUE ||
      stderr_handle == nullptr ||
      uv_guess_handle(_fileno(stderr)) != UV_TTY) {
    vfprintf(stderr, format, ap);
    va_end(ap);
    return;
  }

  // Fill in any placeholders
  int n = _vscprintf(format, ap);
  std::vector<char> out(n + 1);
  vsprintf(out.data(), format, ap);

  // Get required wide buffer size
  n = MultiByteToWideChar(CP_UTF8, 0, out.data(), -1, nullptr, 0);

  std::vector<wchar_t> wbuf(n);
  MultiByteToWideChar(CP_UTF8, 0, out.data(), -1, wbuf.data(), n);

  // Don't include the null character in the output
  CHECK_GT(n, 0);
  WriteConsoleW(stderr_handle, wbuf.data(), n - 1, nullptr, nullptr);
#else
  vfprintf(stderr, format, ap);
#endif
  va_end(ap);
}

static void ReportException(Environment* env,
                            Local<Value> er,
                            Local<Message> message) {
  HandleScope scope(env->isolate());

  AppendExceptionLine(env, er, message, FATAL_ERROR);

  Local<Value> trace_value;
  Local<Value> arrow;
  const bool decorated = IsExceptionDecorated(env, er);

  if (er->IsUndefined() || er->IsNull()) {
    trace_value = Undefined(env->isolate());
  } else {
    Local<Object> err_obj = er->ToObject(env->isolate());

    trace_value = err_obj->Get(env->stack_string());
    arrow =
        err_obj->GetPrivate(
            env->context(),
            env->arrow_message_private_symbol()).ToLocalChecked();
  }

  node::Utf8Value trace(env->isolate(), trace_value);

  // range errors have a trace member set to undefined
  if (trace.length() > 0 && !trace_value->IsUndefined()) {
    if (arrow.IsEmpty() || !arrow->IsString() || decorated) {
      PrintErrorString("%s\n", *trace);
    } else {
      node::Utf8Value arrow_string(env->isolate(), arrow);
      PrintErrorString("%s\n%s\n", *arrow_string, *trace);
    }
  } else {
    // this really only happens for RangeErrors, since they're the only
    // kind that won't have all this info in the trace, or when non-Error
    // objects are thrown manually.
    Local<Value> message;
    Local<Value> name;

    if (er->IsObject()) {
      Local<Object> err_obj = er.As<Object>();
      message = err_obj->Get(env->message_string());
      name = err_obj->Get(FIXED_ONE_BYTE_STRING(env->isolate(), "name"));
    }

    if (message.IsEmpty() ||
        message->IsUndefined() ||
        name.IsEmpty() ||
        name->IsUndefined()) {
      // Not an error object. Just print as-is.
      String::Utf8Value message(er);

      PrintErrorString("%s\n", *message ? *message :
                                          "<toString() threw exception>");
    } else {
      node::Utf8Value name_string(env->isolate(), name);
      node::Utf8Value message_string(env->isolate(), message);

      if (arrow.IsEmpty() || !arrow->IsString() || decorated) {
        PrintErrorString("%s: %s\n", *name_string, *message_string);
      } else {
        node::Utf8Value arrow_string(env->isolate(), arrow);
        PrintErrorString("%s\n%s: %s\n",
                         *arrow_string,
                         *name_string,
                         *message_string);
      }
    }
  }

  fflush(stderr);
}

static void ReportException(Environment* env, const TryCatch& try_catch) {
  ReportException(env, try_catch.Exception(), try_catch.Message());
}

void FatalException(Isolate* isolate,
                    Local<Value> error,
                    Local<Message> message) {
  HandleScope scope(isolate);

  Environment* env = Environment::GetCurrent(isolate);
  Local<Object> process_object = env->process_object();
  Local<String> fatal_exception_string = env->fatal_exception_string();
  Local<Function> fatal_exception_function =
      process_object->Get(fatal_exception_string).As<Function>();

  int exit_code = 0;
  if (!fatal_exception_function->IsFunction()) {
    // failed before the process._fatalException function was added!
    // this is probably pretty bad.  Nothing to do but report and exit.
    ReportException(env, error, message);
    exit_code = 6;
  }

  if (exit_code == 0) {
    TryCatch fatal_try_catch(isolate);

    // Do not call FatalException when _fatalException handler throws
    fatal_try_catch.SetVerbose(false);

    // this will return true if the JS layer handled it, false otherwise
    Local<Value> caught =
        fatal_exception_function->Call(process_object, 1, &error);

    if (fatal_try_catch.HasCaught()) {
      // the fatal exception function threw, so we must exit
      ReportException(env, fatal_try_catch);
      exit_code = 7;
    }

    if (exit_code == 0 && false == caught->BooleanValue()) {
      ReportException(env, error, message);
      exit_code = 1;
    }
  }

  if (exit_code) {
#if HAVE_INSPECTOR
    env->inspector_agent()->FatalException(error, message);
#endif
    exit(exit_code);
  }
}


static void OnMessage(Local<Message> message, Local<Value> error) {
  // The current version of V8 sends messages for errors only
  // (thus `error` is always set).
  FatalException(Isolate::GetCurrent(), error, message);
}

void NodeInstance::StartInspector(Environment* env, const char* path,
                           DebugOptions debug_options) {
#if HAVE_INSPECTOR
  CHECK(!env->inspector_agent()->IsStarted());
  v8_platform.StartInspector(env, path, debug_options);
#endif  // HAVE_INSPECTOR
}

#define JSC "Lorg/liquidplayer/javascript/JNIJSContext;"

#ifdef __ANDROID__
void NodeInstance::NotifyStart(JSContextRef ctxRef, JSContextGroupRef groupRef) {
    JNIEnv *jenv;
    int getEnvStat = m_jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&jenv, NULL);
    }
    
    jclass cls = jenv->GetObjectClass(m_JavaThis);
    jmethodID mid;
    do {
        mid = jenv->GetMethodID(cls,"onNodeStarted","(JJJ)V");
        if (!jenv->ExceptionCheck()) break;
        jenv->ExceptionClear();
        jclass super = jenv->GetSuperclass(cls);
        jenv->DeleteLocalRef(cls);
        if (super == NULL || jenv->ExceptionCheck()) {
            if (super != NULL) jenv->DeleteLocalRef(super);
            if (getEnvStat == JNI_EDETACHED) {
                m_jvm->DetachCurrentThread();
            }
            CHECK_EQ(0,1); // This is bad
        }
        cls = super;
    } while (true);
    jenv->DeleteLocalRef(cls);

    auto group = const_cast<OpaqueJSContextGroup*>(groupRef);

    jenv->CallVoidMethod(m_JavaThis, mid,
                         SharedWrap<JSContext>::New(ctxRef->Context()),
                         SharedWrap<ContextGroup>::New(group->ContextGroup::shared_from_this()),
                         reinterpret_cast<jlong>(ctxRef)
                         );
    
    if (getEnvStat == JNI_EDETACHED) {
        m_jvm->DetachCurrentThread();
    }
}
#else
void NodeInstance::NotifyStart(JSContextRef ctx, JSContextGroupRef group)
{
    if (on_start) {
        on_start(callback_data, ctx, group);
    }
}
#endif

inline int NodeInstance::StartInstance(void* group_, IsolateData* isolate_data,
                 int argc, const char* const* argv,
                 int exec_argc, const char* const* exec_argv) {
  /* ===Start */
  Isolate* isolate = node_isolate;
  JSContextGroupRef group = (JSContextGroupRef)group_;
    
  HandleScope handle_scope(isolate);
  JSGlobalContextRef ctxRef = nullptr;

  Local<Context> context = os_newContext(isolate, group, &ctxRef);
  /* ===End */

  Context::Scope context_scope(context);
  Environment env(isolate_data, context);
  CHECK_EQ(0, uv_key_create(&thread_local_env));
  uv_key_set(&thread_local_env, &env);
  env.Start(argc, argv, exec_argc, exec_argv, v8_is_profiling);

  /* ===Start */
  // Override default chdir and cwd methods
  Local<Object> process = env.process_object();
  env.SetMethod(process, "chdir", Chdir);
  env.SetMethod(process, "cwd", Cwd);

  // Remove process.dlopen().  Nothing good can come of it in this environment.
  CHECK(process->Delete(env.context(), String::NewFromUtf8(isolate, "dlopen")).ToChecked());

  // Override exit() and abort() so they don't nuke the app
  env.SetMethod(process, "reallyExit", Exit);
  env.SetMethod(process, "abort", Abort);
  env.SetMethod(process, "_kill", Kill);
  NotifyStart(ctxRef, group);

  /* ===End */

  const char* path = argc > 1 ? argv[1] : nullptr;
  StartInspector(&env, path, debug_options);

  if (debug_options.inspector_enabled() && !v8_platform.InspectorStarted(&env))
    return 12;  // Signal internal error.

  env.set_abort_on_uncaught_exception(abort_on_uncaught_exception);

  if (force_async_hooks_checks) {
    env.async_hooks()->force_checks();
  }

  {
    Environment::AsyncCallbackScope callback_scope(&env);
    env.async_hooks()->push_async_ids(1, 0);
    LoadEnvironment(&env);
    env.async_hooks()->pop_async_id(1);
  }

  env.set_trace_sync_io(trace_sync_io);
    
#ifdef __APPLE__
  // Set up a dummy runloop source to avoid spinning
  CFRunLoopSourceContext noSpinCtx = {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  CFRunLoopSourceRef noSpinSource = CFRunLoopSourceCreate(NULL, 0, &noSpinCtx);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), noSpinSource, kCFRunLoopDefaultMode);
  CFRelease(noSpinSource);
#endif

  {
    SealHandleScope seal(isolate);

    bool more;
    PERFORMANCE_MARK(&env, LOOP_START);
    do {
#ifdef __ANDROID__
      uv_run(env.event_loop(), UV_RUN_DEFAULT);
#else
      uv_run(env.event_loop(), UV_RUN_NOWAIT);
      CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, false);
#endif

      v8_platform.DrainVMTasks();

      more = uv_loop_alive(env.event_loop());
      if (more)
        continue;

      EmitBeforeExit(&env);

      // Emit `beforeExit` if the loop became alive either after emitting
      // event, or after running some callbacks.
      more = uv_loop_alive(env.event_loop());
    } while (more == true);
    PERFORMANCE_MARK(&env, LOOP_EXIT);
  }

  env.set_trace_sync_io(false);

  const int exit_code = EmitExit(&env);
  RunAtExit(&env);

  /* ===Start */
  os_Dispose(group, ctxRef);
  /* ===End */

  uv_key_delete(&thread_local_env);

  v8_platform.DrainVMTasks();
  WaitForInspectorDisconnect(&env);
#if defined(LEAK_SANITIZER)
  __lsan_do_leak_check();
#endif

  return exit_code;
}

#ifdef __POSIX__
static const unsigned kMaxSignal = 32;
#endif

inline int NodeInstance::StartInstance(uv_loop_t* event_loop,
                 int argc, const char* const* argv,
                 int exec_argc, const char* const* exec_argv) {
  Isolate::CreateParams params;
  ArrayBufferAllocator allocator;
  params.array_buffer_allocator = &allocator;
#ifdef NODE_ENABLE_VTUNE_PROFILING
  params.code_event_handler = vTune::GetVtuneCodeEventHandler();
#endif

  m_loop = event_loop;
  Isolate* const isolate = Isolate::New(params);
  if (isolate == nullptr)
    return 12;  // Signal internal error.

  isolate->AddMessageListener(OnMessage);
  isolate->SetAbortOnUncaughtExceptionCallback(ShouldAbortOnUncaughtException);
  isolate->SetAutorunMicrotasks(false);
  isolate->SetFatalErrorHandler(OnFatalError);

  if (track_heap_objects) {
    isolate->GetHeapProfiler()->StartTrackingHeapObjects(true);
  }

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    node_isolate = isolate;
  }

  JSContextGroupRef group = os_groupFromIsolate(isolate, event_loop);

  int exit_code;
  {
    Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    IsolateData isolate_data(isolate, event_loop, allocator.zero_fill_field());
    exit_code = StartInstance((void*)group, &isolate_data, argc, argv, exec_argc, exec_argv);
  }

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    CHECK_EQ(node_isolate, isolate);
    node_isolate = nullptr;
  }

  isolate->Dispose();

  return exit_code;
}

inline void NodeInstance::PlatformInit() {
#ifdef __POSIX__
#if HAVE_INSPECTOR
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGUSR1);
  const int err = pthread_sigmask(SIG_SETMASK, &sigmask, nullptr);
#endif  // HAVE_INSPECTOR

  // Make sure file descriptors 0-2 are valid before we start logging anything.
  for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; fd += 1) {
    struct stat ignored;
    if (fstat(fd, &ignored) == 0)
      continue;
    // Anything but EBADF means something is seriously wrong.  We don't
    // have to special-case EINTR, fstat() is not interruptible.
    if (errno != EBADF)
      ABORT();
    if (fd != open("/dev/null", O_RDWR))
      ABORT();
  }

#if HAVE_INSPECTOR
  CHECK_EQ(err, 0);
#endif  // HAVE_INSPECTOR

#ifndef NODE_SHARED_MODE
  // Restore signal dispositions, the parent process may have changed them.
  struct sigaction act;
  memset(&act, 0, sizeof(act));

  // The hard-coded upper limit is because NSIG is not very reliable; on Linux,
  // it evaluates to 32, 34 or 64, depending on whether RT signals are enabled.
  // Counting up to SIGRTMIN doesn't work for the same reason.
  for (unsigned nr = 1; nr < kMaxSignal; nr += 1) {
    if (nr == SIGKILL || nr == SIGSTOP)
      continue;
    act.sa_handler = (nr == SIGPIPE) ? SIG_IGN : SIG_DFL;
    CHECK_EQ(0, sigaction(nr, &act, nullptr));
  }
#endif  // !NODE_SHARED_MODE

  RegisterSignalHandler(SIGINT, SignalExit, true);
  RegisterSignalHandler(SIGTERM, SignalExit, true);

  // Raise the open file descriptor limit.
  struct rlimit lim;
  if (getrlimit(RLIMIT_NOFILE, &lim) == 0 && lim.rlim_cur != lim.rlim_max) {
    // Do a binary search for the limit.
    rlim_t min = lim.rlim_cur;
    rlim_t max = 1 << 20;
    // But if there's a defined upper bound, don't search, just set it.
    if (lim.rlim_max != RLIM_INFINITY) {
      min = lim.rlim_max;
      max = lim.rlim_max;
    }
    do {
      lim.rlim_cur = min + (max - min) / 2;
      if (setrlimit(RLIMIT_NOFILE, &lim)) {
        max = lim.rlim_cur;
      } else {
        min = lim.rlim_cur;
      }
    } while (min + 1 < max);
  }
#endif  // __POSIX__
#ifdef _WIN32
  for (int fd = 0; fd <= 2; ++fd) {
    auto handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (handle == INVALID_HANDLE_VALUE ||
        GetFileType(handle) == FILE_TYPE_UNKNOWN) {
      // Ignore _close result. If it fails or not depends on used Windows
      // version. We will just check _open result.
      _close(fd);
      if (fd != _open("nul", _O_RDWR))
        ABORT();
    }
  }
#endif  // _WIN32
}

std::mutex init_mutex;

int NodeInstance::StartInstance(int argc, char *argv[]) {
  atexit([] () { uv_tty_reset_mode(); });
  PlatformInit();
  node::performance::performance_node_start = PERFORMANCE_NOW();

  CHECK_GT(argc, 0);

  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);

  // This needs to run *before* V8::Initialize().  The const_cast is not
  // optional, in case you're wondering.
  int exec_argc;
  const char** exec_argv;
    
  {
    std::unique_lock<std::mutex> lk(init_mutex);
    Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);
  }

#if HAVE_OPENSSL
  {
    std::string extra_ca_certs;
    if (SafeGetenv("NODE_EXTRA_CA_CERTS", &extra_ca_certs))
      crypto::UseExtraCaCerts(extra_ca_certs);
  }
#ifdef NODE_FIPS_MODE
  // In the case of FIPS builds we should make sure
  // the random source is properly initialized first.
  OPENSSL_init();
#endif  // NODE_FIPS_MODE
  // V8 on Windows doesn't have a good source of entropy. Seed it from
  // OpenSSL's pool.
  V8::SetEntropySource(crypto::EntropySource);
#endif  // HAVE_OPENSSL

#ifdef __ANDROID__
  ContextGroup::init_v8();
#else
  V8::Initialize();
#endif

  node::performance::performance_v8_start = PERFORMANCE_NOW();
  v8_initialized = true;
  uv_loop_t uv_loop;
  uv_loop_init(&uv_loop);
  const int exit_code =
      StartInstance(&uv_loop, argc, argv, exec_argc, exec_argv);
  if (trace_enabled) {
    v8_platform.StopTracingAgent();
  }
  v8_initialized = false;

  //CHECK_EQ(uv_loop_alive(&uv_loop), 0);
  //FIXME: why does uncommenting this lead to spurious death?
  //uv_loop_close(&uv_loop);

  delete[] exec_argv;
  exec_argv = nullptr;

  return exit_code;
}
