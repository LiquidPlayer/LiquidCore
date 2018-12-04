/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include <jni.h>
#include <JavaScriptCore/JavaScript.h>

typedef struct _JSCShim {
    /* JSBase.h */
    JSValueRef (*JSEvaluateScript)    (JSContextRef, JSStringRef, JSObjectRef, JSStringRef, int, JSValueRef*);
    bool       (*JSCheckScriptSyntax) (JSContextRef, JSStringRef, JSStringRef, int, JSValueRef*);
    void       (*JSGarbageCollect)    (JSContextRef);

    /* JSContextRef.h */
    JSContextGroupRef  (*JSContextGroupCreate)  ();
    JSContextGroupRef  (*JSContextGroupRetain)         (JSContextGroupRef);
    void               (*JSContextGroupRelease)        (JSContextGroupRef);
    JSGlobalContextRef (*JSGlobalContextCreate)        (JSClassRef);
    JSGlobalContextRef (*JSGlobalContextCreateInGroup) (JSContextGroupRef, JSClassRef);
    JSGlobalContextRef (*JSGlobalContextRetain)        (JSGlobalContextRef);
    void               (*JSGlobalContextRelease)       (JSGlobalContextRef);
    JSObjectRef        (*JSContextGetGlobalObject)     (JSContextRef);
    JSContextGroupRef  (*JSContextGetGroup)            (JSContextRef);
    JSGlobalContextRef (*JSContextGetGlobalContext)    (JSContextRef);
    JSStringRef        (*JSGlobalContextCopyName)      (JSGlobalContextRef);
    void               (*JSGlobalContextSetName)       (JSGlobalContextRef, JSStringRef);

    /* JSObjectRef */
    JSClassRef             (*JSClassCreate)                    (const JSClassDefinition*);
    JSClassRef             (*JSClassRetain)                    (JSClassRef);
    void                   (*JSClassRelease)                   (JSClassRef);
    JSObjectRef            (*JSObjectMake)                     (JSContextRef, JSClassRef, void*);
    JSObjectRef            (*JSObjectMakeFunctionWithCallback) (JSContextRef, JSStringRef, JSObjectCallAsFunctionCallback);
    JSObjectRef            (*JSObjectMakeConstructor)          (JSContextRef, JSClassRef, JSObjectCallAsConstructorCallback);
    JSObjectRef            (*JSObjectMakeArray)                (JSContextRef, size_t argumentCount, const JSValueRef[], JSValueRef*);
    JSObjectRef            (*JSObjectMakeDate)                 (JSContextRef, size_t argumentCount, const JSValueRef[], JSValueRef*);
    JSObjectRef            (*JSObjectMakeError)                (JSContextRef, size_t argumentCount, const JSValueRef[], JSValueRef*);
    JSObjectRef            (*JSObjectMakeRegExp)               (JSContextRef, size_t argumentCount, const JSValueRef[], JSValueRef*);
    JSObjectRef            (*JSObjectMakeFunction)             (JSContextRef, JSStringRef, unsigned, const JSStringRef[], JSStringRef, JSStringRef, int, JSValueRef*);
    JSValueRef             (*JSObjectGetPrototype)             (JSContextRef, JSObjectRef);
    void                   (*JSObjectSetPrototype)             (JSContextRef, JSObjectRef, JSValueRef);
    bool                   (*JSObjectHasProperty)              (JSContextRef, JSObjectRef, JSStringRef);
    JSValueRef             (*JSObjectGetProperty)              (JSContextRef, JSObjectRef, JSStringRef, JSValueRef*);
    void                   (*JSObjectSetProperty)              (JSContextRef, JSObjectRef, JSStringRef, JSValueRef, JSPropertyAttributes, JSValueRef*);
    bool                   (*JSObjectDeleteProperty)           (JSContextRef, JSObjectRef, JSStringRef, JSValueRef*);
    JSValueRef             (*JSObjectGetPropertyAtIndex)       (JSContextRef, JSObjectRef, unsigned, JSValueRef*);
    void                   (*JSObjectSetPropertyAtIndex)       (JSContextRef, JSObjectRef, unsigned, JSValueRef, JSValueRef*);
    void*                  (*JSObjectGetPrivate)               (JSObjectRef);
    bool                   (*JSObjectSetPrivate)               (JSObjectRef, void*);
    bool                   (*JSObjectIsFunction)               (JSContextRef, JSObjectRef);
    JSValueRef             (*JSObjectCallAsFunction)           (JSContextRef, JSObjectRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*);
    bool                   (*JSObjectIsConstructor)            (JSContextRef, JSObjectRef);
    JSObjectRef            (*JSObjectCallAsConstructor)        (JSContextRef, JSObjectRef, size_t, const JSValueRef[], JSValueRef*);
    JSPropertyNameArrayRef (*JSObjectCopyPropertyNames)        (JSContextRef, JSObjectRef);
    JSPropertyNameArrayRef (*JSPropertyNameArrayRetain)        (JSPropertyNameArrayRef);
    void                   (*JSPropertyNameArrayRelease)       (JSPropertyNameArrayRef);
    size_t                 (*JSPropertyNameArrayGetCount)      (JSPropertyNameArrayRef);
    JSStringRef            (*JSPropertyNameArrayGetNameAtIndex)(JSPropertyNameArrayRef, size_t);
    void                   (*JSPropertyNameAccumulatorAddName) (JSPropertyNameAccumulatorRef, JSStringRef);

    /* JSStringRef.h */
    JSStringRef   (*JSStringCreateWithCharacters)     (const JSChar*, size_t);
    JSStringRef   (*JSStringCreateWithUTF8CString)    (const char*);
    JSStringRef   (*JSStringRetain)                   (JSStringRef);
    void          (*JSStringRelease)                  (JSStringRef);
    size_t        (*JSStringGetLength)                (JSStringRef);
    const JSChar* (*JSStringGetCharactersPtr)         (JSStringRef);
    size_t        (*JSStringGetMaximumUTF8CStringSize)(JSStringRef);
    size_t        (*JSStringGetUTF8CString)           (JSStringRef, char*, size_t);
    bool          (*JSStringIsEqual)                  (JSStringRef, JSStringRef);
    bool          (*JSStringIsEqualToUTF8CString)     (JSStringRef, const char*);

    /* JSValueRef.h */
    JSType      (*JSValueGetType)                (JSContextRef, JSValueRef);
    bool        (*JSValueIsUndefined)            (JSContextRef, JSValueRef);
    bool        (*JSValueIsNull)                 (JSContextRef, JSValueRef);
    bool        (*JSValueIsBoolean)              (JSContextRef, JSValueRef);
    bool        (*JSValueIsNumber)               (JSContextRef, JSValueRef);
    bool        (*JSValueIsString)               (JSContextRef, JSValueRef);
    bool        (*JSValueIsObject)               (JSContextRef, JSValueRef);
    bool        (*JSValueIsObjectOfClass)        (JSContextRef, JSValueRef, JSClassRef);
    bool        (*JSValueIsEqual)                (JSContextRef, JSValueRef, JSValueRef, JSValueRef*);
    bool        (*JSValueIsStrictEqual)          (JSContextRef, JSValueRef, JSValueRef);
    bool        (*JSValueIsInstanceOfConstructor)(JSContextRef, JSValueRef, JSObjectRef, JSValueRef*);
    JSValueRef  (*JSValueMakeUndefined)          (JSContextRef);
    JSValueRef  (*JSValueMakeNull)               (JSContextRef);
    JSValueRef  (*JSValueMakeBoolean)            (JSContextRef, bool);
    JSValueRef  (*JSValueMakeNumber)             (JSContextRef, double);
    JSValueRef  (*JSValueMakeString)             (JSContextRef, JSStringRef);
    JSValueRef  (*JSValueMakeFromJSONString)     (JSContextRef, JSStringRef);
    JSStringRef (*JSValueCreateJSONString)       (JSContextRef, JSValueRef, unsigned, JSValueRef*);
    bool        (*JSValueToBoolean)              (JSContextRef, JSValueRef);
    double      (*JSValueToNumber)               (JSContextRef, JSValueRef, JSValueRef*);
    JSStringRef (*JSValueToStringCopy)           (JSContextRef, JSValueRef, JSValueRef*);
    JSObjectRef (*JSValueToObject)               (JSContextRef, JSValueRef, JSValueRef*);
    void        (*JSValueProtect)                (JSContextRef, JSValueRef);
    void        (*JSValueUnprotect)              (JSContextRef, JSValueRef);
} JSCShim;

