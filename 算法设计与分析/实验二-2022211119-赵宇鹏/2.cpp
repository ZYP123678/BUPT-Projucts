#include <stdio.h>
#include <iostream>
using namespace std;

int schedule[20000][20000];

void generateScheduleMatrix(int n) {
    int isOdd = n % 2;
    int m = isOdd ? n + 1 : n;// 若为奇数，则相当于多一个虚拟选手

    for (int day = 0; day < m - 1; ++day) {
        for (int i = 0; i < m / 2; ++i) {
            int p1 = (day + i) % (m - 1);
            int p2 = (m - 1 - i + day) % (m - 1);
            if (i == 0) p2 = m - 1; 

            if (p1 < n && p2 < n) {
                schedule[p1][day] = p2 + 1;
                schedule[p2][day] = p1 + 1;
            }
        }
    }

}

void print(int n) {
    int isOdd = n % 2;
    int m = isOdd ? n + 1 : n;
    for (int i = 0; i < n; ++i) {
        printf("%3d ", i + 1); 
        for (int j = 0; j < m - 1; ++j) {
            if(schedule[i][j] == 0) {
                printf("%3d ", 0);
            } else {
                printf("%3d ", schedule[i][j]);
            }
        }
        printf("\n");
    }
}

int main() {
    int n;
    scanf("%d", &n);
    generateScheduleMatrix(n);
    //print(n);
    return 0;
}
