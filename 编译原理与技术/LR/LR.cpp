//编写 LR语法分析程序，实现对算术表达式的语法分析，在对输入的算术表达式进行分析的过程中，依次输出所采用的产生式，为给定文法自动构造预测分析表，并利用该分析表对输入的算术表达式进行语法分析。
//E -> E+T|E-T|T
//T -> T*F|T/F|F
//F -> (E)|num

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <queue>

using namespace std;

// 文法中的符号类
class Symbol {
public:
    string value;  // 符号的值（字符串类型）
    bool isTerminal;  // 是否为终结符

    Symbol() {}
    Symbol(string val, bool terminal) : value(val), isTerminal(terminal) {}

    bool operator <(const Symbol& other) const {
        return value < other.value;
    }

    bool operator ==(const Symbol& other) const {
        return value == other.value;
    }
};

// 文法规则类
class Production {
public:
    Symbol left;  
    vector<vector<Symbol>> right; 
    Production() {}
    Production(Symbol l, vector<vector<Symbol>> r) : left(l), right(r) {}
};

//项目类（点位于产生式右侧的当前位置）
class Item {
public:
    Production production;
    int dotPosition;

    Item(Production prod, int dot) : production(prod), dotPosition(dot) {}

    bool operator <(const Item& other) const { 
        if(production.left.value != other.production.left.value) {
            return production.left.value < other.production.left.value;
        }
        if(dotPosition != other.dotPosition) {
            return dotPosition < other.dotPosition;
        }
        return production.right < other.production.right;
    } 

    bool operator ==(const Item& other) const {
        return production.left.value == other.production.left.value 
            && dotPosition == other.dotPosition 
            && production.right == other.production.right;
    }
};

class ItemSet {
public:
    set<Item> items;

    bool operator <(const ItemSet& other) const {
        return items < other.items;
    }
    bool operator ==(const ItemSet& other) {
        return items == other.items;
    }
};

