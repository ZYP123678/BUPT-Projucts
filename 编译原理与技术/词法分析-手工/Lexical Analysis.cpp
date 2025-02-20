#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <set>
#include <algorithm>

using namespace std;

#define BUFFERSIZE 4096


class LexicalAnalysis
{
    ifstream file;
    int state = 0;
    char C;
    string token;
    bool end = false;
    bool is_key = false;
    char buffer1[BUFFERSIZE];
    char buffer2[BUFFERSIZE];
    int current; //缓冲区下标
    int char_flag = 0; //由于字符字符串都要识别里面的转义字符，所以需要一个标志来判断是否是字符或字符串转到转义字符去的状态
    int buffer_state = 0; //缓冲区状态
    int lines = 1; //记录行数
    int letters = 0; //记录字符数
    int last_pos = 0;
    int current_pos = 0; //记录字符在每行的第几个
    int comment_state = 0; //注释状态
    char string_flag; //字符串标志
    int id_num = 0;
    int int_num = 0;
    int float_num = 0;
    int op_num = 0;
    int key_num = 0;
    int other_num = 0;
    vector<string> symbol_table;
    vector<pair<string, string>> table; //对识别出的记号以二元表的形式存储
    vector<string> errors;
    set<string> keywords = {
        "auto", "break", "case", "char", "const", 
        "continue", "default", "do", "double", "else", 
        "enum", "extern", "double", "float", "for", "goto", "if", "inline",
        "int", "long", "register", "return", "short", "signed", 
        "sizeof", "static", "struct", "switch", "typedef", "union", 
        "unsigned", "void", "volatile", "while"
    };
public:
    void get_char(); //每调用一次根据向前指针从输入缓冲区读取一个字符并放在c中然后移动current
    void fill_buffer(int buffer_number); //填充缓冲区
    void get_nbc(); //每次调用检查C中字符是否为空格，若是则反复调用get_char直至C中字符不为空格
    void cat(); //将C中字符连接到token的字符串后
    void retract(); //向前指针回退一个字符
    bool is_letter(); //判断C中字符是否为字母
    bool is_digit(); //判断C中字符是否为数字
    bool is_reserved(string token); //判断token是否为保留字
    string* symbol_insert(string token); //将token插入标识符表
    void table_insert(string mark, string attr); //将token插入二元表
    void error(string problem); //报告错误
    void output(); //输出结果
    void run();
    LexicalAnalysis(string filename) {
        for(int i = 0; i < BUFFERSIZE; i++) {
            buffer1[i] = -1;
            buffer2[i] = -1;
        }
        buffer1[BUFFERSIZE] = '\0';
        buffer2[BUFFERSIZE] = '\0';
        file.open(filename, ios::in);    
        if (!file.is_open()) {
            cout << "Error opening file";
            exit(1);
        }
    };
};

// 填充缓冲区
void LexicalAnalysis::fill_buffer(int buffer_number) {
    if (buffer_number == 1) {
        file.read(buffer1, BUFFERSIZE);  // 读取最多 BUFFERSIZE 个字符到缓冲区1
        if (file.gcount() < BUFFERSIZE) {  // 如果读取的字符数量小于 BUFFERSIZE，说明文件读取结束
            buffer1[file.gcount()] = '\0';  // 在缓冲区末尾添加结束符
        }
    } else if (buffer_number == 2) {
        file.read(buffer2, BUFFERSIZE);  // 读取最多 BUFFERSIZE 个字符到缓冲区2
        if (file.gcount() < BUFFERSIZE) {
            buffer2[file.gcount()] = '\0';
        }
    }
}

