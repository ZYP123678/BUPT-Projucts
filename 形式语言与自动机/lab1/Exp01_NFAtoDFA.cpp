# include<iostream>
# include<string>
# include<vector>
# include<set>
# include<map>

bool findChar(std::vector<char> vec, char c){
    for(int i = 0; i < vec.size(); i++)
       if(vec[i] == c)
            return true;
    return false;
}
template <typename T>
bool setsEqual(const std::set<T>& set1, const std::set<T>& set2) {
    if (set1.size() != set2.size()) {
        return false;
    }
    for (const auto& elem : set1) {
        if (set2.find(elem) == set2.end()) {
            return false;
        }
    }
    return true;
}

class FA{
    private:
        class sigma2{
            public:
                std::set<char> pre;//转移前状态
                char input;//输入

                bool operator < (const sigma2 &s) const{
                    if (pre == s.pre)
                        return input < s.input;
                    return pre < s.pre;
                }
        };
        std::vector<char> Q; //状态集
        std::string T; //字母表
        std::map<sigma2,std::set<char>> Sigma; //转移函数
        char q0; //初始状态
        std::vector<char> F; //终态集

        std::map<std::set<char>, char> mapRelation; //将转化中的状态子集映射到新的状态

        bool foundSet_inMap(std::map<sigma2,std::set<char>> m, std::set<char> s){
            for(auto it = m.begin(); it != m.end(); it++)
                if(setsEqual(it->first.pre,s))
                    return true;
            return false;
        }

        auto findSet_inMap(std::map<sigma2,std::set<char>> m, std::set<char> s){
            for(auto it = m.begin(); it != m.end(); it++)
                if(setsEqual(it->first.pre,s))
                    return it;
            return m.end();
        }

        auto findSet_inMap(std::map<sigma2,std::set<char>> m, std::set<char> s, char c){
            for(auto it = m.begin(); it != m.end(); it++)
                if(setsEqual(it->first.pre,s) && it->first.input == c)
                    return it;
            return m.end();
        }

        void createNew_mapRelation(const std::map<sigma2,std::set<char>> Sigma, std::map<sigma2,std::set<char>> &sigma, sigma2 &temp_sigma2, const std::string T){
            for(int i = 0; i < T.size(); i++){
                temp_sigma2.input = T[i];
                std::set<char> temp_Next_set;
                for(auto it = temp_sigma2.pre.begin(); it != temp_sigma2.pre.end(); it++){ //遍历状态集的一个状态it
                    std::set<char> temp_set;
                    temp_set.insert(*it);
                    if(foundSet_inMap(Sigma, temp_set) && findSet_inMap(Sigma, temp_set, T[i])->first.input == T[i]){ //找到状态集的一个状态it对应状态的转移函数
                        std::set<char> find_next_set = findSet_inMap(Sigma, temp_set, T[i])->second;
                        temp_Next_set.insert(find_next_set.begin(), find_next_set.end());
                    }
                }
                if(!temp_Next_set.empty())
                    sigma[temp_sigma2] = temp_Next_set;
            }
        }
    public:
        void inputNFA();
        FA NFAtoDFA();
        void outputDFA();
};

void FA::inputNFA(){
    int Q_num, Sigma_num, F_num;
    std::cin >> Q_num >> Sigma_num >> F_num;
    Q.resize(Q_num);
    for(int i = 0; i < Q_num; i++)
        std::cin >> Q[i];
    std::cin >> T;
    for(int i = 0; i < Sigma_num; i++){
        sigma2 temp_sigma;
        char pre;
        std::cin >> pre >> temp_sigma.input;//输入转移前状态和输入
        temp_sigma.pre.insert(pre);//将转移前状态加入集合
        char temp_input;//输入转移后状态
        std::set<char> temp;
        temp_input = getchar();
        while(temp_input != '\n'){
            temp.insert(temp_input);
            temp_input = getchar();
        }
        Sigma[temp_sigma] = temp;//将转移函数加入Sigma
    }
    std::cin >> q0;
    F.resize(F_num);
    for(int i = 0; i < F_num; i++)
        std::cin >> F[i];
    return;
}

