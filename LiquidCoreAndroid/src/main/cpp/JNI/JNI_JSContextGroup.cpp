/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */

#include "JNI/JNI.h"
#include "JSC/JSC.h"

NATIVE(JNIJSContextGroup,jlong,create) (PARAMS)
{
    // Maintain compatibility at the ContextGroup level with JSC by using JSContextGroupCreate()
    return SharedWrap<ContextGroup>::New(
            const_cast<OpaqueJSContextGroup*>(JSContextGroupCreate())->ContextGroup::shared_from_this()
    );
}

NATIVE(JNIJSContextGroup,jboolean,isManaged) (PARAMS, jlong grpRef)
{
    auto group = SharedWrap<ContextGroup>::Shared(grpRef);

    return (jboolean) (group && group->Loop());
}

NATIVE(JNIJSContextGroup,void,runInContextGroup) (PARAMS, jlong grpRef, jobject thisObj, jobject runnable) {
    auto group = SharedWrap<ContextGroup>::Shared(grpRef);

    if (group && group->Loop() && std::this_thread::get_id() != group->Thread()) {
        group->schedule_java_runnable(env, thisObj, runnable);
    } else {
        jclass cls = env->GetObjectClass(thisObj);
        jmethodID mid;
        do {
            mid = env->GetMethodID(cls,"inContextCallback","(Ljava/lang/Runnable;)V");
            if (!env->ExceptionCheck()) break;
            env->ExceptionClear();
            jclass super = env->GetSuperclass(cls);
            env->DeleteLocalRef(cls);
            if (super == NULL || env->ExceptionCheck()) {
                if (super != NULL) env->DeleteLocalRef(super);
                __android_log_assert("FAIL", "runInContextGroup",
                                     "Internal error.  Can't call back.");
            }
            cls = super;
        } while (true);
        env->DeleteLocalRef(cls);

        env->CallVoidMethod(thisObj, mid, runnable);
    }
}

/*
 * Error codes:
 * 0  = snapshot successfully taken and file written
 * -1 = snashot failed
 * -2 = snapshot taken, but could not open file for writing
 * -3 = snapshot taken, but could not write to file
 * -4 = snapshot taken, but could not close file properly
 */
NATIVE(JNIJSContextGroup,int,createSnapshot) (PARAMS, jstring script_, jstring outFile_)
{
    const char *_script = env->GetStringUTFChars(script_, NULL);
    const char *_outFile = env->GetStringUTFChars(outFile_, NULL);

    int rval = 0;

    ContextGroup::init_v8();
    v8::StartupData data = v8::V8::CreateSnapshotDataBlob(_script);
    ContextGroup::dispose_v8();

    if (data.data == nullptr) {
        rval = -1;
    } else {
        FILE *fp = fopen(_outFile, "wb");
        if (fp == nullptr) {
            rval = -2;
        } else {
            size_t written = fwrite(data.data, sizeof (char), (size_t) data.raw_size, fp);
            rval = (written == (size_t) data.raw_size) ? 0 : -3;
            int c = fclose(fp);
            if (!rval && c) rval = -4;
        }
        delete[] data.data;
    }

    env->ReleaseStringUTFChars(script_, _script);
    env->ReleaseStringUTFChars(outFile_, _outFile);

    return rval;
}

NATIVE(JNIJSContextGroup,jlong,createWithSnapshotFile) (PARAMS, jstring inFile_)
{
    const char *_inFile = env->GetStringUTFChars(inFile_, NULL);

    boost::shared_ptr<ContextGroup> group = ContextGroup::New(_inFile);

    env->ReleaseStringUTFChars(inFile_, _inFile);

    // Maintain compatibility at the ContextGroup level with JSC by using JSContextGroupCreate()
    return SharedWrap<ContextGroup>::New(group);
}

NATIVE(JNIJSContextGroup,void,Finalize) (PARAMS, jlong reference)
{
    SharedWrap<ContextGroup>::Dispose(reference);
}