// 字符读取函数
void LexicalAnalysis::get_char() {
    if (buffer_state == 0) {  // 初始状态时，填充第一个缓冲区
        fill_buffer(1);
        buffer_state = 1;  // 设置状态为第一个缓冲区
        C = buffer1[0];
        current = 1;
    } 
    else if (buffer_state == 1) {  // 正在使用第一个缓冲区
        if (buffer1[current] == '\0' || current >= BUFFERSIZE) {  // 检查是否到达缓冲区末尾
            if(file.eof()) {  // 检查是否到达文件末尾
                end = true;
                C = '\0';
                return;
            }
            fill_buffer(2);  // 填充第二个缓冲区
            buffer_state = 2;
            C = buffer2[0];
            current = 1;
        } else {
            C = buffer1[current++];
        }
    } 
    else if (buffer_state == 2) {  // 正在使用第二个缓冲区
        if (buffer2[current] == '\0' || current >= BUFFERSIZE) {  // 检查是否到达缓冲区末尾
            if(file.eof()) {  // 检查是否到达文件末尾
                end = true;
                C = '\0';
                return;
            }
            fill_buffer(1);  // 填充第一个缓冲区
            buffer_state = 1;
            C = buffer1[0];
            current = 1;
        } else {
            C = buffer2[current++];
        }
    }

    // 检查当前字符是否为多字节字符（UTF-8）
    int first_byte = (unsigned char)C;
    if ((first_byte & 0x80) == 0) {
        // 单字节字符 (ASCII)
        letters++;
        current_pos++;
    } else if ((first_byte & 0xE0) == 0xC0) {
        // 两字节 UTF-8 字符
        letters++;
        current_pos++;
        // 跳过剩下的字节
        if (buffer_state == 1) {
            C = buffer1[current++];  // 读取第二个字节
        } else {
            C = buffer2[current++];  // 读取第二个字节
        }
    } else if ((first_byte & 0xF0) == 0xE0) {
        // 三字节 UTF-8 字符
        letters++;
        current_pos++;

        if (buffer_state == 1) {
            C = buffer1[current++];
            C = buffer1[current++];
        } else {
            C = buffer2[current++];
            C = buffer2[current++];
        }
    } else if ((first_byte & 0xF8) == 0xF0) {
        // 四字节 UTF-8 字符
        letters++;
        current_pos++;

        if (buffer_state == 1) {
            C = buffer1[current++];
            C = buffer1[current++];
            C = buffer1[current++];
        } else {
            C = buffer2[current++];
            C = buffer2[current++];
            C = buffer2[current++];
        }
    }

    // 统计行数
    if (C == '\n') {
        lines++;
        last_pos = current_pos;
        current_pos = 0;
        letters++;
    } else if (C == '\t') {
        current_pos += 4;  // 将制表符算作4个字符
    }
}
void LexicalAnalysis::get_nbc() {
    while (C == ' ' || C == '\t' || C == '\n') {
        get_char();
        if(end) return;
    }
}

void LexicalAnalysis::cat() {
    token += C;
}

void LexicalAnalysis::retract() {
    if(current == 0) {
        current = BUFFERSIZE - 1;
        if(buffer_state == 1) {
            buffer_state = 2;
        } else {
            buffer_state = 1;
        }
    }
    else {
        current--;
        letters--;
    }

    if(C == '\n') {
        lines--;
        letters--;
        current_pos = last_pos;
    }
    else if(C == '\t') {
        current_pos -= 4;
    }
    else {
        current_pos--;
    }
    
}

bool LexicalAnalysis::is_letter() {
    return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z');
}

bool LexicalAnalysis::is_digit() {
    return C >= '0' && C <= '9';
}

bool LexicalAnalysis::is_reserved(string token) {
    return keywords.find(token) != keywords.end();
}

//返回符号表的入口指针
string* LexicalAnalysis::symbol_insert(string token) {
    auto it = std::find(symbol_table.begin(), symbol_table.end(), token);
    if(it == symbol_table.end()) {
        symbol_table.push_back(token);
        return &symbol_table.back();
    }
    return &(*it);   
}

void LexicalAnalysis::table_insert(string mark, string attr) {
    table.push_back(make_pair(mark, attr));
}

void LexicalAnalysis::error(string problem) {
    string all_problem = "Error at line " + to_string(lines) + " position " + to_string(current_pos)+ ": " + problem;
    errors.push_back(all_problem);
}

