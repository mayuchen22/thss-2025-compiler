#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, int>> scopes;

public:
    SymbolTable();
    
    void enterScope();
    void exitScope();
    
    bool define(const std::string& name, int value);
    bool assign(const std::string& name, int value);
    bool lookup(const std::string& name, int& value) const;
    
    size_t getCurrentScopeLevel() const;
};