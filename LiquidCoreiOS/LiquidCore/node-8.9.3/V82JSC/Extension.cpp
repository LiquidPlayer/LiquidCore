/*
 * Copyright (c) 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#include "V82JSC.h"

using namespace v8;

auto s_extensions = std::map<const char*, v8::Extension *>();

Extension::Extension(const char* name,
          const char* source,
          int dep_count,
          const char** deps,
          int source_length) :
    name_(name),
    source_length_(!source ? 0 : source_length<0 ? strlen(source) : source_length),
    source_(!source ? "" : source, !source ? 0 : strlen(source)),
    dep_count_(dep_count),
    deps_(deps),
    auto_enable_(false)
{
}

void V8_EXPORT v8::RegisterExtension(v8::Extension* extension)
{
    s_extensions[extension->name()] = extension;
}

bool InstallExtension(Local<Context> context, const char *extension_name, std::map<std::string, bool>& loaded_extensions)
{
    if (s_extensions.count(extension_name) == 0) return false;
    
    Extension& extension = *s_extensions[extension_name];
    
    ContextImpl *ctximpl = V82JSC::ToContextImpl(context);
    IsolateImpl* iso = V82JSC::ToIsolateImpl(ctximpl);
    if (loaded_extensions.count(extension.name())) return true;
    
    loaded_extensions[extension.name()] = true;
    
    for (int i=0; i<extension.dependency_count(); i++) {
        InstallExtension(context, extension.dependencies()[i], loaded_extensions);
    }
    
    // Lex the code looking for "native function <<function_name>();"
    std::string input(extension.source()->data());
    std::string output;

    char last = 0;
    bool in_line_comment = false,
        in_block_comment = false,
        pattern_matched = false,
        in_name = false;
#define in_comment (in_line_comment || in_block_comment)
#define is_space(c) (c == ' ' || c == '\t')
    
    std::string name;
    std::string maybe;
    std::vector<std::string> native_function_names;
    
    const char *patterns[] = {"native function ", "();"};
    int index = 0;
    int pattern = 0;
    
    int c = 0;
    for (auto i = input.begin(); i != input.end() && c < extension.source_length(); ++i, ++c) {
        switch (*i) {
            case '*':
                if (last == '/' && !in_comment) in_block_comment = true;
                break;
            default:
                if (*i == '\n') in_line_comment = false;
                /* no break -- fallthru */
                if (!in_comment) {
                    if (last == '/') in_line_comment = true;
                } else if (in_block_comment && last == '*') {
                    in_block_comment = false;
                    break;
                }
                if (in_comment) break;
                if (!pattern_matched && !in_name) {
                    if (is_space(*i) && pattern==2) break; // in the second pattern, we ignore whitespace
                    if ((is_space(*i) && patterns[pattern][index] == ' ') || patterns[pattern][index] == *i) {
                        // Make sure we are not in the middle of a word (ex. 'foonative function ' should _not_ match)
                        if (pattern == 0 && index == 0) {
                            if (last == '_' || isalpha(last) || isnumber(last)) break;
                        }
                        index++;
                        if (index == strlen(patterns[pattern])) {
                            pattern_matched = true;
                            pattern++;
                            if (pattern > 1) {
                                // We have a complete native function declaration
                                native_function_names.push_back(std::string(name));
                                name.clear();
                                pattern_matched = false;
                                pattern = 0;
                                index = 0;
                                // Throw out the declaration from source
                                maybe.clear();
                                // And ignore this last character
                                continue;
                            }
                            index = 0;
                            name.clear();
                        }
                    } else if (is_space(*i) && index > 0 && patterns[pattern][index-1] == ' ') {
                        // do nothing to the index; duplicate whitespace
                    } else {
                        // pattern doesn't match.  Start all over again
                        pattern_matched = false;
                        pattern = 0;
                        in_name = false;
                        name.clear();
                        index = 0;
                    }
                } else if (pattern_matched && !in_name) {
                    if (is_space(*i)) break;
                    if (isalpha(*i) || isnumber(*i) || *i=='_') {
                        in_name = true;
                        name += (*i);
                    }
                } else if (pattern_matched && in_name) {
                    if (isalpha(*i) || isnumber(*i) || *i=='_') {
                        name += (*i);
                    } else {
                        in_name = false;
                        pattern_matched = false;
                        if (*i == patterns[pattern][0] /* must be non-whitespace!*/) {
                            index=1;
                        } else {
                            index=0;
                            pattern=0;
                        }
                    }
                }
                break;
        }
        last = *i;
        if (index > 0 || in_name || pattern_matched) {
            maybe += *i;
        } else {
            output += maybe;
            output += *i;
            maybe.clear();
        }
    }
    
    Isolate *isolate = V82JSC::ToIsolate(iso);
    {
        HandleScope handle_scope(isolate);
        Context::Scope context_scope(context);
        for (auto i=native_function_names.begin(); i!=native_function_names.end(); ++i) {
            Local<String> name = String::NewFromUtf8(isolate, i->c_str(), NewStringType::kNormal).ToLocalChecked();
            Local<FunctionTemplate> native_function_template = extension.GetNativeFunctionTemplate(isolate, name);
            if (native_function_template.IsEmpty()) return false;
            Local<Function> native_function = native_function_template->GetFunction(context).ToLocalChecked();
            if (native_function.IsEmpty()) return false;
            Maybe<bool> set = context->Global()->Set(context, name, native_function);
            if (!set.IsJust()) return false;
        }
        Local<String> source = String::NewFromUtf8(isolate, output.c_str(), NewStringType::kNormal).ToLocalChecked();
        if (source.IsEmpty()) return false;
        if (source->Length() > 0) {
            Local<Script> script = Script::Compile(context, source).ToLocalChecked();
            if (script.IsEmpty()) return false;
            MaybeLocal<Value> result = script->Run(context);
            if(result.IsEmpty()) return false;
        }
    }
    return true;
}

bool InstallAutoExtensions(v8::Local<v8::Context> context, std::map<std::string, bool>& loaded_extensions)
{
    for (auto i=s_extensions.begin(); i!=s_extensions.end(); ++i) {
        if (i->second->auto_enable()) {
            if (!InstallExtension(context, i->first, loaded_extensions)) return false;
        }
    }
    return true;
}