#if defined(JSCSHIM_IMPL)
static JSCShim function_map = {
    /* JSBase.h */
    JSEvaluateScript,
    JSCheckScriptSyntax,
    JSGarbageCollect,

    /* JSContextRef.h */
    JSContextGroupCreate,
    JSContextGroupRetain,
    JSContextGroupRelease,
    JSGlobalContextCreate,
    JSGlobalContextCreateInGroup,
    JSGlobalContextRetain,
    JSGlobalContextRelease,
    JSContextGetGlobalObject,
    JSContextGetGroup,
    JSContextGetGlobalContext,
    JSGlobalContextCopyName,
    JSGlobalContextSetName,

    /* JSObjectRef */
    JSClassCreate,
    JSClassRetain,
    JSClassRelease,
    JSObjectMake,
    JSObjectMakeFunctionWithCallback,
    JSObjectMakeConstructor,
    JSObjectMakeArray,
    JSObjectMakeDate,
    JSObjectMakeError,
    JSObjectMakeRegExp,
    JSObjectMakeFunction,
    JSObjectGetPrototype,
    JSObjectSetPrototype,
    JSObjectHasProperty,
    JSObjectGetProperty,
    JSObjectSetProperty,
    JSObjectDeleteProperty,
    JSObjectGetPropertyAtIndex,
    JSObjectSetPropertyAtIndex,
    JSObjectGetPrivate,
    JSObjectSetPrivate,
    JSObjectIsFunction,
    JSObjectCallAsFunction,
    JSObjectIsConstructor,
    JSObjectCallAsConstructor,
    JSObjectCopyPropertyNames,
    JSPropertyNameArrayRetain,
    JSPropertyNameArrayRelease,
    JSPropertyNameArrayGetCount,
    JSPropertyNameArrayGetNameAtIndex,
    JSPropertyNameAccumulatorAddName,

    /* JSStringRef.h */
    JSStringCreateWithCharacters,
    JSStringCreateWithUTF8CString,
    JSStringRetain,
    JSStringRelease,
    JSStringGetLength,
    JSStringGetCharactersPtr,
    JSStringGetMaximumUTF8CStringSize,
    JSStringGetUTF8CString,
    JSStringIsEqual,
    JSStringIsEqualToUTF8CString,

    /* JSValueRef.h */
    JSValueGetType,
    JSValueIsUndefined,
    JSValueIsNull,
    JSValueIsBoolean,
    JSValueIsNumber,
    JSValueIsString,
    JSValueIsObject,
    JSValueIsObjectOfClass,
    JSValueIsEqual,
    JSValueIsStrictEqual,
    JSValueIsInstanceOfConstructor,
    JSValueMakeUndefined,
    JSValueMakeNull,
    JSValueMakeBoolean,
    JSValueMakeNumber,
    JSValueMakeString,
    JSValueMakeFromJSONString,
    JSValueCreateJSONString,
    JSValueToBoolean,
    JSValueToNumber,
    JSValueToStringCopy,
    JSValueToObject,
    JSValueProtect,
    JSValueUnprotect
};

