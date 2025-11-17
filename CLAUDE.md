# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LiquidCore enables Node.js virtual machines to run inside Android and iOS apps. It provides a complete runtime environment including a virtual file system. The project consists of two main modules:

- **liquidcore-Nodejs** (`LiquidCore/`): Full Node.js runtime with MicroService API
- **liquidcore-V8** (`LiquidV8/`): Raw JavaScript engine for Android

## Architecture Overview

### Multi-Platform Design

The codebase is organized into platform-specific directories:
- `LiquidCore/src/main/` - Android implementation (Java/C++)
- `LiquidCore/src/ios/` - iOS implementation (Objective-C/Swift/C++)
- `LiquidCore/src/common/` - Shared C++ code between platforms
- `LiquidV8/src/` - V8 JavaScript engine wrapper for Android

### Layer Stack

```
┌─────────────────────────────────────┐
│   MicroService (Java/ObjC API)      │  High-level service coordination
├─────────────────────────────────────┤
│   Process (Java/ObjC)               │  Node.js instance management
├─────────────────────────────────────┤
│   JNI/JSExport Bridge               │  Cross-language communication
├─────────────────────────────────────┤
│   C++ Bridge (node_instance)        │  Process lifecycle & threading
├─────────────────────────────────────┤
│   V8/JSC + Node.js Runtime          │  JavaScript execution
└─────────────────────────────────────┘
```

### V82JSC: The iOS V8→JavaScriptCore Shim

The most critical architectural component is `LiquidCore/src/ios/V82JSC/`, which implements a V8-to-JavaScriptCore bridge. This allows Node.js (deeply coupled to V8) to run on iOS using Apple's JavaScriptCore engine:

- **V8 API Mimicry**: Implements `v8.h` and its inlines, translating all calls to JavaScriptCore
- **Custom Memory Management**: Includes custom heap management and garbage collection (V8 inlines assume specific implementation details)
- **API Coverage**: ~90% of V8 API surface needed for Node.js, with assertions for unimplemented functions
- **App Store Compliance**: Apple prohibits dynamic code generation except via JavaScriptCore
- **Footprint**: Tiny - test app weighs just 2.3 MB vs V8's much larger binary

### Node.js Integration Strategy

The project embeds Node.js 10.15.3 (`deps/node-10.15.3/`) with minimal modifications to maintain upgrade path:

**Key Modifications** (in `LiquidCore/src/common/`):
- `node_file.cc`: Intercepts all POSIX filesystem calls for virtualization
- `node_instance.cc`: Custom process lifecycle, event loop integration, exit handlers
- `liquid_file.cc`: Path resolution and access control enforcement
- `process_wrap.cc`: Process management customizations

**Dependencies**:
- Boost 1.66.0 for C++ utilities
- JavaScriptCore headers for iOS
- OpenSSL, c-ares, http_parser, nghttp2 (bundled with Node)

### Platform-Specific Modules

**Android:**
- `org.liquidplayer.node.*` - Node.js process and filesystem management
- `org.liquidplayer.service.*` - MicroService API for app integration
- `org.liquidplayer.javascript.*` - V8 wrapper (LiquidV8 module)

**iOS:**
- `API/` - Public Objective-C/Swift interfaces (LCMicroService, FileSystem, LCProcess)
- `LiquidCore/` - Core implementation
- `V82JSC/` - V8 compatibility layer

## Deep Architecture: Cross-Language Bridge

### JNI Value Encoding (Android)

To minimize JNI overhead, LiquidCore uses an optimized encoding scheme in `LiquidV8/src/main/java/org/liquidplayer/javascript/JNIJSValue.java` that packs JavaScript values into 64-bit longs:

```
xxx 00 = 62-bit double (fits directly in jlong)
xxx 10 = Oddball value (special constants)
xxx 01 = 4-byte aligned pointer to non-Object JSValue
xxx 11 = 4-byte aligned pointer to Object JSValue

Oddball Values:
0x2  = Undefined
0x6  = Null
0xa  = False
0xe  = True
```

**Benefits**: Primitives (numbers, booleans, null, undefined) don't require JNI calls. Only strings and objects cross the JNI boundary as pointers. This dramatically reduces JNI overhead.

**C++ Mirroring**: `LiquidV8/src/main/cpp/JNI/SharedWrap.h` implements the C++ side of this encoding with template specializations for encoding/decoding.

### JNI Method Registration Pattern (Android)