ItemSet closure(ItemSet& itemset, vector<Production>& productions) {
    ItemSet result = itemset;
    bool changed = true;
    while(changed) {
        changed = false;
        for(auto& item : result.items) {
            auto rightSide = item.production.right;
            for(auto right : rightSide) {
                if(item.dotPosition < right.size()) {
                    Symbol B = right[item.dotPosition];
                    if(!B.isTerminal) {
                        for(auto& production : productions) {
                            if(production.left == B) {
                                Item newItem = Item(production, 0);
                                if(result.items.find(newItem) == result.items.end()) {
                                    result.items.insert(newItem);
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

ItemSet gotoSet(ItemSet& itemset, Symbol X, vector<Production>& productions) {
    ItemSet gotoSet;
    for(auto& item : itemset.items) {
        auto rightSide = item.production.right;
        for(auto right : rightSide) {
            if(item.dotPosition < right.size()) {
                if(right[item.dotPosition] == X) {
                    Production newProduction(item.production.left, {right});
                    Item newItem = Item(newProduction, item.dotPosition + 1);
                    gotoSet.items.insert(newItem);
                }
            }
        }
    }
    return closure(gotoSet, productions);
}

// 语法分析器类
class GrammarAnalyzer {
private:
    map<string, Symbol> symbols;  // 保存终结符和非终结符
    vector<Production> productions;  // 保存文法产生式
    map<Symbol, set<Symbol>> first;  // FIRST集
    map<Symbol, set<Symbol>> follow;  // FOLLOW集
    map<ItemSet, int> itemSetMap;
    vector<ItemSet> dfaStates;
    Symbol startSymbol;  // 开始符
    map<pair<int, Symbol>, tuple<string, int, Production>> parsingTable;  // 分析表
    void parseSymbols(const string& input, bool isTerminal) {
        stringstream ss(input);
        string symbol;
        while (getline(ss, symbol, ',')) {
            symbols[symbol] = Symbol(symbol, isTerminal);
        }
    }
public:
    // 读取文法
    void readGrammar() {
        string nonTerminals;
        cout << "请输入非终结符集N，以逗号分隔: ";
        getline(cin, nonTerminals);
        parseSymbols(nonTerminals, false); 

        string terminals;
        cout << "请输入终结符集T，以逗号分隔: ";
        getline(cin, terminals);
        parseSymbols(terminals, true);

        string start;
        cout << "请输入开始符号S: ";
        getline(cin, start);
        startSymbol = symbols[start]; 

        cout << "请输入产生式P（每行一个产生式，右侧多个生成式用 | 分隔:\n";
        string line;
        while (getline(cin, line)) {
            if (line == ".") {
                return;
            }
            stringstream ss(line);
            string left, arrow;
            ss >> left >> arrow;  // 读取产生式左部和"->"

            // 确保箭头是"->"
            if (arrow != "->") {
                cerr << "产生式格式错误: " << line << endl;
                continue;  // 跳过格式错误的产生式
            }

            // 读取右侧的生成式
            string rightSide;
            getline(ss, rightSide);  // 获取整行右侧生成式
            stringstream rightStream(rightSide);
            string right;

            // 对相同左侧的多个右侧生成式进行处理
            while (getline(rightStream, right, '|')) { 
                vector<Symbol> symbolsRight;  // 保存当前生成式的右部符号
                stringstream rightParts(right);
                string symbol;

                // 处理每个右侧生成式的符号
                while (rightParts >> symbol) { 
                    symbolsRight.push_back(symbols[symbol]);  // 从符号表中获取符号
                }

                // 如果该左侧已经有产生式，合并右部符号
                bool found = false;
                for (auto& prod : productions) {
                    if (prod.left.value == symbols[left].value) {
                        prod.right.push_back(symbolsRight);  // 将右侧生成式加入已存在的产生式
                        found = true;
                        break;
                    }
                }

                // 如果该左侧没有产生式，添加新的产生式
                if (!found) {
                    productions.emplace_back(symbols[left], vector<vector<Symbol>>{symbolsRight});  // 添加新的产生式
                }
            }
        }

        cout << "文法输入完成" << endl;
    }

    // 生成FIRST集
    void FirstSets() {
        bool changed = true;
        while(changed) {
            changed = false;
            for(auto& it : symbols) {
                Symbol symbol = it.second;
                if(symbol.isTerminal) {
                    if(first[symbol].insert(symbol).second) changed = true;
                }
                else {
                    for (auto& production : productions) {
                        if (production.left == symbol) {
                            for (auto& right : production.right) {
                                bool allContainEpsilon = true;  // 用于检查右部所有符号是否可以推导为空
                                
                                // 遍历产生式右部的每个符号 Yi
                                for (size_t i = 0; i < right.size(); ++i) {
                                    Symbol Yi = right[i];
                                    
                                    // 将 FIRST(Yi) 中所有非空元素加入到 FIRST(X) 中
                                    for (const Symbol& sym : first[Yi]) {
                                        if (sym.value != "ε") {  // 排除 ε
                                            if (first[symbol].insert(sym).second) {
                                                changed = true;
                                            }
                                        }
                                    }

                                    // 检查 FIRST(Yi) 是否包含 ε
                                    if (first[Yi].find(symbols["ε"]) == first[Yi].end()) {
                                        allContainEpsilon = false;  // 如果没有 ε，停止继续检查
                                        break;
                                    }
                                }

                                // 如果右部的所有符号都可以推导为空，将 ε 加入 FIRST(X) 中
                                if (allContainEpsilon) {
                                    if (first[symbol].insert(symbols["ε"]).second) {
                                        changed = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 生成FOLLOW集
    void FollowSets() {
        bool changed = true;
        while(changed) {
            changed = false;
            for(auto& it : symbols) {
                Symbol symbol = it.second;
                if(!symbol.isTerminal) {
                    if(symbol == startSymbol) {
                        if (symbols.find("$") == symbols.end()) {
                            symbols["$"] = Symbol("$", true);
                        }
                        if(follow[symbol].insert(symbols["$"]).second) changed = true;
                    }
                    for(auto& production : productions) {
                        const Symbol& A = production.left;
                        const vector<vector<Symbol>>& rightSide = production.right;
                        
                        for(auto& right : rightSide) {
                            for(int i = 0; i < right.size(); i++) {
                                Symbol B = right[i];
                                
                                if(!B.isTerminal) {
                                    //后面还有符号,A -> αBβ
                                    if(i + 1 < right.size()) {
                                        for(auto& s : first[right[i + 1]]) {
                                            if(s.value != "ε") {
                                                if(follow[B].insert(s).second) changed = true;
                                            }
                                        }
                                        //若β能推导出ε
                                        if(first[right[i+1]].find(symbols["ε"]) != first[right[i+1]].end()) {
                                            for(auto& s : follow[A]) {
                                                if(follow[B].insert(s).second) changed = true;
                                            }
                                        }
                                    } 
                                    else {
                                        for(auto& s : follow[A]) {
                                            if(follow[B].insert(s).second) changed = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    // 生成可以识别输入文法所有活前缀的DFA
    void generateDFA() {
        //首先生成拓广文法
        symbols["S'"] = Symbol("S'",false);
        productions.emplace_back(symbols["S'"], vector<vector<Symbol>>{{startSymbol}});
        startSymbol = symbols["S'"];

        //在状态0先输入拓广文法第一句，然后求闭包
        ItemSet startItemSet;
        startItemSet.items.insert(Item(productions.back(), 0));
        startItemSet = closure(startItemSet, productions);

        queue<ItemSet> itemQueue;
        itemQueue.push(startItemSet);
        itemSetMap[startItemSet] = 0;
        dfaStates.push_back(startItemSet);

        while(!itemQueue.empty()) {
            ItemSet currentSet = itemQueue.front();
            itemQueue.pop();
            int currentState = itemSetMap[currentSet];

            for(auto& s : symbols) {
                Symbol symbol = s.second;
                if(symbol.value == "ε" || symbol.value == "$") continue;
                ItemSet gotoSets = gotoSet(currentSet, symbol, productions);
                if(!gotoSets.items.empty()) {
                    if(itemSetMap.find(gotoSets) == itemSetMap.end()) {
                        itemSetMap[gotoSets] = dfaStates.size();
                        dfaStates.push_back(gotoSets);
                        itemQueue.push(gotoSets);
                    }
                    int nextState = itemSetMap[gotoSets];
                    cout << "状态" << currentState << "通过" << symbol.value << "转移到状态" << nextState << endl;
                }
            }
        }
        cout << "DFA生成完成" << endl;
    }
    // 生成分析表
    void generateParsingTable() {
        for(int state = 0; state < dfaStates.size(); state++) {
            ItemSet itemSet = dfaStates[state];
            for(auto& item : itemSet.items) {
                auto rightSide = item.production.right;
                for(auto right : rightSide) {
                    if(item.dotPosition == right.size()) {
                        for(auto s : follow[item.production.left]) {
                            if(parsingTable.find({state, s}) != parsingTable.end()) {
                                cerr << "文法不是SLR文法" << endl;
                                return;
                            }
                            Production newProduction(item.production.left, {right});
                            parsingTable[{state, s}] = {"R", -1, newProduction};
                        }
                    }
                    else {
                        Symbol nextSymbol = right[item.dotPosition];
                        if(nextSymbol.isTerminal) {
                            ItemSet gotoSets = gotoSet(itemSet, nextSymbol, productions);
                            if(itemSetMap.find(gotoSets) != itemSetMap.end()) {
                                int nextState = itemSetMap[gotoSets];
                                parsingTable[{state, nextSymbol}] = {"S", nextState, Production()};
                            }
                            else {
                                cerr << "文法不是SLR文法" << endl;
                                return;
                            }
                        }
                        else {
                            int nextState = itemSetMap[gotoSet(itemSet, nextSymbol, productions)];
                            parsingTable[{state, nextSymbol}] = {"G", nextState, Production()};
                        }
                    }
                }
                if(item.production.left == startSymbol && item.dotPosition == item.production.right[0].size()) {
                    parsingTable[{state, symbols["$"]}] = {"ACC", -1, Production()};
                }
            }
        }
    }

    void parsing() {
        string input;
        cout << "请输入要分析的字符串: ";
        getline(cin, input);
        vector<Symbol> inputSymbols;
        istringstream iss(input);
        string s;
        while (iss >> s) {  // Read each symbol separated by spaces
            if (symbols.find(s) == symbols.end()) {
                cerr << "输入包含未知符号 " << s << endl;
                return;
            }
            inputSymbols.push_back(symbols[s]);
        }

        vector<int> stateStack = {0};
        vector<Symbol> symbolStack = {symbols["$"]};
        int pos = 0;

        while(pos < inputSymbols.size()) {
            int currentState = stateStack.back();
            Symbol currentSymbol = inputSymbols[pos];
            if(parsingTable.find({currentState, currentSymbol}) == parsingTable.end()) {
                cerr << "无法识别的输入" << endl;
                return;
            }
            auto action = parsingTable[{currentState, currentSymbol}];
            if(get<0>(action) == "S") {
                stateStack.push_back(get<1>(action));
                symbolStack.push_back(currentSymbol);
                pos++;
                cout << "移进 " << currentSymbol.value << " Shift " << get<1>(action) << endl;
            }
            else if(get<0>(action) == "R") {
                Production production = get<2>(action);
                for(int i = 0; i < production.right[0].size(); i++) {
                    stateStack.pop_back();
                    symbolStack.pop_back();
                }
                Symbol left = production.left;
                currentState = stateStack.back();
                stateStack.push_back(get<1>(parsingTable[{currentState, left}]));
                symbolStack.push_back(left);
                cout << "规约 " << production.left.value << "->";
                for(auto& s : production.right[0]) {
                    cout << s.value;
                }
                cout << endl;
            }
            else if(get<0>(action) == "ACC") {
                cout << "分析成功" << endl;
                return;
            }
            else {
                cerr << "无法识别的输入" << endl;
                return;
            }
        }

    }

};

int main() {
    GrammarAnalyzer analyzer;

    // 读取并存储文法
    analyzer.readGrammar();

    // 生成FIRST集
    analyzer.FirstSets();

    // 生成FOLLOW集
    analyzer.FollowSets();

    analyzer.generateDFA();
    // 生成分析表
    analyzer.generateParsingTable();

    //非递归预测
    analyzer.parsing();
    return 0;
}