void LexicalAnalysis::output() {
    cout << "Total lines: " << lines << endl;
    cout << "Total letters: " << letters << endl;
    cout << "Total identifiers: " << id_num << endl;
    cout << "Total int numbers: " << int_num << endl;
    cout << "Total float numbers: " << float_num << endl;
    cout << "Total operators: " << op_num << endl;
    cout << "Total keywords: " << key_num << endl;
    cout << "Total others: " << other_num << endl;
    cout << "Symbol table: "<<endl;
    for (int i = 0; i < symbol_table.size(); i++) {
        cout << symbol_table[i] << " ";
    }
    cout << endl;
    cout << "Table: " << endl;
    for (int i = 0; i < table.size(); i++) {
        cout << "<" << table[i].first << " , " << table[i].second << ">" << endl;
    }
    cout << "Errors: " << endl;
    for (int i = 0; i < errors.size(); i++) {
        cout << errors[i] << endl;
    }
}

void LexicalAnalysis::run() {
    state = 0;
    while (!end) {
        switch (state) {
            case 0: {
                token = ""; //初始化
                get_char();
                get_nbc();
                switch (C) {
                    case ',':   table_insert(","," ");    other_num++;    break;
                    case ';':   table_insert(";"," ");    other_num++;    break;
                    case '(':   table_insert("("," ");    other_num++;    break;
                    case ')':   table_insert(")"," ");    other_num++;    break;
                    case '[':   table_insert("["," ");    other_num++;    break;
                    case ']':   table_insert("]"," ");    other_num++;    break;
                    case '{':   table_insert("{"," ");    other_num++;    break;
                    case '}':   table_insert("}"," ");    other_num++;    break;
                    case '.':   table_insert("MAO",".");  op_num++;       break;
                    case '~':   table_insert("BOP","~");  op_num++;       break;
                    case '+':   state = 8;    break;
                    case '-':   state = 9;    break;
                    case '*':   state = 10;   break;
                    case '/':   state = 11;   break;
                    case '%':   state = 13;   break;
                    case '=':   state = 14;   break;
                    case '!':   state = 15;   break;
                    case '>':   state = 16;   break;
                    case '<':   state = 17;   break;
                    case '&':   state = 18;   break;
                    case '|':   state = 19;   break;
                    case '^':   state = 20;   break;
                    case '?':   state = 21;   break;
                    case ':':   table_insert(":"," ");  other_num++;   break;
                    case '#':   state = 22;   break;
                    case '\'':  string_flag = '\'';    state = 23;    break;
                    case '\"':  string_flag = '\"';    state = 23;    break;
                    case '\\':  state = 24;   break;
                    default: 
                        if(is_letter() || C == '_') {
                            state = 1;
                            break;    
                        }
                        else if(is_digit()) {
                            state = 2;
                            break;
                        }
                        if(C == '\0') {
                            return ;
                        }
                        if(C == '\n' || C == '\t' || C == ' ') {
                            break;
                        }
                        error("Invalid character");
                        break;
                }
                break;
            }
            case 1: {//标识符或关键字
                cat();
                get_char();
                while(is_letter() || is_digit() || C == '_') {
                    cat();
                    get_char();
                    if(end) break;
                }
                retract();
                state = 0;
                if(is_reserved(token)) {
                    table_insert("KEY",token);
                    key_num++;
                }
                else {
                    string* attr = symbol_insert(token);
                    id_num++;
                    table_insert("ID",*attr);
                }
                break;
            }
            case 2: {//整数
                cat();
                get_char();
                switch (C) {
                    case '.':   state = 3;    break;
                    case 'E':
                    case 'e':    state = 5;    break;
                    //处理十六进制
                    case 'x':
                    case 'X':
                        if(token.length() == 1 && token[0] == '0') {
                            state = 25;
                            break;
                        }
                        else {
                            error("Invalid hex number");
                            state = 0;
                            break;
                        }
                    default:
                        if(token.length() == 1 && token[0] == '0') {
                            state = 26;
                            break;
                        }
                        if(is_digit()) {
                            state = 2;
                            break;
                        }
                        retract();
                        state = 0;
                        int_num++;
                        table_insert("NUM",token);
                }
                break;
            }
            case 3: {//小数点
                cat();
                get_char();
                if(is_digit()) {
                    state = 4;
                }
                else {
                    error("Invalid float number");
                    state = 0;
                }
                break;
            }
            case 4: {//小数
                cat();
                get_char();
                switch (C) {
                    case 'E':
                    case 'e':   state = 5;    break;
                    default:
                        if(is_digit()) {
                            state = 4;
                            break;
                        }
                        else {
                            retract();
                            state = 0;
                            float_num++;
                            table_insert("NUM",token);
                            break;
                        }
                }
                break;
            }
            case 5: {//科学计数法
                cat();
                get_char();
                switch (C) {
                    case '+':
                    case '-':   state = 6;    break;
                    default:
                        if(is_digit()) {
                            state = 7;
                            break;
                        }
                        else {
                            error("Invalid float number");
                            retract();
                            state = 0;
                            break;
                        }
                }
                break;
            }
            case 6: {//符号
                cat();
                get_char();
                if(is_digit()) {
                    state = 7;
                }
                else {
                    error("Invalid float number");
                    retract();
                    state = 0;
                }
                break;
            }
            case 7: {//返回无符号数
                cat();
                get_char();
                if(is_digit()) {
                    state = 7;
                }
                else {
                    retract();
                    state = 0;
                    float_num++;
                    table_insert("NUM",token);
                }
                break;
            }
            case 8: {//+
                cat();
                get_char();
                switch (C) {
                    case '+':
                        cat();
                        state = 0;
                        table_insert("IOP",token);
                        op_num++;
                        break;
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP",token);
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","+");
                        op_num++;
                        break;
                }
                break;
            }
            case 9: {//-
                cat();
                get_char();
                switch (C) {
                    case '-':
                        cat();
                        state = 0;
                        table_insert("DOP",token);
                        op_num++;
                        break;
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP",token);
                        op_num++;
                        break;
                    case '>':
                        cat();
                        state = 0;
                        table_insert("AROP","->");
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","-");
                        op_num++;
                        break;
                }
                break;
            }
            case 10: {//*
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP",token);
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","*");
                        op_num++;
                        break;
                }
                break;
            }
            case 11: {///
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP",token);
                        op_num++;
                        break;
                    case '/':
                        comment_state = 1;
                        state = 12;
                        break;
                    case '*':
                        comment_state = 2;
                        state = 12;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","/");
                        op_num++;
                        break;
                }
                break;
            }
            case 12: {//注释
                get_char();
                //行注释
                if(comment_state == 1 && C == '\n') {
                    comment_state = 0;
                    state = 0;
                    break;
                }
                //块注释
                if(comment_state == 2) {
                    if(C == '*') {
                        get_char();
                        if(C == '/') {
                            comment_state = 0;
                            state = 0;
                            break;
                        }
                        else 
                            retract();
                    }
                    if(end) {
                        error("Unclosed comment");
                        state = 0;
                        return;
                    }
                }
                break;
            }
            case 13: {//%
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP",token);
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","%");
                        op_num++;
                        break;
                }
                break;
            }
            case 14: {//=
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("ROP","==");
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("AOP","assign-op");
                        op_num++;
                        break;
                }
                break;
            } 
            case 15: {//!
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("ROP","!=");
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("LOP","!");
                        op_num++;
                        break;
                }
                break;
            }
            case 16: {//>
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("ROP",">=");
                        op_num++;
                        break;
                    case '>':
                        cat();
                        get_char();
                        switch (C) {
                            case '=':
                                cat();
                                state = 0;
                                table_insert("AOP",">>=");
                                op_num++;
                                break;
                            default:
                                retract();
                                state = 0;
                                table_insert("BOP",">>");
                                op_num++;
                                break;
                        }
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("ROP",">");
                        op_num++;
                        break;
                }
                break;
            }
            case 17: {//<
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("ROP","<");
                        op_num++;
                        break;
                    case '<':
                        cat();
                        get_char();
                        switch (C) {
                            case '=':
                               cat();
                               state = 0;
                               table_insert("AOP","<<=");
                               op_num++;
                               break;
                            default:
                                retract();
                                state = 0;
                                table_insert("BOP","<<");
                                op_num++;
                                break;
                        }
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("ROP","<");
                        op_num++;
                        break;
                }
                break;
            }
            case 18: {//&
                cat();
                get_char();
                switch (C) {
                    case '&':
                        cat();
                        state = 0;
                        table_insert("LOP","&&");
                        op_num++;
                        break;
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP","&=");
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("BOP","&");
                        op_num++;
                        break; 
                }
                break;
            }
            case 19: {//|
                cat();
                get_char();
                switch (C) {
                    case '|':
                        cat();
                        state = 0;
                        table_insert("LOP","||");
                        op_num++;
                        break;
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP","|=");op_num++;
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("BOP","|");
                        op_num++;
                        break;
                }
                break;
            }
            case 20: {//^
                cat();
                get_char();
                switch (C) {
                    case '=':
                        cat();
                        state = 0;
                        table_insert("AOP","^=");
                        op_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("BOP","^");
                        op_num++;
                        break;
                }
                break;
            }
            case 21: {//?
                cat();
                get_char();
                switch (C) {
                    case ':':
                        cat();
                        state = 0;
                        table_insert("?:"," ");
                        other_num++;
                        break;
                    default:
                        retract();
                        state = 0;
                        table_insert("?"," ");
                        other_num++;
                        break;
                }
                break;
            }
            case 22: {//#直接跳过
                get_char();
                while(C != '\n') {
                    get_char();
                    if(end) {
                        state = 0;
                        return ;
                    }
                }
                state = 0;
                break;
            }
            case 23: {//字符及字符串
                cat();
                get_char();
                if(C == '\\') {
                    state = 24;
                    char_flag = 1;
                    break;
                }
                if(C == string_flag) {
                    cat();
                    state = 0;
                    if(string_flag == '\"') {
                        table_insert("STRING",token);
                        other_num++;
                    }
                    else { 
                        table_insert("CHAR",token);
                        other_num++;
                    }
                }
                else if(C == '\n') {
                    retract();
                    error("Invalid string or char");
                    state = 0;
                }
                if(end) {
                    error("Unclosed string or char");
                    state = 0;
                }
                break;
            }
            case 24: {//转义字符
                cat();
                get_char();
                if(C == '\n') {
                    table_insert("ESCAPE","\\");
                    state = 0;
                    break;
                }// 行拼接
                if(end) {
                    error("Unclosed escape character");
                    state = 0;
                }
                switch(C) {
                    case 'a': table_insert("ESCAPE","\\a"); other_num++; state = 0; break;
                    case 'b': table_insert("ESCAPE","\\b"); other_num++; state = 0; break;
                    case 'f': table_insert("ESCAPE","\\f"); other_num++; state = 0; break;
                    case 'n': table_insert("ESCAPE","\\n"); other_num++; state = 0; break;
                    case 'r': table_insert("ESCAPE","\\r"); other_num++; state = 0; break;
                    case 't': table_insert("ESCAPE","\\t"); other_num++; state = 0; break;
                    case 'v': table_insert("ESCAPE","\\v"); other_num++; state = 0; break;
                    case '\\': table_insert("ESCAPE","\\\\"); other_num++; state = 0; break;
                    case '\'': table_insert("ESCAPE","\\\'"); other_num++; state = 0; break;
                    case '\"': table_insert("ESCAPE","\\\""); other_num++; state = 0; break;
                    default:
                        if(char_flag != 1)
                            retract();
                        error("Invalid escape character");
                        state = 0;
                        break;
                }
                if(char_flag == 1) {
                    state = 23;
                    char_flag = 0;
                            
                }
                break;
            }
            case 25: {//十六进制
                cat();
                get_char();
                bool flag = false;
                while((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f') || (C >= 'A' && C <= 'F')) {
                    cat();
                    get_char();
                    if(end) break;
                    flag = true;
                }
                if(flag) {
                    retract();
                    state = 0;
                    int_num++;
                    table_insert("NUM",token);
                }
                else {
                    error("Invalid hex number");
                    state = 0;
                }
                break;
            }
            case 26: {//八进制
                cat();
                get_char();
                bool flag = false;
                while(C >= '0' && C <= '7') {
                    cat();
                    get_char();
                    if(end) break;
                    flag = true;
                }
                if(flag) {
                    retract();
                    state = 0;
                    int_num++;
                    table_insert("NUM",token);
                }
                else {
                    error("Invalid octal number");
                    state = 0;
                }
                break;
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    string file_name;
    if(argc == 1) {
        cout << "Please input the file name: ";
        cin >> file_name;
    }
    else {
        file_name = argv[1];
    }
    LexicalAnalysis LE(file_name);
    LE.run();
    LE.output();
    return 0;
}
    