**Macro-Based Registration** (`LiquidV8/src/main/cpp/JNI/JNI.h`):
```cpp
#define NATIVE(package,rt,f) extern "C" JNIEXPORT \
    rt JNICALL Java_org_liquidplayer_javascript_##package##_##f
```

This generates JNI method signatures automatically. For example:
- `NATIVE(JNIJSValue, jboolean, isNull)` → `Java_org_liquidplayer_javascript_JNIJSValue_isNull`

**Initialization**: `JNI_OnLoad()` in `LiquidV8/src/main/cpp/JNI/JNI_OnLoad.cpp` stores global ClassLoader reference for dynamic class resolution across packages.

### Thread Synchronization Model

**V8 Isolate Locking** (`LiquidV8/src/main/cpp/Common/Macros.h`):
```cpp
V8_ISOLATE_CTX(ctx, iso, Ctx) {
    // Acquires v8::Locker for exclusive isolate access
    // Creates Isolate::Scope and HandleScope
    // Creates Context::Scope
    // ... your code ...
} V8_UNLOCK()
    // Schedules execution on event loop via group->sync()
```

**Cross-Thread Communication**:
1. If calling from JS thread → execute immediately
2. If calling from different thread:
   - Queue runnable in `std::vector<>`
   - Trigger `uv_async_send()` to wake event loop
   - For sync calls: block caller with semaphore until completion
   - For async calls: return immediately

See "Key File Reference Map" section for all important files.

## Building

### Android

Build both modules:
```bash
./gradlew :liquidcore-Nodejs:assembleRelease
./gradlew :liquidcore-V8:assembleRelease
```

Build outputs are AAR files in `LiquidCore/build/outputs/aar/` and `LiquidV8/build/outputs/aar/`.

The build process includes special handling for 16KB page size support (recent Android versions):
- Native libraries (.so files) are aligned to 16KB boundaries using zipalign
- CMake builds with `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON`
- AAR packaging uses uncompressed storage and explicit alignment

Run Android tests:
```bash
./gradlew :liquidcore-Nodejs:connectedAndroidTest
./gradlew :liquidcore-V8:connectedAndroidTest
```

### iOS

iOS builds use CocoaPods (required - no SPM/Carthage support):
- `LiquidCore.podspec` - Main framework
- `LiquidCore-headers.podspec` - Development headers
- **Must use** `use_frameworks!` in Podfile (Swift/ObjC interop requirement)

Build scripts in `LiquidCore/src/ios/`:
- `generate_node_javascript.sh` - Bundles Node.js JavaScript modules
- `generate_javascript_polyfills.sh` - Creates polyfills
- `copy_headers.sh` - Prepares header files

**Important**: After adding `liquidcore.entry` in `package.json`, run `npx liquidcore bundler --platform=ios` then `pod install` (files must exist at install time).

Run iOS tests via Xcode or:
```bash
xcodebuild test -workspace YourApp.xcworkspace -scheme LiquidCore -destination 'platform=iOS Simulator,name=iPhone 14'
```

### Native Code (CMake)

C++ code is built via CMake for Android:
- `LiquidCore/src/main/cpp/CMakeLists.txt` - Node.js native module
- `LiquidV8/src/main/cpp/CMakeLists.txt` - V8 wrapper

The CMakeLists compiles:
- All Node.js dependencies (c-ares, http_parser, nghttp2, OpenSSL, uv, zlib)
- Custom Node.js integration code (`node_file.cc`, `node_instance.cc`, `process_wrap.cc`, etc.)
- Platform-specific filesystem abstractions (`liquid_file.cc`)

## NPM Integration

The `package.json` at the root provides CLI tools for app developers:
```bash
npx liquidcore init              # Initialize project
npx liquidcore gradle-config     # Configure Android module
npx liquidcore pod-config        # Configure iOS Podfile
npx liquidcore bundler          # Bundle JavaScript with Metro
npx liquidcore server           # Development server for hot-reload
```

These tools help developers bundle JavaScript code into mobile apps using Metro bundler.

## Deep Architecture: Virtual Filesystem

### Filesystem Virtualization Strategy

LiquidCore intercepts all POSIX filesystem operations from Node.js and redirects them to a sandboxed directory structure. This is a multi-layer interception system:

**Layer 1: JavaScript Helpers** (`FileSystem.java`/`FileSystem.m`)
- `fs_.fs(path)` - Translates virtual paths to real paths and enforces permissions
- `fs_.alias(path)` - Translates real paths back to virtual paths
- `fs_.aliases_` - Map of virtual→real path translations
- `fs_.access_` - Permission bitmask for each virtual path
- `fs_.cwd` - Current working directory (virtual)

