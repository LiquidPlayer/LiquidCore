//
// Created by Eric on 9/9/16.
//
#include "NodeInstance.h"

#include "nodedroid_file.h"

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

NodeInstance::NodeInstance(JNIEnv* env, jobject thiz) {
    env->GetJavaVM(&m_jvm);
    m_JavaThis = env->NewGlobalRef(thiz);
    node_main_thread = new std::thread(node_main_task,reinterpret_cast<void*>(this));
}

NodeInstance::~NodeInstance() {
    node_main_thread->join();
    delete node_main_thread;
}

void NodeInstance::spawnedThread()
{
    enum { kMaxArgs = 64 };
    char cmd[] = "node -e global.__nodedroid_onLoad();";

    int argc = 0;
    char *argv[kMaxArgs];

    char *p2 = strtok(cmd, " ");
    while (p2 && argc < kMaxArgs-1)
      {
        argv[argc++] = p2;
        p2 = strtok(0, " ");
      }
    argv[argc] = 0;

    int ret = Start(argc, argv);

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

    // Commit suicide
    //delete this;
}

void NodeInstance::node_main_task(void *inst) {
    reinterpret_cast<NodeInstance*>(inst)->spawnedThread();
}

void NodeInstance::PumpMessageLoop(Isolate* isolate) {
    v8::platform::PumpMessageLoop(ContextGroup::Platform(), isolate);
}

bool NodeInstance::StartInspector(Environment *env, int port, bool wait) {
#if HAVE_INSPECTOR
    return env->inspector_agent()->Start(ContextGroup::Platform(), port, wait);
#else
    return true;
#endif  // HAVE_INSPECTOR
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

  int err = uv_chdir(*path);
  if (err) {
    return env->ThrowUVException(err, "uv_chdir");
  }
}


void NodeInstance::Cwd(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
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
  Local<Value> aliased = nodedroid::alias_(env, cwd);
  args.GetReturnValue().Set(aliased);
}



Environment* NodeInstance::CreateEnvironment(Isolate* isolate,
                                      Local<Context> context,
                                      NodeInstanceData* instance_data) {
  return node::CreateEnvironment(isolate,
                           instance_data->event_loop(),
                           context,
                           instance_data->argc(),
                           instance_data->argv(),
                           instance_data->exec_argc(),
                           instance_data->exec_argv());
}

int NodeInstance::StartNodeInstance(void* arg) {
  NodeInstanceData* instance_data = static_cast<NodeInstanceData*>(arg);
  Isolate::CreateParams params;
  ArrayBufferAllocator* array_buffer_allocator = new ArrayBufferAllocator();
  params.array_buffer_allocator = array_buffer_allocator;
  Isolate* isolate = Isolate::New(params);
  int exit_code = 1;
  ContextGroup *group = nullptr;

  auto notify_start = [&] (JSContext *java_node_context) {
      JNIEnv *jenv;
      int getEnvStat = m_jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6);
      if (getEnvStat == JNI_EDETACHED) {
        m_jvm->AttachCurrentThread(&jenv, NULL);
      }

      jclass cls = jenv->GetObjectClass(m_JavaThis);
      jmethodID mid;
      do {
        mid = jenv->GetMethodID(cls,"onNodeStarted","(JJ)V");
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

      jenv->CallVoidMethod(m_JavaThis, mid, reinterpret_cast<jlong>(java_node_context),
        reinterpret_cast<jlong>(group));

      if (getEnvStat == JNI_EDETACHED) {
          m_jvm->DetachCurrentThread();
      }
  };

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    if (instance_data->is_main()) {
      CHECK_EQ(node_isolate, nullptr);
      node_isolate = isolate;
    }
  }

  {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Environment* env = CreateEnvironment(isolate, context, instance_data);
    array_buffer_allocator->set_env(env);
    Context::Scope context_scope(context);

    {
      ContextGroup::Mutex()->lock();

      // Override default chdir and cwd methods
      Local<Object> process = env->process_object();
      env->SetMethod(process, "chdir", Chdir);
      env->SetMethod(process, "cwd", Cwd);

      // Remove process.dlopen().  Nothing good can come of it in this environment.
      process->Delete(env->context(), String::NewFromUtf8(isolate, "dlopen"));

      isolate->SetAbortOnUncaughtExceptionCallback(
        ShouldAbortOnUncaughtException);

      // Start debug agent when argv has --debug
      /*
      if (instance_data->use_debug_agent()) {
        StartDebug(env, debug_wait_connect);
        if (use_inspector && !debugger_running) {
          exit(12);
        }
      }
      */

      {
        Environment::AsyncCallbackScope callback_scope(env);
        LoadEnvironment(env);
      }

      // Enable debugger
      /*
      if (instance_data->use_debug_agent())
        EnableDebug(env);
      */
      ContextGroup::Mutex()->unlock();
    }

    JSContext *java_node_context;
    {
      SealHandleScope seal(isolate);

      // call back Java via JNI and pass on the context
      {
        ContextGroup::Mutex()->lock();
        Isolate::Scope isolate_scope_(isolate);
        HandleScope handle_scope_(isolate);

        group = new ContextGroup(isolate, env->event_loop());
        java_node_context = new JSContext(group, context);
        ContextGroup::Mutex()->unlock();
      }

      java_node_context->retain();
      notify_start(java_node_context);

      bool more;
      do {
        PumpMessageLoop(isolate);
        more = uv_run(env->event_loop(), UV_RUN_ONCE);

        if (more == false) {
          PumpMessageLoop(isolate);
          EmitBeforeExit(env);

          // Emit `beforeExit` if the loop became alive either after emitting
          // event, or after running some callbacks.
          more = uv_loop_alive(env->event_loop());
          if (uv_run(env->event_loop(), UV_RUN_NOWAIT) != 0)
            more = true;
        }
      } while (more == true);
    }

    {
      ContextGroup::Mutex()->lock();

      env->set_trace_sync_io(false);

      exit_code = EmitExit(env);
      RunAtExit(env);

      java_node_context->SetDefunct();
      int count = java_node_context->release();
      ASSERT_EQ(count,0);

      WaitForInspectorDisconnect(env);
#if defined(LEAK_SANITIZER)
      __lsan_do_leak_check();
#endif

      array_buffer_allocator->set_env(nullptr);

      env->Dispose();
      env = nullptr;

      ContextGroup::Mutex()->unlock();
    }
  }

  {
    Mutex::ScopedLock scoped_lock(node_isolate_mutex);
    if (node_isolate == isolate)
      node_isolate = nullptr;
  }

  CHECK_NE(isolate, nullptr);

  isolate->Dispose();
  isolate = nullptr;

  int count = group->release();
  ASSERT_EQ(count,0);

  delete array_buffer_allocator;

  return exit_code;
}