extern "C" JNIEXPORT jlong JNICALL Java_org_liquidplayer_jscshim_JSCShim_getJSCShimToken(
    JNIEnv* env, jobject thiz)
{
    return (long) &function_map;
}

#else
static JSCShim * s;

extern "C" JNIEXPORT void JNICALL Java_org_liquidplayer_jscshim_JSCShim_setJSCShimToken(
    JNIEnv* env, jobject thiz, jlong token)
{
    s = (JSCShim *)token;
}

JS_EXPORT JSValueRef JSEvaluateScript(JSContextRef ctx, JSStringRef script,
    JSObjectRef thisObject, JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception) {

    return s->JSEvaluateScript(ctx, script, thisObject, sourceURL, startingLineNumber, exception);
}
JS_EXPORT bool JSCheckScriptSyntax(JSContextRef ctx, JSStringRef script,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception) {

    return s->JSCheckScriptSyntax(ctx, script, sourceURL, startingLineNumber, exception);
}
JS_EXPORT void JSGarbageCollect(JSContextRef ctx) {
    return s->JSGarbageCollect(ctx);
}

JS_EXPORT JSContextGroupRef JSContextGroupCreate() {
    return s->JSContextGroupCreate();
}
JS_EXPORT JSContextGroupRef JSContextGroupRetain(JSContextGroupRef group) {
    return s->JSContextGroupRetain(group);
}
JS_EXPORT void JSContextGroupRelease(JSContextGroupRef group) {
    return s->JSContextGroupRelease(group);
}
JS_EXPORT JSGlobalContextRef JSGlobalContextCreate(JSClassRef globalObjectClass) {
    return s->JSGlobalContextCreate(globalObjectClass);
}
JS_EXPORT JSGlobalContextRef JSGlobalContextCreateInGroup(JSContextGroupRef group,
    JSClassRef globalObjectClass) {
    return s->JSGlobalContextCreateInGroup(group, globalObjectClass);
}
JS_EXPORT JSGlobalContextRef JSGlobalContextRetain(JSGlobalContextRef ctx) {
    return s->JSGlobalContextRetain(ctx);
}
JS_EXPORT void JSGlobalContextRelease(JSGlobalContextRef ctx) {
    return s->JSGlobalContextRelease(ctx);
}
JS_EXPORT JSObjectRef JSContextGetGlobalObject(JSContextRef ctx) {
    return s->JSContextGetGlobalObject(ctx);
}
JS_EXPORT JSContextGroupRef JSContextGetGroup(JSContextRef ctx) {
    return s->JSContextGetGroup(ctx);
}
JS_EXPORT JSGlobalContextRef JSContextGetGlobalContext(JSContextRef ctx) {
    return s->JSContextGetGlobalContext(ctx);
}
JS_EXPORT JSStringRef JSGlobalContextCopyName(JSGlobalContextRef ctx) {
    return s->JSGlobalContextCopyName(ctx);
}
JS_EXPORT void JSGlobalContextSetName(JSGlobalContextRef ctx, JSStringRef name) {
    return s->JSGlobalContextSetName(ctx, name);
}

