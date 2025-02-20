#include <iostream>
#include <vector>
#include <iomanip>
using namespace std;

vector<vector<int>> schedule; // 比赛日程表
vector<vector<bool>> used;    // 记录两人是否已经比赛过
int n, totalDays;

// 检查某天两个选手是否可以比赛
bool canPlay(int p1, int p2, int day) {
    return !used[p1][p2] && !used[p2][p1] && schedule[p1][day] == 0 && schedule[p2][day] == 0;
}

bool assignMatch(int day, int pairIndex) {
if (day >= totalDays) return true;
    if (pairIndex >= n / 2) {
        return assignMatch(day + 1, 0);
    }

    // 尝试为当前天分配比赛对
    for (int p1 = 1; p1 <= n; p1++) {
        if (schedule[p1][day] != 0) continue;

        for (int p2 = p1 + 1; p2 <= n; p2++) {
            if (!canPlay(p1, p2, day)) continue;

            // 分配比赛
            schedule[p1][day] = p2;
            schedule[p2][day] = p1;
            used[p1][p2] = used[p2][p1] = true;

            // 递归分配下一对
            if (assignMatch(day, pairIndex + 1)) return true;

            // 回溯
            schedule[p1][day] = 0;
            schedule[p2][day] = 0;
            used[p1][p2] = used[p2][p1] = false;
        }
    }
    return false;
}

void printSchedule(bool isOdd) {
    if(isOdd) {
        for (int i = 1; i <= n - 1; i++) {
            cout << setw(3) << i << "  ";
            for (int j = 0; j < totalDays; j++) {
                if(schedule[i][j] == n) {
                    cout << setw(3) << "0" << "  "; 
                } else {
                    cout << setw(3) << schedule[i][j] << "  ";
                }
            }
            cout << endl;
        }
    }
    else {
        for (int i = 1; i <= n; i++) {
            cout << setw(3) << i << "  ";
            for (int j = 0; j < totalDays; j++) {
                cout << setw(3) << schedule[i][j] << "  ";
            }
            cout << endl;
        }
    }
}

int main() {
    cin >> n;
    bool isOdd = (n % 2 == 1);

    if (isOdd) n++; // 引入虚拟选手
    totalDays = n - 1;

    // 初始化数据结构
    schedule = vector<vector<int>>(n + 1, vector<int>(totalDays, 0));
    used = vector<vector<bool>>(n + 1, vector<bool>(n + 1, false));

    if (assignMatch(0, 0)) {
        printSchedule(isOdd);
    } else {
        cout << "No solution" << endl;
    }

    return 0;
}
