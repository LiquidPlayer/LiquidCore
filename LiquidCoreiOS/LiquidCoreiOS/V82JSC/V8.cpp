//
//  V8.cpp
//  LiquidCore
//
//  Created by Eric Lange on 1/28/18.
//  Copyright © 2018 LiquidPlayer. All rights reserved.
//

#include <v8.h>

using namespace v8;

/**
 * Hand startup data to V8, in case the embedder has chosen to build
 * V8 with external startup data.
 *
 * Note:
 * - By default the startup data is linked into the V8 library, in which
 *   case this function is not meaningful.
 * - If this needs to be called, it needs to be called before V8
 *   tries to make use of its built-ins.
 * - To avoid unnecessary copies of data, V8 will point directly into the
 *   given data blob, so pretty please keep it around until V8 exit.
 * - Compression of the startup blob might be useful, but needs to
 *   handled entirely on the embedders' side.
 * - The call will abort if the data is invalid.
 */
void V8::SetNativesDataBlob(StartupData* startup_blob)
{
    
}
void V8::SetSnapshotDataBlob(StartupData* startup_blob)
{
    
}

/**
 * Bootstrap an isolate and a context from scratch to create a startup
 * snapshot. Include the side-effects of running the optional script.
 * Returns { NULL, 0 } on failure.
 * The caller acquires ownership of the data array in the return value.
 */
StartupData V8::CreateSnapshotDataBlob(const char* embedded_source)
{
    return StartupData();
}

/**
 * Bootstrap an isolate and a context from the cold startup blob, run the
 * warm-up script to trigger code compilation. The side effects are then
 * discarded. The resulting startup snapshot will include compiled code.
 * Returns { NULL, 0 } on failure.
 * The caller acquires ownership of the data array in the return value.
 * The argument startup blob is untouched.
 */
StartupData V8::WarmUpSnapshotDataBlob(StartupData cold_startup_blob,
                                          const char* warmup_source)
{
    return StartupData();
}

/**
 * Sets V8 flags from a string.
 */
void V8::SetFlagsFromString(const char* str, int length)
{
    
}

/**
 * Sets V8 flags from the command line.
 */
void V8::SetFlagsFromCommandLine(int* argc,
                                    char** argv,
                                    bool remove_flags)
{
    
}

/** Get the version string. */
const char* V8::GetVersion()
{
    return nullptr;
}

/**
 * Initializes V8. This function needs to be called before the first Isolate
 * is created. It always returns true.
 */
bool V8::Initialize()
{
    return false;
}

/**
 * Allows the host application to provide a callback which can be used
 * as a source of entropy for random number generators.
 */
void V8::SetEntropySource(EntropySource source)
{
    
}

/**
 * Allows the host application to provide a callback that allows v8 to
 * cooperate with a profiler that rewrites return addresses on stack.
 */
void V8::SetReturnAddressLocationResolver(
        ReturnAddressLocationResolver return_address_resolver)
{
    
}

/**
 * Releases any resources used by v8 and stops any utility threads
 * that may be running.  Note that disposing v8 is permanent, it
 * cannot be reinitialized.
 *
 * It should generally not be necessary to dispose v8 before exiting
 * a process, this should happen automatically.  It is only necessary
 * to use if the process needs the resources taken up by v8.
 */
bool V8::Dispose()
{
    return false;
}

/**
 * Initialize the ICU library bundled with V8. The embedder should only
 * invoke this method when using the bundled ICU. If V8 was compiled with
 * the ICU data in an external file and when the default location of that
 * file should be used, a path to the executable must be provided.
 * Returns true on success.
 *
 * The default is a file called icudtl.dat side-by-side with the executable.
 *
 * Optionally, the location of the data file can be provided to override the
 * default.
 */
bool V8::InitializeICUDefaultLocation(const char* exec_path,
                                      const char* icu_data_file)
{
    return false;
}

/**
 * Initialize the external startup data. The embedder only needs to
 * invoke this method when external startup data was enabled in a build.
 *
 * If V8 was compiled with the startup data in an external file, then
 * V8 needs to be given those external files during startup. There are
 * three ways to do this:
 * - InitializeExternalStartupData(const char*)
 *   This will look in the given directory for files "natives_blob.bin"
 *   and "snapshot_blob.bin" - which is what the default build calls them.
 * - InitializeExternalStartupData(const char*, const char*)
 *   As above, but will directly use the two given file names.
 * - Call SetNativesDataBlob, SetNativesDataBlob.
 *   This will read the blobs from the given data structures and will
 *   not perform any file IO.
 */
void V8::InitializeExternalStartupData(const char* directory_path)
{
    
}
void V8::InitializeExternalStartupData(const char* natives_blob,
                                          const char* snapshot_blob)
{
    
}
/**
 * Sets the v8::Platform to use. This should be invoked before V8 is
 * initialized.
 */
void V8::InitializePlatform(Platform* platform)
{
    
}

/**
 * Clears all references to the v8::Platform. This should be invoked after
 * V8 was disposed.
 */
void V8::ShutdownPlatform()
{
    
}

/**
 * Enable the default signal handler rather than using one provided by the
 * embedder.
 */
bool V8::RegisterDefaultSignalHandler()
{
    return false;
}

internal::Object** V8::GlobalizeReference(internal::Isolate* isolate,
                                             internal::Object** handle)
{
    return nullptr;
}
internal::Object** V8::CopyPersistent(internal::Object** handle)
{
    return nullptr;
}
void V8::DisposeGlobal(internal::Object** global_handle)
{
    
}
void V8::MakeWeak(internal::Object** location, void* data,
                     WeakCallbackInfo<void>::Callback weak_callback,
                     WeakCallbackType type)
{
    
}
void V8::MakeWeak(internal::Object** location, void* data,
                     // Must be 0 or -1.
                     int internal_field_index1,
                     // Must be 1 or -1.
                     int internal_field_index2,
                     WeakCallbackInfo<void>::Callback weak_callback)
{
    
}
void V8::MakeWeak(internal::Object*** location_addr)
{
    
}
void* V8::ClearWeak(internal::Object** location)
{
    return nullptr;
}
Value* V8::Eternalize(Isolate* isolate, Value* handle)
{
    return nullptr;
}

void V8::RegisterExternallyReferencedObject(internal::Object** object,
                                               internal::Isolate* isolate)
{
    
}

void V8::FromJustIsNothing()
{
    
}
void V8::ToLocalEmpty()
{
    
}
void V8::InternalFieldOutOfBounds(int index)
{
    
}

