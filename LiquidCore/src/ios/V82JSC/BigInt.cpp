/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"
#include <iomanip>

using namespace V82JSC;
using namespace v8;

Local<BigInt> BigInt::New(Isolate* isolate, int64_t value)
{
    EscapableHandleScope scope(isolate);
    auto context = OperatingContext(isolate);
    auto ctx = ToContextRef(context);

    std::stringstream stream;
    stream << value;
    std::string strValue(stream.str());
    auto arg_local = v8::String::NewFromUtf8(isolate, strValue.c_str(), v8::String::NewStringType::kNormalString);
    auto arg = ToJSValueRef(arg_local, context);

    auto bigint = exec(ctx, "return BigInt(_1)", 1, &arg);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), bigint).As<BigInt>());
}

Local<BigInt> BigInt::NewFromUnsigned(Isolate* isolate, uint64_t value)
{
    EscapableHandleScope scope(isolate);
    auto context = OperatingContext(isolate);
    auto ctx = ToContextRef(context);

    std::stringstream stream;
    stream << value;
    std::string strValue(stream.str());
    auto arg_local = v8::String::NewFromUtf8(isolate, strValue.c_str(), v8::String::NewStringType::kNormalString);
    auto arg = ToJSValueRef(arg_local, context);

    auto bigint = exec(ctx, "return BigInt(_1)", 1, &arg);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), bigint).As<BigInt>());
}

/**
 * Creates a new BigInt object using a specified sign bit and a
 * specified list of digits/words.
 * The resulting number is calculated as:
 *
 * (-1)^sign_bit * (words[0] * (2^64)^0 + words[1] * (2^64)^1 + ...)
 */
MaybeLocal<BigInt> BigInt::NewFromWords(Local<v8::Context> context, int sign_bit,
                                int word_count, const uint64_t* words)
{
    auto isolate = ToIsolate(ToContextImpl(context)->GetIsolate());
    EscapableHandleScope scope(isolate);
    auto ctx = ToContextRef(context);

    std::stringstream stream;
    if (sign_bit) stream << "-";
    stream << "0x";
    for (int i=word_count; i>0; --i) {
        stream << std::setfill('0') << std::setw(16) << std::hex << words[i-1];
    }
    std::string strValue(stream.str());
    auto arg_local = v8::String::NewFromUtf8(isolate, strValue.c_str(), v8::String::NewStringType::kNormalString);
    auto arg = ToJSValueRef(arg_local, context);

    auto bigint = exec(ctx, "return BigInt(_1)", 1, &arg);
    return scope.Escape(V82JSC::Value::New(ToContextImpl(context), bigint).As<BigInt>());
}

/**
 * Returns the value of this BigInt as an unsigned 64-bit integer.
 * If `lossless` is provided, it will reflect whether the return value was
 * truncated or wrapped around. In particular, it is set to `false` if this
 * BigInt is negative.
 */
uint64_t BigInt::Uint64Value(bool* lossless) const
{
    auto context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto value = ToJSValueRef(this, context);

    auto int64 = exec(ctx, "let v = BigInt.asUIntN(64,_1); return BigInt.EQ === undefined ? [v, _1==v] : [v, BigInt.EQ(_1, v)]", 1, &value);
    auto int64_v = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)int64, 0, nullptr);
    auto int64_lossless = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)int64, 1, nullptr);
    if (lossless) {
        *lossless = JSValueToBoolean(ctx, int64_lossless);
    }
    return JSValueToNumber(ctx, int64_v, nullptr);
}

/**
 * Returns the value of this BigInt as a signed 64-bit integer.
 * If `lossless` is provided, it will reflect whether this BigInt was
 * truncated or not.
 */
int64_t BigInt::Int64Value(bool* lossless) const
{
    auto context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto value = ToJSValueRef(this, context);

    auto int64 = exec(ctx, "let v = BigInt.asIntN(64,_1); return BigInt.EQ === undefined ? [v, _1==v] : [v, BigInt.EQ(_1, v)]", 1, &value);
    auto int64_v = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)int64, 0, nullptr);
    auto int64_lossless = JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)int64, 1, nullptr);
    if (lossless) {
        *lossless = JSValueToBoolean(ctx, int64_lossless);
    }
    return JSValueToNumber(ctx, int64_v, nullptr);
}

/**
 * Returns the number of 64-bit words needed to store the result of
 * ToWordsArray().
 */
int BigInt::WordCount() const
{
    auto context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto value = ToJSValueRef(this, context);
    auto words = exec(ctx, "let s = _1.toString(16); let l = s.length - (s[0]==='-')?1:0; return (l/16) + 1", 1, &value);
    return JSValueToNumber(ctx, words, nullptr);
}

/**
 * Writes the contents of this BigInt to a specified memory location.
 * `sign_bit` must be provided and will be set to 1 if this BigInt is
 * negative.
 * `*word_count` has to be initialized to the length of the `words` array.
 * Upon return, it will be set to the actual number of words that would
 * be needed to store this BigInt (i.e. the return value of `WordCount()`).
 */
void BigInt::ToWordsArray(int* sign_bit, int* word_count, uint64_t* words) const
{
    auto context = ToCurrentContext(this);
    auto ctx = ToContextRef(context);
    auto value = ToJSValueRef(this, context);

    const auto code =
    "let s = _1.toString(16); "
    "let sign_bit = s[0] === '-'; "
    "if (sign_bit) s = s.substr(1); "
    "let padn = s.length % 16; "
    "for (let i=0; i < padn; i++) s = '0'.concat(s); "
    "let words = [];"
    "while (s.length > 0) {"
    "  let w = s.substr(0,16); s = s.substr(16);"
    "  words.push(parseInt(w,16));"
    "}"
    "return [sign_bit, words.length, words];";

    auto ret = exec(ctx, code, 1, &value);
    *sign_bit = JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)ret, 0, nullptr), nullptr);
    int wc = JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)ret, 1, nullptr), nullptr);
    auto arr = JSValueToObject(ctx, JSObjectGetPropertyAtIndex(ctx, (JSObjectRef)ret, 2, nullptr), nullptr);
    for (int i=0; i<wc && i<*word_count; i++) {
        words[i] = JSValueToNumber(ctx, JSObjectGetPropertyAtIndex(ctx, arr, i, nullptr), nullptr);
    }
    *word_count = wc;
}

