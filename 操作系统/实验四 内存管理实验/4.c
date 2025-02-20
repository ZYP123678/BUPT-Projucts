#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ADDRESS_COUNT 100
#define PAGE_SIZE 10    

void generate_memory_access_sequence(int *sequence) {
    srand(time(NULL));
    int current_address = 0;
    sequence[0] = current_address;

    for (int i = 1; i < ADDRESS_COUNT; i++) {
        int rand_prob = rand() % 100;

        if (rand_prob < 70) {
            current_address = (current_address + 1) % ADDRESS_COUNT;
        } else if (rand_prob < 80) {
            if (current_address > 0) {
                current_address = rand() % current_address;
            } else {
                current_address = 0;
            }
        } else {
            if (current_address < ADDRESS_COUNT - 1) {
                current_address = current_address + 1 + rand() % (ADDRESS_COUNT - current_address - 1);
            } else {
                current_address = ADDRESS_COUNT - 1;
            }
        }

        sequence[i] = current_address;
    }
}

void map_to_page_sequence(int *address_sequence, int *page_sequence) {
    for (int i = 0; i < ADDRESS_COUNT; i++) {
        page_sequence[i] = address_sequence[i] / PAGE_SIZE;
    }
}

void print_sequence(const char *label, int *sequence, int size) {
    printf("%s:\n", label);
    for (int i = 0; i < size; i++) {
        printf("%d ", sequence[i]);
        if ((i + 1) % 10 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int FIFO(int* page_sequence, int block_num) {
    int page_talbe[block_num];
    int page_fault = 0;
    int current_index = 0;

    for(int i = 0; i < block_num; i++) {
        page_talbe[i] = -1;
    }
    for(int i = 0; i < ADDRESS_COUNT; i++) {
        int hit = 0;
        for(int j = 0; j < block_num; j++) {
            if(page_talbe[j] == page_sequence[i]) {
                hit = 1;
                break;
            }
        }
        if(hit == 0) {
            page_fault++;
            page_talbe[current_index] = page_sequence[i];
            current_index = (current_index + 1) % block_num;
        }
    }
    return page_fault;
}

int LRU(int* page_sequence, int block_num) {
    int page_talbe[block_num];
    int time_stamp[block_num];
    int page_fault = 0;

    for(int i = 0; i < block_num; i++) {
        page_talbe[i] = -1;
        time_stamp[i] = 0;
    }
    for(int i = 0; i < ADDRESS_COUNT; i++) {    
        int hit = 0;
        int current_index = 0;
        int max_time = time_stamp[0];
        for(int j = 0; j < block_num; j++) {
            if(page_talbe[j] == -1) {
                current_index = j;
                break;
            }
            else if(page_talbe[j] == page_sequence[i]) {
                hit = 1;
                current_index = j;
                break;
            }
            if(time_stamp[j] > max_time) {
                max_time = time_stamp[j];
                current_index = j;
            }
        }
        for(int j = 0; j < block_num; j++) {
            if(j == current_index) {
                time_stamp[j] = 0;
            }
            else {
                if(page_talbe[j] != -1) {
                    time_stamp[j]++;
                }
            }
        }
        if(hit == 0) {
            page_fault++;
            page_talbe[current_index] = page_sequence[i];
        }
    }
    return page_fault;
}

int OPT(int* page_sequence, int block_num) {
    int page_talbe[block_num];
    int page_fault = 0;

    for(int i = 0; i < block_num; i++) {
        page_talbe[i] = -1;
    }

    for(int i = 0; i < ADDRESS_COUNT; i++) {
        int hit = 0;
        int index = -1;
        for(int j = 0; j < block_num; j++) {
            if(page_talbe[j] == -1) {
                index = j;
                break;
            }
            else if(page_talbe[j] == page_sequence[i]) {
                hit = 1;
                break;
            }
        }
        if(hit == 0) {
            page_fault++;
            int max = 0;
            if(index == -1) {
                for(int j = 0; j < block_num; j++) {
                    int k = i + 1;
                    int flag = 0;
                    while(k < ADDRESS_COUNT) {
                        if(page_talbe[j] == page_sequence[k]) {
                            if(k > max) {
                                max = k;
                                index = j;
                            }
                            flag = 1;
                            break;
                        }
                        k++;
                    }
                    if(flag == 0) {
                        index = j;
                        break;
                    }
                }
            } 

            page_talbe[index] = page_sequence[i];
        }
    }
    return page_fault;
}

int main() {

    int min_block, max_block;

    printf("Enter min block num: ");
    scanf("%d", &min_block);
    printf("Enter max block num: ");
    scanf("%d", &max_block);

    int address_sequence[ADDRESS_COUNT];
    int page_sequence[ADDRESS_COUNT];

    generate_memory_access_sequence(address_sequence);
    map_to_page_sequence(address_sequence, page_sequence);

    for (int block_num = min_block; block_num <= max_block; block_num++) {
        int page_fault_FIFO = FIFO(page_sequence, block_num);
        int page_fault_LRU = LRU(page_sequence, block_num);
        int page_fault_OPT = OPT(page_sequence, block_num);

        printf("Block Num: %d, FIFO: %d, LRU: %d, OPT: %d\n",
               block_num, page_fault_FIFO, page_fault_LRU, page_fault_OPT);
    }

    return 0;
}
