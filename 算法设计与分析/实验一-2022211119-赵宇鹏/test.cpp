#include "SortData.h"

void merge_sort_wrapper(int a[], int n, long long &comparisons, long long &moves) {
    merge_sort(a, 0, n - 1, comparisons, moves);
}

void quick_sort_wrapper(int a[], int n, long long &comparisons, long long &moves) {
    quick_sort(a, 0, n - 1, comparisons, moves);
}

void quick_sort_3way_wrapper(int a[], int n, long long &comparisons, long long &moves) {
    quick_sort_3way(a, 0, n - 1, comparisons, moves);
}

void quick_sort_medians_wrapper(int a[], int n, long long &comparisons, long long &moves) {
    quick_sort_medians(a, 0, n - 1, comparisons, moves);
}

bool isSorted(const vector<int>& data) {
    return std::is_sorted(data.begin(), data.end());
}
void runTest(void (*sortFunc)(int*, int, long long&, long long&), vector<int> data, const string& sortName) {
    long long comparisons = 0, moves = 0;
    int n = data.size();
    int* arr = data.data();
    auto start = high_resolution_clock::now();
    sortFunc(arr, n, comparisons, moves);
    auto end = high_resolution_clock::now();
    duration<double> execTime = duration_cast<duration<double>>(end - start);

    bool sorted = isSorted(data);
    cout << setw(15) << sortName << " | "
         << setw(10) << n << " | "
         << setw(10) << execTime.count() << " s | "
         << setw(12) << comparisons << " | "
         << setw(10) << moves  << "|"
         << (sorted ? "Correct" : "Incorrect") << endl;
}


int main() {
    vector<int> sizes = {100, 5000, 10000, 20000}; // 数据集大小
    vector<int> types = {0, 1, 2, 3, 4}; // 0：随机数据，1：有序数据，2：逆序数据，3：近乎有序，4：所有元素相等
    vector<string> typeNames = {"Random", "Sorted", "Reversed", "Nearly Sorted", "All Equal"};
    
    cout << "Sort Algorithm | Size       | Time          | Comparisons   | Moves" << endl;
    cout << "--------------------------------------------------------------" << endl;

    for (int size : sizes) {
        for (int type : types) {
            vector<int> data(size);
            generateData(data, type);
            cout << "Data Type: " << typeNames[type] << " | Size: " << size << endl;
            runTest(heap_sort, data, "Heap Sort    ");
            runTest(merge_sort_wrapper, data, "Merge Sort    ");
            runTest(quick_sort_wrapper, data, "Quick Sort    ");
            runTest(quick_sort_3way_wrapper, data, "3-Way Quick Sort ");
            runTest(quick_sort_medians_wrapper, data, "Median Quick Sort");
            cout << "--------------------------------------------------------------" << endl;
        }
    }

    return 0;
}