FA FA::NFAtoDFA(){
    FA DFA;
    DFA.T = T;
    DFA.q0 = q0;
    std::map<sigma2,std::set<char>> temp_sigma;//新的转移函数
    //将起始状态对应的转换函数加入temp_sigma
    for(int i = 0; i < T.size(); i++){
        sigma2 temp_sigma2;
        temp_sigma2.pre.insert(q0);//将起始状态加入集合
        temp_sigma2.input = T[i];//输入
        std::set<char> temp;
        if(Sigma.find(temp_sigma2) != Sigma.end())
            temp = Sigma[temp_sigma2];
        if(!temp.empty())
            temp_sigma[temp_sigma2] = temp;
    }

    bool inCircle = true;
    while(inCircle){ //直到没有新的状态集合产生
        inCircle = false;
        for(auto it = temp_sigma.begin(); it != temp_sigma.end(); it++){ //遍历每一个状态集合,找到新的状态集合
            if(foundSet_inMap(temp_sigma,it->second))
                continue;//如果已经找到了这个状态集合,则跳过
            else{
                inCircle = true;
                sigma2 temp_sigma2;
                temp_sigma2.pre = it->second;//将新的状态集合加入集合
                createNew_mapRelation(Sigma, temp_sigma ,temp_sigma2,DFA.T); //根据新的状态集合生成新的转移函数
            }
        }
    }

    DFA.Sigma = temp_sigma;
    DFA.F = F;
    return DFA;
}

void FA::outputDFA(){
    std::cout << "Q: 状态集 ";
    std::set<std::set<char>> temp_Q;
    for(auto it = Sigma.begin(); it != Sigma.end(); it++)
        temp_Q.insert(it->first.pre);
    for(auto it = temp_Q.begin(); it != temp_Q.end(); it++){
        std::cout << "{";
        for(auto it2 = it->begin(); it2 != it->end(); it2++)
            std::cout << " " << *it2;
        std::cout << " } ";
    }
    std::cout << std::endl;
    std::cout << "T: 字母表 " << T << std::endl;
    std::cout << "Sigma: 转移函数" << std::endl;
    for(auto it = Sigma.begin(); it != Sigma.end(); it++){
        std::cout << "转移前状态: ";
        std::cout << "{";
        for(auto it2 = it->first.pre.begin(); it2 != it->first.pre.end(); it2++)
            std::cout << " " << *it2;
        std::cout << " } \t";
        std::cout << "输入: " << it->first.input << "\t 转移后状态: ";
        std::cout << "{";
        for(auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
            std::cout << " " << *it2;
        std::cout << " } ";
        std::cout << std::endl;
    }
    std::cout << "q0: 初始状态 " << q0 << std::endl;
    std::cout << "F: 终态集 ";
    std::set<std::set<char>> temp_F;
    for(auto it = Sigma.begin(); it != Sigma.end(); it++){
        bool notIncludeF = true;
        for(auto it2 = it->first.pre.begin(); it2 != it->first.pre.end(); it2++){
            if(findChar(F,*it2))
                notIncludeF = false;
        }
        if(notIncludeF)
            continue;
        else
            temp_F.insert(it->first.pre);
    }

    for(auto it = temp_F.begin(); it != temp_F.end(); it++){
        std::cout << "{";
        for(auto it2 = it->begin(); it2 != it->end(); it2++)
            std::cout << " " << *it2;
        std::cout << " } ";
    }
    std::cout << std::endl;
    return;
}

int main(){
    FA NFA;
    NFA.inputNFA();
    FA DFA = NFA.NFAtoDFA();
    DFA.outputDFA();
    return 0;
}