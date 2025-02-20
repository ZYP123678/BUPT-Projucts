//网球循环赛
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
using namespace std;

const int MAX = 20000;

int a[MAX][MAX]; //a[i][j + 1]表示第i个人第j天比赛的对手
int b[MAX]; //b[i]表示第i个人的轮空对手

void print(int n) {
    //偶数时直接输出
    if(n % 2 == 0) {
        for(int i = 1; i <= n; i++) {
            for(int j = 1; j <= n; j++)
                cout << setw(3) << a[i][j] << "  ";
            cout << endl;
        }
        return;
    } else { //奇数时必有人轮空
        for(int i = 1; i <= n; i++) {
            for(int j = 1; j <= n + 1; j++){
                if(a[i][j] >= n + 1)    cout << setw(3) << 0 << "  ";
                else    cout << setw(3) << a[i][j] << "  ";
            }
            cout << endl;
        }
    }
}
//偶数时直接复制
void even(int n) {
    int m = n / 2;
    for(int i = 1; i <= m; i++) {
        for(int j = 1; j <= m; j++) {
            a[i + m][j + m] = a[i][j];//右下角
            a[i + m][j] = a[i][j] + m;//左下角
            a[i][j + m] = a[i + m][j];//右上角 
        }
    }
}
//奇数时把轮空的和后面的进行匹配
void odd(int n) {
    int m = n / 2;
    
    for(int i = 1; i <= m; i++) {
        b[i] = i + m;
        b[m + i] = i + m;//实现循环
    }
    for(int i = 1; i <= m; i++) {
        for(int j = 1; j <= m + 1; j++) {
            if(a[i][j] > m) {
                a[i][j] = b[i];
                a[m + i][j] = i;
            }
            else 
            a[m + i][j] = a[i][j] + m;//左下角
        }
        for(int j = 1; j <= m - 1; j++) {
            a[i][m + 1 + j] = b[i + j];//右上角   
            a[b[i + j]][m + j + 1] = i;//右下角
        }
    }
}

void decide(int n) {
    if(n / 2 > 1 && ((n / 2) % 2 == 1)) {
        odd(n);
    } else  even(n);
}

void merge(int n) {
    if(n == 1) {
        a[1][1] = 1;
        return;
    }
    if(n % 2 == 1) {
        merge(n + 1);
        return;
    }
    merge(n / 2);
    
    decide(n);
}


int main() {
    int n;
    cin >> n;
    merge(n);
    //print(n);
}