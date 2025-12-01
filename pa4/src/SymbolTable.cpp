#include "SymbolTable.h"

SymbolTable::SymbolTable() {
    // Start with global scope
    enterScope();
}

void SymbolTable::enterScope() {
    scopes.push_back(std::unordered_map<std::string, int>());
}

void SymbolTable::exitScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

bool SymbolTable::define(const std::string& name, int value) {
    // Define in current scope
    auto& currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
        // Variable already defined in current scope
        return false;
    }
    currentScope[name] = value;
    return true;
}

bool SymbolTable::assign(const std::string& name, int value) {
    // 在当前作用域查找，如果找到就修改，否则在当前作用域定义新变量
    auto& currentScope = scopes.back();
    auto found = currentScope.find(name);
    if (found != currentScope.end()) {
        // 变量在当前作用域已定义，直接修改
        found->second = value;
        return true;
    }
    
    // 如果当前作用域没有，就在当前作用域定义新变量（不搜索外层作用域）
    currentScope[name] = value;
    return true;
}

bool SymbolTable::lookup(const std::string& name, int& value) const {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            value = found->second;
            return true;
        }
    }
    return false;
}

size_t SymbolTable::getCurrentScopeLevel() const {
    return scopes.size();
}