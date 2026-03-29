#pragma once
#include <string>
#include <vector>

extern "C" {
    struct lua_State;
}

class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    void Initialize();
    void Shutdown();
    void ExecuteString(const std::string& script);
    
    // FiveM'in çalışan lua_State'ini çaldığımızda buraya set edeceğiz
    void SetActiveState(lua_State* state);

private:
    lua_State* L;
    std::vector<std::string> scriptQueue; // Scriptleri bekletmek için kuyruk
    
    void ProcessQueue();
};

