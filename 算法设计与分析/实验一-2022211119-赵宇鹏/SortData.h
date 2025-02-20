#include "SortAlgorithm.h"
void generateData(vector<int>& data, int type) {
    static random_device rd;
    static mt19937 gen(rd());
    int n = data.size();
    if (type == 0) { // Random data
        uniform_int_distribution<> dist(1, 1000000);
        generate(data.begin(), data.end(), [&]() { return dist(gen); });
    } else if (type == 1) { // Sorted data
        iota(data.begin(), data.end(), 0);
    } else if (type == 2) { // Reversed data
        iota(data.rbegin(), data.rend(), 0);
    } else if (type == 3) { // Nearly sorted data
        iota(data.begin(), data.end(), 0);
        shuffle(data.begin(), data.begin() + n / 10, gen);
    } else if (type == 4) { // All equal data
        int equal_value = 42; 
        fill(data.begin(), data.end(), equal_value);
    }
}