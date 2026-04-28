#include "LuaEngine.h"
#include <windows.h>

// Lua stub definitions (replace with real Lua SDK)
struct lua_State {};
#define LUA_OK 0
typedef int (*lua_CFunction)(lua_State*);

int luaL_dostring(lua_State* L, const char* str) { return LUA_OK; }
void lua_pop(lua_State* L, int n) {}
const char* lua_tostring(lua_State* L, int idx) { return ""; }

LE::LE() : _L(nullptr) {}
LE::~LE() {}

void LE::Attach(lua_State* st) {
    if (this->_L != st) {
        this->_L = st;
        _Flush();
    }
}

void LE::_Flush() {
    if (!_L) return;
    while (!_q.empty()) {
        std::string s = _q.front();
        _q.erase(_q.begin());
        luaL_dostring(_L, s.c_str());
        // Zero out script from memory after execution
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
