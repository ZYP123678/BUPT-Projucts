#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

class G {
private:
    std::set<std::string> N;
    std::set<char> T;
    std::map<std::string, std::set<std::vector<std::string>>> P; //单个string表示终结符或非终结符；vector<string>表示单个生成式右侧；set表示多个生成式
    inline void input_P();
    std::string S;
public:
    void input();
    void erase_epsilon();
    void erase_single_gener();
    void erase_unreachable();
    void output();
};

int main() {
    G g;
    g.input();
    g.erase_epsilon();
    g.erase_single_gener();
    g.erase_unreachable();
    g.output();
    return 0;
}

void G::input(){
    std::cout << "请输入非终结符个数, 与非终结符集合 N: " << std::endl;
    int n;
    std::cin >> n;
    for (int i = 0; i < n; i++) {
        std::string temp;
        std::cin >> temp;
        N.insert(temp);
    }
    std::cout << "请输入终结符个数, 与终结符集合 T: " << std::endl;
    int m;
    std::cin >> m;
    for (int i = 0; i < m; i++) {
        char temp;
        std::cin >> temp;
        T.insert(temp);
    }
    std::cout << "请输入起始符 S: " << std::endl;
    std::cin >> S;
    input_P();
}

inline void G::input_P(){
    std::cout << "请输入产生式集合 P:  // 使用#表示Epsilon空串" << std::endl;
    std::string left;
    std::vector<std::string> right;
    char c;
    c = getchar(); //读取换行符
    c = getchar();
    int state = 0; //0: 读取生成式左侧; 1: 读取生成式右侧
    while(!feof(stdin)){
        if(state == 0){
            if(c == '-'){
                state = 1;
                right.clear();
            }
            else
                left += c;
        }
        else if(state == 1){
            if(c == '\n'){
                state = 0;
                if(P.find(left) == P.end()){
                    std::set<std::vector<std::string>> temp;
                    temp.insert(right);
                    P[left] = temp;
                }
                else
                    P[left].insert(right);
                left.clear();
            }
            else{
                std::string temp(1, c);
                right.push_back(temp);
            }
        }
        c = getchar();
    }
}

void G::erase_epsilon(){

}

void G::erase_single_gener(){

}

void G::erase_unreachable(){

}

void G::output(){
    std::cout << "上下文无关文法 G = (N, T, P, S) 为: " << std::endl;
    std::cout << "  非终结符集合\t N = { ";
    for(auto it = N.begin(); it != N.end(); it++){
        std::cout << *it;
        if(it != --N.end())
            std::cout << ", ";
    }
    std::cout << " }" << std::endl;
    std::cout << "  终结符集合\t T = { ";
    for(auto it = T.begin(); it != T.end(); it++){
        std::cout << *it;
        if(it != --T.end())
            std::cout << ", ";
    }
    std::cout << " }" << std::endl;
    std::cout << "  生成式\t P = { ";
    for(auto it = P.begin(); it != P.end(); it++){
        std::cout << it->first << " -> ";
        for(auto itt = it->second.begin(); itt != it->second.end(); itt++){
            for(auto ittt = itt->begin(); ittt != itt->end(); ittt++){
                std::cout << *ittt;
            }
            if(itt != --it->second.end())
                std::cout << " | ";
        }
        if(it != --P.end())
            std::cout << ", ";
    }
    std::cout << " }" << std::endl;
    std::cout << "  起始符\t S = " << S << std::endl;
}