int NodeInstance::Start(int argc, char *argv[]) {
  CHECK_GT(argc, 0);

  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);

  // This needs to run *before* V8::Initialize().  The const_cast is not
  // optional, in case you're wondering.
  int exec_argc;
  const char** exec_argv;
  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

#if HAVE_OPENSSL
#ifdef NODE_FIPS_MODE
  // In the case of FIPS builds we should make sure
  // the random source is properly initialized first.
  OPENSSL_init();
#endif  // NODE_FIPS_MODE
  // V8 on Windows doesn't have a good source of entropy. Seed it from
  // OpenSSL's pool.
  V8::SetEntropySource(crypto::EntropySource);
#endif

  ContextGroup::init_v8();

  int exit_code = 1;
  {
    uv_loop_t uv_loop;
    uv_loop_init(&uv_loop);
    NodeInstanceData instance_data(NodeInstanceType::MAIN,
                                   &uv_loop,
                                   argc,
                                   const_cast<const char**>(argv),
                                   exec_argc,
                                   exec_argv,
                                   use_debug_agent);
    exit_code = StartNodeInstance(&instance_data);
    uv_loop_close(&uv_loop);
  }

  delete[] exec_argv;
  exec_argv = nullptr;

  return exit_code;
}

#undef NATIVE
#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_node_##package##_##f

static int pfd[2];
static pthread_t thr;
static const char *tag = nullptr;

static void *thread_func(void*)
{
    ssize_t rdsz;
    char buf[128];
    while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if (rdsz > 0) {
            buf[rdsz] = 0;  /* add null-terminator */
            char *line = strtok(buf, "\n");
            while (line != nullptr) {
                __android_log_write(ANDROID_LOG_DEBUG, tag, line);
                line = strtok(nullptr, "\n");
            }
        }
    }
    return 0;
}

int start_logger(const char *app_name)
{
    tag = app_name;

    /* make stdout line-buffered and stderr unbuffered */
    setvbuf(stdout, 0, _IOLBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    /* create the pipe and redirect stdout and stderr */
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    /* spawn the logging thread */
    if(pthread_create(&thr, 0, thread_func, 0) == -1)
        return -1;
    pthread_detach(thr);
    return 0;
}

NATIVE(Process,jlong,start) (PARAMS)
{
    // One time set up
    if (tag == nullptr) {
        start_logger("nodedroid");
    }

    NodeInstance *instance = new NodeInstance(env, thiz);
    return reinterpret_cast<jlong>(instance);
}

NATIVE(Process,void,dispose) (PARAMS, jlong ref)
{
    delete reinterpret_cast<NodeInstance*>(ref);
}

NATIVE(Process,long,keepAlive) (PARAMS, jlong contextRef)
{
    auto done = [](uv_async_t* handle) {
        uv_close((uv_handle_t*)handle, [](uv_handle_t *h){
            delete (uv_async_t*)h;
        });
    };

    JSContext *context = reinterpret_cast<JSContext*>(contextRef);
    ContextGroup *group = context->Group();
    uv_async_t *async_handle = new uv_async_t();
    uv_async_init(group->Loop(), async_handle, done);
    return reinterpret_cast<jlong>(async_handle);
}

NATIVE(Process,void,letDie) (PARAMS, jlong ref)
{
    uv_async_t *async_handle = reinterpret_cast<uv_async_t*>(ref);
    uv_async_send(async_handle);
}

NATIVE(Process,void,setFileSystem) (PARAMS, jlong contextRef, jlong fsObjectRef)
{
    V8_ISOLATE_CTX(contextRef,isolate,context);

    Local<Object> globalObj = context->Global();
    Local<Object> fsObj = reinterpret_cast<JSValue<Value>*>(fsObjectRef)
        ->Value()->ToObject(context).ToLocalChecked();

    Local<Private> privateKey = v8::Private::ForApi(isolate,
        String::NewFromUtf8(isolate, "__fs"));
    globalObj->SetPrivate(context, privateKey, fsObj);

    V8_UNLOCK();
}
