#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <random>
#include <iomanip>
using namespace std;
using namespace std::chrono;

//堆调整
void heapify(int *a, int n, int i, long long &comparions, long long &moves) {
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;
    comparions++;
    if(l < n) {
        comparions++;
        if(a[l] > a[largest]) {
            largest = l;
        }
    }
    comparions++;
    if(r < n) {
        comparions++;
        if(a[r] > a[largest]) {
            largest = r;
        }
    }
    comparions++;
    if(largest != i) {
        swap(a[i], a[largest]);
        moves++;
        heapify(a, n, largest, comparions, moves);
    }
}
//堆排序
void heap_sort(int *a, int n, long long &comparisons, long long &moves) {
    for (int i = n / 2 - 1; i >= 0; i--) {
        comparisons++;
        heapify(a, n, i, comparisons, moves);
    }

    for (int i = n - 1; i > 0; i--) {
        comparisons++;
        swap(a[0], a[i]);
        moves++;
        heapify(a, i, 0, comparisons, moves);
    }
}

//归并排序
void merge(int *a, int l, int m, int r, long long &comparisons, long long &moves) {
    int n1 = m - l + 1;
    int n2 = r - m;
    int L[n1], R[n2];
    for(int i = 0; i < n1; i++) {
        comparisons++;
        moves++;
        L[i] = a[l + i];
    }
    for(int i = 0; i < n2; i++) {
        comparisons++;
        moves++;
        R[i] = a[m + 1 + i];
    }
    int i = 0, j = 0, k = l;
    while(i < n1 && j < n2) {
        comparisons += 2;
        comparisons++;
        if(L[i] <= R[j]) {
            a[k] = L[i];
            i++;
        } else {
            a[k] = R[j];
            j++;
        }
        moves++;
        k++;
    }
    while(i < n1) {
        comparisons++;
        a[k] = L[i];
        moves++;
        i++;
        k++;
    }
    while(j < n2) {
        comparisons++;
        a[k] = R[j];
        moves++;
        j++;
        k++;
    }
}

void merge_sort(int *a, int l, int r, long long &comparisons, long long &moves) {
    if(l < r) {
        comparisons++;
        int m = l + (r - l) / 2;
        merge_sort(a, l, m, comparisons, moves);
        merge_sort(a, m + 1, r, comparisons, moves);
        merge(a, l, m, r, comparisons, moves);
    }
}

//普通快排
int partition(int *a, int low, int high, long long &comparisons, long long &moves) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(low, high);
    int randomIndex = dist(gen);    
    swap(a[randomIndex], a[high]);  
    moves++;
    int pivot = a[high];
    int i = low - 1;
    for(int j = low; j < high; j++) {
        comparisons += 2;
        if(a[j] < pivot) {
            i++;
            swap(a[i], a[j]);
            moves++;
        }
    }
    swap(a[i + 1], a[high]);
    moves++;
    return i + 1;
}

void quick_sort(int *a, int low, int high, long long &comparisons, long long &moves) {
    if(low < high) {
        comparisons++;
        int pi = partition(a, low, high, comparisons, moves);
        quick_sort(a, low, pi - 1, comparisons, moves);
        quick_sort(a, pi + 1, high, comparisons, moves);
    }
}

//三路快排
void quick_sort_3way(int *a, int low, int high, long long &comparisons, long long &moves) {
    if(low >= high) {
        comparisons++;
        return;
    }
    int lt = low, gt = high;
    int i = low;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(low, high);
    int randomIndex = dist(gen);    
    swap(a[randomIndex], a[high]);  
    moves++;
    int pivot = a[high];
    while(i <= gt) {
        comparisons += 2;
        if(a[i] < pivot) {
            swap(a[lt], a[i]);
            moves++;
            lt++;
            i++;
        } else if(a[i] > pivot) {
            swap(a[i], a[gt]);
            moves++;
            gt--;
        } else {
            i++;
        }
    }
    quick_sort_3way(a, low, lt - 1, comparisons, moves);
    quick_sort_3way(a, gt + 1, high, comparisons, moves);
}

// 首尾中数三数取中法找到 pivot
int medianOfThree(int *a, int low, int high, long long &comparisons) {
    int mid = low + (high - low) / 2;
    comparisons += 3;
    if (a[high] < a[low]) swap(a[low], a[high]);
    if (a[mid] < a[low]) swap(a[low], a[mid]);
    if (a[high] < a[mid]) swap(a[mid], a[high]);
    swap(a[mid], a[high]);
    return a[high]; // 返回枢轴
}

// 分区函数
int partition_of_three(int *a, int low, int high, long long &comparisons, long long &moves) {
    int pivot = a[high];
    int i = low - 1;
    for (int j = low; j <= high - 1; j++) {
        comparisons++;
        if (a[j] < pivot) {
            i++;
            swap(a[i], a[j]);
            moves++;
        }
    }
    swap(a[i + 1], a[high]);
    moves++;
    return i + 1;
}

// 三数取中法的快速排序
void quick_sort_medians(int *a, int low, int high, long long &comparisons, long long &moves) {
    while (low < high) {
        comparisons++;
        int pivot = medianOfThree(a, low, high, comparisons);
        int pi = partition_of_three(a, low, high, comparisons, moves);

        // 选择较小部分递归，优化尾递归
        if (pi - low < high - pi) {
            quick_sort_medians(a, low, pi - 1, comparisons, moves);
            low = pi + 1;
        } else {
            quick_sort_medians(a, pi + 1, high, comparisons, moves);
            high = pi - 1;
        }
    }
}