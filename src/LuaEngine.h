#pragma once
#include <string>
#include <vector>

extern "C" {
    struct lua_State;
}

class LE {
public:
    LE();
    ~LE();

    void Start();
    void Stop();
    void Exec(const std::string& s);
    void Attach(lua_State* st);

private:
    lua_State* _L;
    std::vector<std::string> _q;
    void _Flush();
};
