/*
 * Copyright (c) 2016 - 2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "Common/ContextGroup.h"
#include "Common/JSContext.h"
#include "JNI/SharedWrap.h"
#include "JavaScriptCore/JavaScript.h"
#include "JSC/JSC.h"
#include <jni.h>
#include "node_instance.h"
#include "liquid_file.h"
#include "node.cc"

namespace node {

static std::recursive_mutex s_init_mutex;

static void InitializeNode() {
    std::unique_lock<std::recursive_mutex> lock(s_init_mutex);
    if (!v8_initialized) {
        v8_platform.Initialize(
                per_process_opts->v8_thread_pool_size);
        NodeInstance::Init();
        performance::performance_v8_start = PERFORMANCE_NOW();
        v8_initialized = true;
    }
}

static void DisposeNode() {
#if 0
    v8_platform.StopTracingAgent();
    v8_initialized = false;
    V8::Dispose();

    // uv_run cannot be called from the time before the beforeExit callback
    // runs until the program exits unless the event loop has any referenced
    // handles after beforeExit terminates. This prevents unrefed timers
    // that happen to terminate during shutdown from being run unsafely.
    // Since uv_run cannot be called, uv_async handles held by the platform
    // will never be fully cleaned up.
    v8_platform.Dispose();
#endif
}

static void LiquidExit(const FunctionCallbackInfo<Value>& args) {
    Environment *env = Environment::GetCurrent(args);
    WaitForInspectorDisconnect(env);
    //v8_platform.StopTracingAgent();
    int code = args[0]->Int32Value(env->context()).FromMaybe(0);
    // FIXME: fix exit code?

    //env->Exit(code);
    uv_walk(env->event_loop(), [](uv_handle_t* h, void* arg) {
        uv_unref(h);
    }, nullptr);
    uv_stop(env->event_loop());
}

std::recursive_mutex mp_mutex;

extern "C" void node_module_register(void* m) {
    std::unique_lock<std::recursive_mutex> lock(mp_mutex);
    struct node_module* mp = reinterpret_cast<struct node_module*>(m);

    if (mp->nm_flags & NM_F_BUILTIN) {
        if (FindModule(modlist_builtin, mp->nm_modname, mp->nm_flags)) return;
        mp->nm_link = modlist_builtin;
        modlist_builtin = mp;
    } else if (mp->nm_flags & NM_F_INTERNAL) {
        if (FindModule(modlist_internal, mp->nm_modname, mp->nm_flags)) return;
        mp->nm_link = modlist_internal;
        modlist_internal = mp;
    } else if (!node_is_initialized) {
        // "Linked" modules are included as part of the node project.
        // Like builtins they are registered *before* node::Init runs.
        mp->nm_flags = NM_F_LINKED;
        if (FindModule(modlist_linked, mp->nm_modname, mp->nm_flags)) return;
        mp->nm_link = modlist_linked;
        modlist_linked = mp;
    } else {
        uv_key_set(&thread_local_modpending, mp);
    }
}

struct node_module* FindModule(struct node_module* list,
                                      const char* name,
                                      int flag) {
    std::unique_lock<std::recursive_mutex> lock(mp_mutex);
    struct node_module* mp;

    for (mp = list; mp != nullptr; mp = mp->nm_link) {
        if (strcmp(mp->nm_modname, name) == 0)
            break;
    }

    CHECK(mp == nullptr || (mp->nm_flags & flag) != 0);
    return mp;
}


Local<Context> NewContext(Isolate* isolate,
                          Local<ObjectTemplate> object_template,
                          JSContextGroupRef group,
                          JSGlobalContextRef *ctxRef) {
    Local<Context> context;

    if (!group) {
        context = Context::New(isolate, nullptr, object_template);
    } else {
        context = NodeInstance::NewContext(isolate, group, ctxRef);
    }
    /*--*/
    if (context.IsEmpty()) return context;
    HandleScope handle_scope(isolate);

    context->SetEmbedderData(
            ContextEmbedderIndex::kAllowWasmCodeGeneration, True(isolate));

    {
        // Run lib/internal/per_context.js
        Context::Scope context_scope(context);
        Local<String> per_context = NodePerContextSource(isolate);
        ScriptCompiler::Source per_context_src(per_context, nullptr);
        Local<Script> s = ScriptCompiler::Compile(
                context,
                &per_context_src).ToLocalChecked();
        s->Run(context).ToLocalChecked();
    }

    return context;
}

Local<Context> NewContext(Isolate* isolate,
                          Local<ObjectTemplate> object_template) {
    return NewContext(isolate, object_template, nullptr, nullptr);
}

