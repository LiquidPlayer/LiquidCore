/*
 * Copyright (c) 2018-2019 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
#ifndef V82JSC_Extension_h
#define V82JSC_Extension_h

namespace V82JSC {
    
bool InstallAutoExtensions(v8::Local<v8::Context> context, std::map<std::string, bool>& loaded_extensions);
bool InstallExtension(v8::Local<v8::Context> context, const char *extension_name,
                      std::map<std::string, bool>& loaded_extensions);
}

#endif /* V82JSC_Extension_h */