**Layer 2: V8 Private Key Binding** (`liquid_file.cc`, `jni_process.cc`)
- FileSystem object attached to V8 global context via `v8::Private` key `"__fs"`
- Inaccessible from JavaScript but available to C++ code
- Survives across contexts within same isolate

**Layer 3: C++ Interception** (`liquid_file.cc`):
```cpp
Local<Value> fs_(Environment *env, Local<Value> path, int req_access) {
    // 1. Retrieve fs object from private key
    // 2. Make path absolute (prepend cwd if relative)
    // 3. Resolve symlinks via uv_fs_realpath()
    // 4. Call fs_.fs(path) → returns [access_mask, real_path]
    // 5. Check permissions: (req_access & access_mask) == req_access
    // 6. Return translated real_path or throw EACCES
}
```

**Layer 4: Node.js Filesystem Interception** (`node_file.cc`):
Every POSIX operation goes through `fs_()`:
- `Open()`, `Stat()`, `LStat()`, `Unlink()`, `RMDir()`, `MKDir()`, `Rename()`, `CopyFile()`, `ReadDir()`, etc.
- Access levels: `_FS_ACCESS_RD` (read), `_FS_ACCESS_WR` (write), or both

### Virtual Directory Structure

Each MicroService instance sees:
```
/home/                          [Read-only root]
├── node_modules/               [Read-only] Bundled Node modules
├── module/                     [Read-only] Downloaded JS code
├── temp/                       [Read/Write] Session-temporary (cleared on exit)
├── cache/                      [Read/Write] Service-wide cache
├── local/                      [Read/Write] Persistent local storage
└── public/                     [Varies] External storage access
    ├── data/                   [Read/Write] Public app data
    └── media/                  [Varies] Pictures, Movies, Music, etc.
```

**Real Filesystem Mapping** (Android):
- Session temp: `cache_dir/__org.liquidplayer.node__/sessions/{sessionID}/temp`
- Session home: `cache_dir/__org.liquidplayer.node__/sessions/{sessionID}/home`
- Service local: `files_dir/__org.liquidplayer.node__/_{uniqueID}/local`
- Service cache: `cache_dir/__org.liquidplayer.node__/_{uniqueID}/cache`
- Service module: `files_dir/__org.liquidplayer.node__/_{uniqueID}/module`

**Isolation Guarantees**:
1. **Path Escape Prevention**: Symlink resolution within sandbox, alias mapping whitelist
2. **Per-Instance Isolation**: Each MicroService gets unique session ID
3. **Permission Enforcement**: Bitmask checked before every syscall
4. **No Cross-Service Access**: Session paths completely isolated by UUID

### Custom Working Directory

Node's `process.chdir()` and `process.cwd()` are overridden in `node_instance.cc`:
- **NOT** changing actual process working directory (would affect all isolates)
- **Instead**: Updates `fs_.cwd` property in filesystem object
- All relative paths resolved against this virtual cwd
- Each isolate maintains independent cwd state

Key files referenced in "Key File Reference Map" section.

## Deep Architecture: Process Lifecycle

### Multi-Layer Process Management

**Process Creation Flow**:
```
MicroService.start()
    ↓
Process.__init__() [Java/ObjC]
    ↓
start() [JNI] → Creates iOSInstance/AndroidInstance [C++]
    ↓
Thread.start() → Dedicated "nodejs" thread
    ↓
runInThread(processRef) [JNI]
    ↓
iOSInstance::spawnedThread() [C++]
    ↓
node::Start(argc, argv) [Node.js core]
    ↓
NodeInstance::NotifyStart(ctxRef, group) [Callback]
    ↓
onNodeStarted() [Native→JS callback]
    ↓
__nodedroid_onLoad() [JavaScript setup]
    ↓
onProcessStart() [JS→Native callback]
    ↓
MicroService.onProcessStart() [Java/ObjC - ready to run code]
```

### Thread Architecture

**One Thread Per Process**:
- Each Process gets dedicated thread (named "nodejs" on Android)
- Thread runs libuv event loop: `uv_run(env.event_loop(), UV_RUN_DEFAULT)`
- All async operations (timers, I/O, callbacks) run on this thread
- Thread local storage maps `std::thread::id` to instance pointer

