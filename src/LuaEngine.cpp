#include "LuaEngine.h"
#include <windows.h>
#include "Core/Stealth.hpp"
#include "Core/Crypto.hpp"

// ============================================================
// Lua API Definitions
// ============================================================
#define LUA_OK 0
#define LUA_MULTRET (-1)

typedef int(__cdecl* fn_luaL_loadbufferx)(lua_State* L, const char* buff, size_t sz, const char* name, const char* mode);
typedef int(__cdecl* fn_lua_pcallk)(lua_State* L, int nargs, int nresults, int errfunc, void* ctx, void* k);
typedef void(__cdecl* fn_lua_settop)(lua_State* L, int idx);
typedef const char*(__cdecl* fn_lua_tolstring)(lua_State* L, int idx, size_t* len);

// We hooked loadbufferx in Hooks.cpp, so we can access the original function via _pOrig
extern fn_luaL_loadbufferx _pOrig;

namespace LuaAPI {
    fn_lua_pcallk pcallk = nullptr;
    fn_lua_settop settop = nullptr;
    fn_lua_tolstring tolstring = nullptr;
    bool initialized = false;

    void Init() {
        if (initialized) return;
        XC("citizen-scripting-lua.dll", modLua);
        
        // Resolve functions dynamically from the game's Lua engine
        pcallk = Stealth::Api<fn_lua_pcallk>(modLua, "lua_pcallk");
        if (!pcallk) {
            // Fallback for older/different builds if lua_pcallk is not exported directly
            pcallk = Stealth::Api<fn_lua_pcallk>(modLua, "lua_pcall");
        }

        settop = Stealth::Api<fn_lua_settop>(modLua, "lua_settop");
        tolstring = Stealth::Api<fn_lua_tolstring>(modLua, "lua_tolstring");
        
        initialized = true;
    }
}

// ============================================================
// LuaEngine Implementation
// ============================================================

LE::LE() : _L(nullptr) {}
LE::~LE() {}

void LE::Attach(lua_State* st) {
    if (this->_L != st) {
        this->_L = st;
        LuaAPI::Init();
        _Flush();
    }
}

void LE::_Flush() {
    if (!_L) return;
    if (!LuaAPI::pcallk || !_pOrig) return; // If API is missing, we can't execute

    while (!_q.empty()) {
        std::string s = _q.front();
        _q.erase(_q.begin());

        // Load the string into a chunk on the top of the Lua stack
        if (_pOrig(_L, s.c_str(), s.size(), "chunk", "t") == LUA_OK) {
            // Execute the chunk safely
            if (LuaAPI::pcallk(_L, 0, LUA_MULTRET, 0, nullptr, nullptr) != LUA_OK) {
                // If execution fails, we can grab the error message
                if (LuaAPI::tolstring) {
                    const char* err = LuaAPI::tolstring(_L, -1, nullptr);
                    if (err) {
                        // In a real environment, log this to a console or file
                        OutputDebugStringA("[LuaEngine] Execution Error: ");
                        OutputDebugStringA(err);
                    }
                }
            }
        } else {
            // Compilation error
            if (LuaAPI::tolstring) {
                const char* err = LuaAPI::tolstring(_L, -1, nullptr);
                if (err) {
                    OutputDebugStringA("[LuaEngine] Compilation Error: ");
                    OutputDebugStringA(err);
                }
            }
        }
        
        // Clean up the stack
        if (LuaAPI::settop) {
            LuaAPI::settop(_L, 0); 
        }

        // Zero out script from memory after execution for stealth
        SecureZeroMemory(&s[0], s.size());
    }
}

void LE::Start() {}

void LE::Stop() {
    _L = nullptr;
}

void LE::Exec(const std::string& s) {
    _q.push_back(s);
    _Flush();
}