JS_EXPORT JSClassRef JSClassCreate(const JSClassDefinition* definition){
    return s->JSClassCreate(definition);
}
JS_EXPORT JSClassRef JSClassRetain(JSClassRef jsClass) {
    return s->JSClassRetain(jsClass);
}
JS_EXPORT void JSClassRelease(JSClassRef jsClass) {
    s->JSClassRelease(jsClass);
}
JS_EXPORT JSObjectRef JSObjectMake(JSContextRef ctx, JSClassRef jsClass, void* data) {
    return s->JSObjectMake(ctx, jsClass, data);
}
JS_EXPORT JSObjectRef JSObjectMakeFunctionWithCallback(JSContextRef ctx, JSStringRef name,
    JSObjectCallAsFunctionCallback callAsFunction) {
    return s->JSObjectMakeFunctionWithCallback(ctx, name, callAsFunction);
}
JS_EXPORT JSObjectRef JSObjectMakeConstructor(JSContextRef ctx, JSClassRef jsClass,
    JSObjectCallAsConstructorCallback callAsConstructor) {
    return s->JSObjectMakeConstructor(ctx, jsClass, callAsConstructor);
}
JS_EXPORT JSObjectRef JSObjectMakeArray(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception) {
    return s->JSObjectMakeArray(ctx, argumentCount, arguments, exception);
}
JS_EXPORT JSObjectRef JSObjectMakeDate(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception) {
    return s->JSObjectMakeDate(ctx, argumentCount, arguments, exception);
}
JS_EXPORT JSObjectRef JSObjectMakeError(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception) {
    return s->JSObjectMakeError(ctx, argumentCount, arguments, exception);
}
JS_EXPORT JSObjectRef JSObjectMakeRegExp(JSContextRef ctx, size_t argumentCount,
    const JSValueRef arguments[], JSValueRef* exception) {
    return s->JSObjectMakeRegExp(ctx, argumentCount, arguments, exception);
}
JS_EXPORT JSObjectRef JSObjectMakeFunction(JSContextRef ctx, JSStringRef name,
    unsigned parameterCount, const JSStringRef parameterNames[], JSStringRef body,
    JSStringRef sourceURL, int startingLineNumber, JSValueRef* exception) {
    return s->JSObjectMakeFunction(ctx, name, parameterCount, parameterNames, body, sourceURL,
        startingLineNumber, exception);
}
JS_EXPORT JSValueRef JSObjectGetPrototype(JSContextRef ctx, JSObjectRef object) {
    return s->JSObjectGetPrototype(ctx, object);
}
JS_EXPORT void JSObjectSetPrototype(JSContextRef ctx, JSObjectRef object, JSValueRef value) {
    return s->JSObjectSetPrototype(ctx, object, value);
}
JS_EXPORT bool JSObjectHasProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName) {
    return s->JSObjectHasProperty(ctx, object, propertyName);
}
JS_EXPORT JSValueRef JSObjectGetProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exception) {
    return s->JSObjectGetProperty(ctx, object, propertyName, exception);
}
JS_EXPORT void JSObjectSetProperty(JSContextRef ctx, JSObjectRef object, JSStringRef propertyName,
    JSValueRef value, JSPropertyAttributes attributes, JSValueRef* exception) {
    s->JSObjectSetProperty(ctx, object, propertyName, value, attributes, exception);
}
JS_EXPORT bool JSObjectDeleteProperty(JSContextRef ctx, JSObjectRef object,
    JSStringRef propertyName, JSValueRef* exception) {
    return s->JSObjectDeleteProperty(ctx, object, propertyName, exception);
}
JS_EXPORT JSValueRef JSObjectGetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef* exception) {
    return s->JSObjectGetPropertyAtIndex(ctx, object, propertyIndex, exception);
}
JS_EXPORT void JSObjectSetPropertyAtIndex(JSContextRef ctx, JSObjectRef object,
    unsigned propertyIndex, JSValueRef value, JSValueRef* exception) {
    return s->JSObjectSetPropertyAtIndex(ctx, object, propertyIndex, value, exception);
}
JS_EXPORT void* JSObjectGetPrivate(JSObjectRef object) {
    return s->JSObjectGetPrivate(object);
}
JS_EXPORT bool JSObjectSetPrivate(JSObjectRef object, void* data) {
    return s->JSObjectSetPrivate(object, data);
}
JS_EXPORT bool JSObjectIsFunction(JSContextRef ctx, JSObjectRef object) {
    return s->JSObjectIsFunction(ctx, object);
}
JS_EXPORT JSValueRef JSObjectCallAsFunction(JSContextRef ctx, JSObjectRef object,
    JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[],
    JSValueRef* exception) {
    return s->JSObjectCallAsFunction(ctx, object, thisObject, argumentCount, arguments, exception);
}
JS_EXPORT bool JSObjectIsConstructor(JSContextRef ctx, JSObjectRef object) {
    return s->JSObjectIsConstructor(ctx, object);
}
JS_EXPORT JSObjectRef JSObjectCallAsConstructor(JSContextRef ctx, JSObjectRef object,
    size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
    return s->JSObjectCallAsConstructor(ctx, object, argumentCount, arguments, exception);
}
JS_EXPORT JSPropertyNameArrayRef JSObjectCopyPropertyNames(JSContextRef ctx, JSObjectRef object) {
    return s->JSObjectCopyPropertyNames(ctx, object);
}
JS_EXPORT JSPropertyNameArrayRef JSPropertyNameArrayRetain(JSPropertyNameArrayRef array) {
    return s->JSPropertyNameArrayRetain(array);
}
JS_EXPORT void JSPropertyNameArrayRelease(JSPropertyNameArrayRef array) {
    return s->JSPropertyNameArrayRelease(array);
}
JS_EXPORT size_t JSPropertyNameArrayGetCount(JSPropertyNameArrayRef array) {
    return s->JSPropertyNameArrayGetCount(array);
}
JS_EXPORT JSStringRef JSPropertyNameArrayGetNameAtIndex(JSPropertyNameArrayRef array, size_t index){
    return s->JSPropertyNameArrayGetNameAtIndex(array, index);
}
JS_EXPORT void JSPropertyNameAccumulatorAddName(JSPropertyNameAccumulatorRef accumulator,
    JSStringRef propertyName) {
    return s->JSPropertyNameAccumulatorAddName(accumulator, propertyName);
}