**Event Loop Integration**:
- **Android**: Standard libuv loop (`UV_RUN_DEFAULT`)
- **iOS**: Cooperative with CFRunLoop - `uv_run(UV_RUN_NOWAIT)` + `CFRunLoopRunInMode()`
- V8 platform tasks drained between loop iterations
- iOS: Periodic scavenging GC every 5 seconds

### JavaScript Context Initialization

**Critical Sequence** (from `node_instance.cc`):
1. Create V8 Isolate (or JSC context group on iOS)
2. Create V8 Context with custom ObjectTemplate
3. Setup Environment with process, global, require
4. **Override** process methods:
   - `process.chdir` → `fs::Chdir` (virtual cwd)
   - `process.cwd` → `fs::Cwd` (virtual cwd)
   - `process.reallyExit` → `LiquidExit` (stop event loop, don't exit process)
   - `process.abort` → `LiquidExit`
5. Call `NodeInstance::NotifyStart()` → triggers platform callback
6. Platform creates FileSystem and injects `__nodedroid_onLoad`
7. Load Node.js environment → executes `__nodedroid_onLoad()`
8. Setup complete → `onProcessStart` callback to platform
9. Run main event loop until no more work or explicit exit

### Process Exit & Cleanup

**Custom Exit Handler** (`node_instance.cc`):
```cpp
void LiquidExit(const FunctionCallbackInfo<Value>& args) {
    // Don't exit the host process - we're embedded
    // Instead: unreference all handles and stop event loop
    uv_walk(env->event_loop(), [](uv_handle_t* h, void*) {
        uv_unref(h);  // Mark all handles as non-blocking
    }, nullptr);
    uv_stop(env->event_loop());  // Stop loop
}
```

**Cleanup Sequence**:
1. `LiquidExit()` stops event loop
2. Loop drains remaining work
3. `EmitExit()` triggers 'exit' event
4. `onProcessAboutToExit()` callback → platform (can prevent exit)
5. `onProcessExit()` callback → platform
6. `env.RunCleanup()` - cleanup handlers
7. `RunAtExit()` - atexit callbacks
8. Platform calls `FileSystem.cleanUp()` - session cleanup
9. C++ instance deleted, thread ends

**Keep-Alive Mechanism**: `process.keepAlive()` returns handle that prevents event loop exit even when no work queued. Call `handle.letDie()` to allow natural exit.

## Deep Architecture: Event Communication

### Event System Architecture

LiquidCore provides bidirectional event communication between JavaScript and native code using the EventEmitter pattern:

**JavaScript Side** (`MicroService.onProcessStart`):
```javascript
// Global LiquidCore EventEmitter created in context
class LiquidCore extends require('events') {}
global.LiquidCore = new LiquidCore();

// JS → Native
LiquidCore.emit('my_event', {foo: 'bar', num: 42});

// Native → JS
LiquidCore.on('host_event', function(payload) {
    console.log('Received:', payload.message);
});
```

**Native Side** (Android):
```java
// Native → JS
JSONObject payload = new JSONObject();
payload.put("message", "Hello from Java");
service.emit("host_event", payload);

// JS → Native
MicroService.EventListener listener = (service, event, payload) -> {
    String foo = payload.getString("foo");
    int num = payload.getInt("num");
};
service.addEventListener("my_event", listener);
```

### Event Flow: JavaScript → Native

1. `LiquidCore.emit('event', data)` in JavaScript
2. EventEmitter triggers registered listeners
3. Native listener is JSFunction callback wrapper
4. `JSFunction::FunctionCallback()` invoked by V8/JSC
5. Marshals V8/JSC values to Java/ObjC types
6. Calls registered EventListener on native side

**Type Marshaling** (Android):
- JavaScript Object → JSONObject (via safe JSON.stringify with circular reference detection)
- JavaScript primitives → Java primitives (Number→double, String→String, Boolean→boolean)
- JavaScript Arrays → JSONArray

**Type Marshaling** (iOS):
- Automatic via JSExport bridge (JSValue → NSNumber, NSString, NSArray, NSDictionary)

### Event Flow: Native → JavaScript

1. `service.emit("event", payload)` on native side
2. Payload converted to JSON string (Android) or JSValue (iOS)
3. Executed on JS thread via sync/async call
4. `LiquidCore.emit()` called with parsed payload
5. JavaScript listeners invoked

**Thread Safety**:
- **Android**: All JS calls queued via ContextGroup.sync() - blocks until complete
- **iOS**: Explicit `[process sync:]` for blocking or `[process async:]` for non-blocking

### MicroService Layer

MicroService wraps Process and provides service-oriented features:
- **Service Code Loading**: Fetches JavaScript from URI, caches locally
- **require() Override**: Intercepts native module loading for AddOn system
- **Event Listener Management**: Synchronized map of event→listeners
- **Service Registry**: Global map of serviceId→MicroService instances

## Common Development Tasks

### Updating Node.js Version

1. Replace `deps/node-10.15.3/` with new Node.js source
2. Update all CMakeLists.txt path references (search for "node-10.15.3")
3. Update references in:
   - `LiquidCore/build.gradle` (if version paths hardcoded)
   - iOS build scripts (`generate_node_javascript.sh`, etc.)
4. Regenerate JavaScript bundles with iOS build scripts
5. **Critical**: Test V82JSC extensively - new V8 API usage may require shim updates
6. Update version in `package.json`

**Watch For**:
- New V8 API calls in Node.js that aren't shimmed in V82JSC (will assert)
- Changes to V8 inline behavior (V82JSC replicates implementation details)
- Changes to libuv that affect event loop integration

### Adding Native Node Modules

For modules requiring native code:
1. Add C/C++ source files to CMakeLists.txt SOURCES list
2. Update include paths in CMakeLists.txt
3. For Node.js built-in modules:
   - Modify `node_instance.cc` to register module
   - Add initialization call in `Start()` function
4. For AddOn modules:
   - Implement `org.liquidplayer.addon.*` package
   - Implement `AddOn` interface with `register()` and `require()` methods
5. **iOS Only**: Check if module uses V8 APIs - may need V82JSC extensions

**Filesystem Module Considerations**:
- All filesystem operations go through `fs_()` interception
- If module does direct syscalls (bypassing Node.js fs), it won't be sandboxed
- Consider whether module needs access outside sandbox (requires `exposeRawFS()`)

### Modifying Filesystem Virtualization

**Adding New Virtual Paths**:
1. Update `FileSystem.java` (Android) or `FileSystem.m` (iOS)
2. Add new `alias()` or `mkdir()` call with path mapping and permission mask
3. Update `fs_.aliases_` and `fs_.access_` dictionaries
4. No changes needed to `liquid_file.cc` (uses dynamic lookup)

**Changing Access Permissions**:
- Modify permission mask in `alias()` call
- Masks: `_FS_ACCESS_RD` (1), `_FS_ACCESS_WR` (2), or both (3)
- Changes take effect immediately on next path resolution

**Exposing Raw Filesystem** (use sparingly):
```java
process.exposeRawFS("/external/pictures",
    Process.kMediaAccessPermissionsRW);
```
This bypasses sandboxing for specific real paths - **security risk**.

### Debugging Native Code

**Android:**
- Use Android Studio with NDK debugging enabled
- Set breakpoints in C++ files via IDE
- Check `adb logcat -s "nodejs:V" "liquidcore:V"` for logging
- For JNI crashes: `adb logcat -s "DEBUG:*"`
- Thread name is "nodejs" for event loop thread

**iOS:**
- Use Xcode with LiquidCore scheme
- Enable C++ exception breakpoints in Breakpoint Navigator
- V82JSC assertions fail loudly on unimplemented APIs (check console)
- For V82JSC debugging: Set breakpoints in `V82JSC/*.cpp` to trace V8→JSC translation
- Use Instruments for memory leak detection (watch for retain cycles)

**Common Issues**:
- **Deadlock**: Check for sync calls from JS thread (already on event loop)
- **Segfault in V82JSC**: Likely unimplemented V8 API or incorrect handle scope
- **FileSystem EACCES**: Check `fs_.access_` permissions and path mapping
- **Process won't exit**: Check for unreferenced timers or I/O (use `keepAlive().letDie()`)

### Working with JNI Bridge (Android)

**Adding New JNI Methods**:
1. Declare `native` method in Java class (e.g., `JNIJSValue.java`)
2. Implement in C++ using `NATIVE(Package, ReturnType, methodName)` macro
3. Use `PARAMS` for instance methods, `STATIC` for static methods
4. Always acquire V8 lock: `V8_ISOLATE_CTX(ctx, isolate, context) { ... } V8_UNLOCK()`
5. Use `SharedWrap<T>::New()` to encode C++ objects as jlong
6. Use `SharedWrap<T>::Shared()` to decode jlong to C++ objects

**Example**:
```cpp
NATIVE(JNIJSValue, jboolean, isString) (STATIC, jlong valueRef) {
    auto value = SharedWrap<JSValue>::Shared(boost::shared_ptr<JSContext>(), valueRef);
    bool result = false;
    V8_ISOLATE_CTX(value->Context(), isolate, context)
        result = value->Value()->IsString();
    V8_UNLOCK()
    return (jboolean)result;
}
```

### Testing

Test locations: `LiquidCore/src/androidTest/java/`, `LiquidV8/src/androidTest/java/` (Android), `LiquidCore/src/ios/Tests/*.m` (iOS)

Coverage: MicroService lifecycle, filesystem operations, JavaScript execution, event communication, thread safety.

Run commands in Building section above.

## Critical Architecture Constraints

### Node.js Modification Philosophy
- **Minimize changes** to Node.js source to ease upgrades
- All customizations in `LiquidCore/src/common/` (separate from Node.js tree)
- Override process methods rather than modifying Node.js internals
- Keep track of all modifications in commit messages

### V82JSC Completeness
- **~90% of V8 API** implemented, focused on Node.js requirements
- Unimplemented functions **fail with assertions** (not silent failures)
- If you hit an assertion, file an issue with:
  - V8 API function being called
  - Call stack leading to the call
  - Node.js module triggering the call
- Some esoteric V8 features not implemented (e.g., some compiler hints, advanced debugging APIs)

### Memory Management
- **Android**: V8's garbage collector manages JavaScript heap
- **iOS**: Custom GC in V82JSC, periodic scavenging every 5 seconds
- **Zombie Queue**: C++ objects deferred cleanup to avoid use-after-free during GC
- **Weak References**: Use `WeakGlobalRef` in JNI to prevent circular references
- **Keep-Alive**: Must explicitly manage process lifetime with `keepAlive()`/`letDie()`

### Thread Safety
- **One thread per Process**: Never access V8/JSC from multiple threads
- **Event Loop Thread**: All JavaScript execution on dedicated "nodejs" thread
- **Cross-Thread Calls**: Always use `sync()`/`async()` from other threads
- **Isolate Lock**: `v8::Locker` prevents concurrent access to isolate
- **Platform Callbacks**: Can be called from any thread, must re-dispatch to JS thread

### 16KB Page Size Support (Android)
- **Critical for Android 15+**: Devices may use 16KB pages instead of 4KB
- All .so files must be **aligned to 16KB boundaries**
- AAR packaging uses **uncompressed storage** to preserve alignment
- Build scripts use `zipalign -p 16384` for proper alignment
- CMake flag: `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON`
- **Never modify** AAR packaging or alignment steps without testing on 16KB device

### iOS Platform Constraints
- Metro bundler **pinned to 0.57.0** for stability
- See Building > iOS section for CocoaPods requirements

## Version Information

Current versions:
- Node.js: 10.15.3
- Boost: 1.66.0
- Project version: Managed in `package.json` (currently 0.7.10)
- Android: minSdk 21, targetSdk 34, compileSdk 34
- iOS: Deployment target 11.0
- Metro bundler: 0.57.0 (pinned)

## Key File Reference Map

**JNI Bridge (Android)**:
- `LiquidV8/src/main/cpp/JNI/JNI.h` - Macro definitions
- `LiquidV8/src/main/cpp/JNI/SharedWrap.h` - Value encoding
- `LiquidV8/src/main/cpp/JNI/JNI_JSValue.cpp` - Value operations
- `LiquidV8/src/main/cpp/JNI/JSFunction.cpp` - Function callbacks

**Process Lifecycle**:
- `LiquidCore/src/common/node_instance.cc` - Core orchestration
- `LiquidCore/src/ios/node_bridge.cc` - iOS wrapper
- `LiquidCore/src/main/cpp/jni_process.cc` - Android wrapper

**Filesystem Virtualization**:
- `LiquidCore/src/common/liquid_file.cc` - Path interception
- `LiquidCore/src/common/node_file.cc` - POSIX interception points
- `LiquidCore/src/main/java/org/liquidplayer/node/FileSystem.java` - Android setup
- `LiquidCore/src/ios/API/FileSystem.m` - iOS setup

**MicroService API**:
- `LiquidCore/src/main/java/org/liquidplayer/service/MicroService.java` - Android
- `LiquidCore/src/ios/API/LCMicroService.m` - iOS
