//编写 LL(1)语法分析程序，实现对算术表达式的语法分析，在对输入的算术表达式进行分析的过程中，依次输出所采用的产生式，为给定文法自动构造预测分析表，并利用该分析表对输入的算术表达式进行语法分析。
//E -> E+T|E-T|T
//T -> T*F|T/F|F
//F -> (E)|num

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <sstream>

using namespace std;

// 文法中的符号类
class Symbol {
public:
    string value;  // 符号的值（字符串类型）
    bool isTerminal;  // 是否为终结符

    Symbol() {}
    Symbol(string val, bool terminal) : value(val), isTerminal(terminal) {}

    bool operator<(const Symbol& other) const {
        return value < other.value;
    }

    bool operator==(const Symbol& other) const {
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

// 语法分析器类
class GrammarAnalyzer {
private:
    map<string, Symbol> symbols;  // 保存终结符和非终结符
    vector<Production> productions;  // 保存文法产生式
    map<Symbol, set<Symbol>> first;  // FIRST集
    map<Symbol, set<Symbol>> follow;  // FOLLOW集
    Symbol startSymbol;  // 开始符
    map<pair<Symbol, Symbol>, vector<Symbol>> parsingTable;  // 分析表
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


    // 消除左递归
    void eliminateLeftRecursion() {
        vector<Production> newProductions;
        map<string, Symbol> newNonTerminals;

        for (auto& production : productions) {
            Symbol left = production.left;
            vector<vector<Symbol>> rightSide = production.right; 
            // 检查是否存在左递归
            bool LeftRecursion = false;

            for (const auto& right : rightSide) {
                if (!right.empty() && right[0].value == left.value) { // 直接左递归
                    LeftRecursion = true;
                    break;
                }
            }

            if (LeftRecursion) {
                string newN_Name = left.value + "'";
                while (newNonTerminals.find(newN_Name) != newNonTerminals.end()) {
                    newN_Name += "'"; // 确保非终结符唯一
                }
                Symbol newNonTerminal(newN_Name, false);
                newNonTerminals[newN_Name] = newNonTerminal;
                symbols[newN_Name] = Symbol(newN_Name, false);
                vector<vector<Symbol>> nonRecursive; // 存储非递归部分的产生式
                vector<vector<Symbol>> recursive; // 存储递归部分的产生式

                for (const auto& right : rightSide) {
                    if (!right.empty() && right[0].value == left.value) {
                        // 直接左递归部分
                        vector<Symbol> recursiveRight(right.begin() + 1, right.end());
                        recursiveRight.push_back(newNonTerminal);
                        recursive.push_back(recursiveRight);
                    } else {
                        // 非递归部分
                        vector<Symbol> nonRecursiveRight = right;
                        nonRecursiveRight.push_back(newNonTerminal);
                        nonRecursive.push_back(nonRecursiveRight);
                    }
                }
                // 添加新的产生式
                if (!nonRecursive.empty()) {
                    newProductions.emplace_back(left, nonRecursive); // 原产生式
                }
                if (!recursive.empty()) {
                    if (symbols.find("ε") == symbols.end()) {
                        symbols["ε"] = Symbol("ε", true);
                    }
                    recursive.push_back({symbols["ε"]}); // 添加ε产生式
                    newProductions.emplace_back(newNonTerminal, recursive); // 新产生式 
                }
            } else {
                // 如果没有左递归，保留原产生式
                newProductions.push_back(production);
            }
        }
        productions = newProductions;
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

    // 生成分析表，算法4.2
    void generateParsingTable() {
        for(auto& production : productions) {
            Symbol left = production.left;
            for(auto& right : production.right) {
                for(auto& s : symbols) {
                    Symbol symbol = s.second;
                    if(symbol.isTerminal && symbol.value != "ε") {
                        if(first[right[0]].find(symbol) != first[right[0]].end()) {
                            auto key = make_pair(left, symbol);
                            if (parsingTable.find(key) == parsingTable.end()) {
                                parsingTable[key] = right;
                            } else {
                                cout << "文法有冲突1"<< endl;
                                return;
                            }
                        }
                    }
                }
                if(first[right[0]].find(symbols["ε"]) != first[right[0]].end()) {
                    for(auto& s : follow[left]) {
                        auto key = make_pair(left, s);
                        if (parsingTable.find(key) == parsingTable.end()) {
                            parsingTable[key] = right;
                        } else {
                            cout << "文法有冲突" << endl;
                        }
                    }
                } 
            }
        }
        for(auto& s : symbols) {
            Symbol symbol = s.second;
            if(!symbol.isTerminal) {
                for(auto& b : follow[symbol]) {
                    if(b.isTerminal) {
                        auto key = make_pair(symbol, b);
                        if (parsingTable.find(key) == parsingTable.end()) {
                            if (symbols.find("synch") == symbols.end()) {
                                symbols["synch"] = Symbol("synch", true);
                            }
                            parsingTable[key] = {symbols["synch"]};
                        }
                    }
                }
            }   
        }
    }

    void parsing() {
        string inputString;
        cout << "请输入要分析的句子，以$结尾: ";
        getline(cin, inputString);
        vector<Symbol> input;
        stringstream ss(inputString);
        string token;
    
        while (ss >> token) {
            if (symbols.find(token) != symbols.end()) {
                input.push_back(symbols[token]);
            } else {
                cout << "Error: 未知符号 " << token << endl;
                return;
            }
        }
        vector<Symbol> stack;
        input.push_back(symbols["$"]);
        
        stack.push_back(symbols["$"]);
        stack.push_back(startSymbol);

        size_t i = 0;
        while(!stack.empty()) {
            Symbol X = stack.back();
            Symbol a = input[i];
            
            if(X.isTerminal || X.value == "$") {
                if(X == a) {
                    stack.pop_back();
                    i++;
                } else {
                    cout << "Error: 终结符 " + X.value + "与输入符号" + a.value + "不匹配，弹出" + X.value<< endl;
                    stack.pop_back();
                }
            }
            else {
                auto key = make_pair(X, a);
                if(parsingTable.find(key) != parsingTable.end()) {
                    vector<Symbol> right = parsingTable[key];
                    if(right.size() == 1 && right[0].value == "synch") {
                        cout << "Error: 出错 M[" + X.value + "," + a.value + "] = synch, 弹出" + X.value << endl;
                        stack.pop_back();
                    }
                    else {
                        cout << X.value << "->";
                        for(auto& symbol : right) {
                            cout << symbol.value;
                        }
                        cout << endl;
                        stack.pop_back();
                        if(!(right.size() == 1 && right[0].value == "ε")) {
                            for(int j = right.size() - 1; j >= 0; j--) {
                                stack.push_back(right[j]);
                            }
                        }
                    }
                } else {
                    cout << "Error: M[" + X.value + "," + a.value + "] = 空白, 跳过" + a.value << endl;
                    i++;
                }
            }
        }
        cout << "分析完成" << endl;
    }
};

int main() {
    GrammarAnalyzer analyzer;

    // 读取并存储文法
    analyzer.readGrammar();

    // 消除左递归
    analyzer.eliminateLeftRecursion();

    // 生成FIRST集
    analyzer.FirstSets();

    // 生成FOLLOW集
    analyzer.FollowSets();

    // 生成分析表
    analyzer.generateParsingTable();

    //非递归预测
    analyzer.parsing();
    return 0;
}