JS_EXPORT JSStringRef JSStringCreateWithCharacters(const JSChar* chars, size_t numChars) {
    return s->JSStringCreateWithCharacters(chars, numChars);
}
JS_EXPORT JSStringRef JSStringCreateWithUTF8CString(const char* string) {
    return s->JSStringCreateWithUTF8CString(string);
}
JS_EXPORT JSStringRef JSStringRetain(JSStringRef string) {
    return s->JSStringRetain(string);
}
JS_EXPORT void JSStringRelease(JSStringRef string) {
    s->JSStringRelease(string);
}
JS_EXPORT size_t JSStringGetLength(JSStringRef string) {
    return s->JSStringGetLength(string);
}
JS_EXPORT const JSChar* JSStringGetCharactersPtr(JSStringRef string) {
    return s->JSStringGetCharactersPtr(string);
}
JS_EXPORT size_t JSStringGetMaximumUTF8CStringSize(JSStringRef string) {
    return s->JSStringGetMaximumUTF8CStringSize(string);
}
JS_EXPORT size_t JSStringGetUTF8CString(JSStringRef string, char* buffer, size_t bufferSize) {
    return s->JSStringGetUTF8CString(string, buffer, bufferSize);
}
JS_EXPORT bool JSStringIsEqual(JSStringRef a, JSStringRef b) {
    return s->JSStringIsEqual(a, b);
}
JS_EXPORT bool JSStringIsEqualToUTF8CString(JSStringRef a, const char* b) {
    return s->JSStringIsEqualToUTF8CString(a, b);
}