int Start(Isolate* isolate, IsolateData* isolate_data,
                 const std::vector<std::string>& args,
                 const std::vector<std::string>& exec_args) {

    HandleScope handle_scope(isolate);
    /* LC
    Local<Context> context = NewContext(isolate);
    */
    auto group = NodeInstance::GroupFromIsolate(isolate, isolate_data->event_loop());
    JSGlobalContextRef ctxRef;
    Local<Context> context = NewContext(isolate, Local<ObjectTemplate>(), group, &ctxRef);
    /*--*/
    Context::Scope context_scope(context);
    Environment env(isolate_data, context, v8_platform.GetTracingAgentWriter());
    env.Start(args, exec_args, v8_is_profiling);

    /* ===Start */
    // Override default chdir and cwd methods
    Local<Object> process = env.process_object();
    env.SetMethod(process, "chdir", fs::Chdir);
    env.SetMethod(process, "cwd", fs::Cwd);

    // Override DLOpen with our own implementation
    env.SetMethod(process, "dlopen", DLOpen);

    // Override exit() and abort() so they don't nuke the app
    env.SetMethod(process, "reallyExit", LiquidExit);
    env.SetMethod(process, "abort", LiquidExit);
    env.SetMethod(process, "_kill", LiquidExit);
    NodeInstance::NotifyStart(ctxRef, group);

    /* ===End */

    const char* path = args.size() > 1 ? args[1].c_str() : nullptr;
    StartInspector(&env, path, env.options()->debug_options);

    if (env.options()->debug_options->inspector_enabled &&
        !v8_platform.InspectorStarted(&env)) {
        return 12;  // Signal internal error.
    }

    {
        Environment::AsyncCallbackScope callback_scope(&env);
        env.async_hooks()->push_async_ids(1, 0);
        LoadEnvironment(&env);
        env.async_hooks()->pop_async_id(1);
    }

    {
        SealHandleScope seal(isolate);
        bool more;
        env.performance_state()->Mark(
                node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_START);
        do {
            uv_run(env.event_loop(), UV_RUN_DEFAULT);

            v8_platform.DrainVMTasks(isolate);

            more = uv_loop_alive(env.event_loop());
            if (more)
                continue;

            RunBeforeExit(&env);

            // Emit `beforeExit` if the loop became alive either after emitting
            // event, or after running some callbacks.
            more = uv_loop_alive(env.event_loop());
        } while (more == true);
        env.performance_state()->Mark(
                node::performance::NODE_PERFORMANCE_MILESTONE_LOOP_EXIT);
    }

    env.set_trace_sync_io(false);

    const int exit_code = EmitExit(&env);

    WaitForInspectorDisconnect(&env);

    env.set_can_call_into_js(false);
    env.stop_sub_worker_contexts();
    uv_tty_reset_mode();
    env.RunCleanup();
    RunAtExit(&env);

    v8_platform.DrainVMTasks(isolate);
    v8_platform.CancelVMTasks(isolate);
#if defined(LEAK_SANITIZER)
    __lsan_do_leak_check();
#endif

    return exit_code;
}

int Start(int argc, char** argv) {
    atexit([] () { uv_tty_reset_mode(); });

    {
        std::unique_lock<std::recursive_mutex> lock(s_init_mutex);
        PlatformInit();
        performance::performance_node_start = PERFORMANCE_NOW();
    }

    CHECK_GT(argc, 0);

#ifdef NODE_ENABLE_LARGE_CODE_PAGES
    if (node::IsLargePagesEnabled()) {
    if (node::MapStaticCodeToLargePages() != 0) {
      fprintf(stderr, "Reverting to default page size\n");
    }
  }
#endif

    // Hack around with the argv pointer. Used for process.title = "blah".
    argv = uv_setup_args(argc, argv);

    std::vector<std::string> args(argv, argv + argc);
    std::vector<std::string> exec_args;
    {
        std::unique_lock<std::recursive_mutex> lock(s_init_mutex);
        // This needs to run *before* V8::Initialize().
        Init(&args, &exec_args);

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

        /* LC
        v8_platform.Initialize(
                per_process_opts->v8_thread_pool_size);
        V8::Initialize();
        performance::performance_v8_start = PERFORMANCE_NOW();
        v8_initialized = true;
        */
        InitializeNode();
    }
    uv_loop_t uv_loop;
    uv_loop_init(&uv_loop);
    /*--*/
    /* LC
    const int exit_code =
            Start(uv_default_loop(), args, exec_args);

    v8_platform.StopTracingAgent();
    v8_initialized = false;
    V8::Dispose();

    // uv_run cannot be called from the time before the beforeExit callback
    // runs until the program exits unless the event loop has any referenced
    // handles after beforeExit terminates. This prevents unrefed timers
    // that happen to terminate during shutdown from being run unsafely.
    // Since uv_run cannot be called, uv_async handles held by the platform
    // will never be fully cleaned up.
    v8_platform.Dispose();
    */
    const int exit_code =
            Start(&uv_loop, args, exec_args);
    DisposeNode();
    /*--*/

    return exit_code;
}

}