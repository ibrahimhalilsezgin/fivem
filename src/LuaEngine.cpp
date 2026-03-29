#include "LuaEngine.h"
#include <iostream>

// --- Lua Sağlam Tanımları ---
// Gerçek projede bunu gerçek Lua/FiveM SDK'sı ile değiştireceksiniz.
struct lua_State {};
#define LUA_OK 0
typedef int (*lua_CFunction) (lua_State *L);

// Sahte Lua fonksiyonları
int luaL_dostring(lua_State* L, const char* str) { return LUA_OK; }
void lua_pop(lua_State* L, int n) {}
const char* lua_tostring(lua_State* L, int idx) { return "Lua_Sahte_String"; }
// -----------------------------
LuaEngine::LuaEngine() : L(nullptr) {}

LuaEngine::~LuaEngine() {}

// Dışarıdan yakaladığımız state'i buraya atarız
void LuaEngine::SetActiveState(lua_State* state) {
    if (this->L != state) {
        this->L = state;
        std::cout << "[+] Yeni bir lua_State (" << state << ") yakalandi ve eklendi!" << std::endl;
        
        // Eğer bekleyen scriptler varsa çalıştır
        ProcessQueue();
    }
}

void LuaEngine::ProcessQueue() {
    if (!L) return;

    while (!scriptQueue.empty()) {
        std::string script = scriptQueue.front();
        scriptQueue.erase(scriptQueue.begin());

        std::cout << "[*] Siradaki Lua betigi FiveM (veya hedef) state'inde calistiriliyor..." << std::endl;
        
        if (luaL_dostring(L, script.c_str()) != LUA_OK) {
            std::cerr << "[-] Lua Calistirma Hatasi: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
        } else {
            std::cout << "[+] Lua betigi basariyla tamamlandi." << std::endl;
        }
    }
}

void LuaEngine::Initialize() {
    std::cout << "[+] Lua Engine baslatildi. FiveM lua_State'inin yakalanmasi bekleniyor..." << std::endl;
}

void LuaEngine::Shutdown() {
    std::cout << "[-] Lua Engine kapatiliyor..." << std::endl;
    L = nullptr; // State bizim olmadığı için silmiyoruz (delete/lua_close YAPMIYORUZ)
}

void LuaEngine::ExecuteString(const std::string& script) {
    // Script'i sıraya ekle
    scriptQueue.push_back(script);
    std::cout << "[*] Lua betigi siraya eklendi. (" << scriptQueue.size() << " bekleyen var)" << std::endl;
    
    // Eğer hali hazırda aktif bir state varsa hemen çalıştırır
    ProcessQueue();
}