JS_EXPORT JSType JSValueGetType(JSContextRef ctx, JSValueRef value) {
    return s->JSValueGetType(ctx, value);
}
JS_EXPORT bool JSValueIsUndefined(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsUndefined(ctx, value);
}
JS_EXPORT bool JSValueIsNull(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsNull(ctx, value);
}
JS_EXPORT bool JSValueIsBoolean(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsBoolean(ctx, value);
}
JS_EXPORT bool JSValueIsNumber(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsNumber(ctx, value);
}
JS_EXPORT bool JSValueIsString(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsString(ctx, value);
}
JS_EXPORT bool JSValueIsObject(JSContextRef ctx, JSValueRef value) {
    return s->JSValueIsObject(ctx, value);
}
JS_EXPORT bool JSValueIsObjectOfClass(JSContextRef ctx, JSValueRef value, JSClassRef jsClass) {
    return s->JSValueIsObjectOfClass(ctx, value, jsClass);
}
JS_EXPORT bool JSValueIsEqual(JSContextRef ctx, JSValueRef a, JSValueRef b, JSValueRef* exception) {
    return s->JSValueIsEqual(ctx, a, b, exception);
}
JS_EXPORT bool JSValueIsStrictEqual(JSContextRef ctx, JSValueRef a, JSValueRef b) {
    return s->JSValueIsStrictEqual(ctx, a, b);
}
JS_EXPORT bool JSValueIsInstanceOfConstructor(JSContextRef ctx, JSValueRef value,
    JSObjectRef constructor, JSValueRef* exception) {
    return s->JSValueIsInstanceOfConstructor(ctx, value, constructor, exception);
}
JS_EXPORT JSValueRef JSValueMakeUndefined(JSContextRef ctx) {
    return s->JSValueMakeUndefined(ctx);
}
JS_EXPORT JSValueRef JSValueMakeNull(JSContextRef ctx) {
    return s->JSValueMakeNull(ctx);
}
JS_EXPORT JSValueRef JSValueMakeBoolean(JSContextRef ctx, bool boolean) {
    return s->JSValueMakeBoolean(ctx, boolean);
}
JS_EXPORT JSValueRef JSValueMakeNumber(JSContextRef ctx, double number) {
    return s->JSValueMakeNumber(ctx, number);
}
JS_EXPORT JSValueRef JSValueMakeString(JSContextRef ctx, JSStringRef string) {
    return s->JSValueMakeString(ctx, string);
}
JS_EXPORT JSValueRef JSValueMakeFromJSONString(JSContextRef ctx, JSStringRef string) {
    return s->JSValueMakeFromJSONString(ctx, string);
}
JS_EXPORT JSStringRef JSValueCreateJSONString(JSContextRef ctx, JSValueRef value, unsigned indent,
    JSValueRef* exception) {
    return s->JSValueCreateJSONString(ctx, value, indent, exception);
}
JS_EXPORT bool JSValueToBoolean(JSContextRef ctx, JSValueRef value) {
    return s->JSValueToBoolean(ctx, value);
}
JS_EXPORT double JSValueToNumber(JSContextRef ctx, JSValueRef value, JSValueRef* exception) {
    return s->JSValueToNumber(ctx, value, exception);
}
JS_EXPORT JSStringRef JSValueToStringCopy(JSContextRef ctx, JSValueRef value,
    JSValueRef* exception) {
    return s->JSValueToStringCopy(ctx, value, exception);
}
JS_EXPORT JSObjectRef JSValueToObject(JSContextRef ctx, JSValueRef value, JSValueRef* exception) {
    return s->JSValueToObject(ctx, value, exception);
}
JS_EXPORT void JSValueProtect(JSContextRef ctx, JSValueRef value) {
    s->JSValueProtect(ctx, value);
}
JS_EXPORT void JSValueUnprotect(JSContextRef ctx, JSValueRef value) {
    s->JSValueUnprotect(ctx, value);
}

const JSClassDefinition kJSClassDefinitionEmpty = {
    0, 0,
    NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL
};

#endif
