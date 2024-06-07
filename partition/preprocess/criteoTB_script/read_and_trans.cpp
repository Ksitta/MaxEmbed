#include <iostream>
#include <stdio.h>
#include <chrono>
#include <assert.h>

inline int getnumber(char ch){
    if(ch >= '0' && ch <= '9'){
        return ch - '0';
    }
    if(ch >= 'a' && ch <= 'f'){
        return ch - 'a' + 10;
    }
    return 0;
}

void process_line(char *line, FILE *fout){
    uint64_t numbers[26];
    char ch;
    int cnt = 0;
    int pos = 0;
    while(true){
        ch = line[pos];
        pos++;
        if(ch == '\t'){
            cnt++;
            if(cnt == 14){
                break;
            }
        }
    }
    uint64_t number = 0;
    uint64_t number_cnt = 0;
    while(true){
        ch = line[pos];
        if(ch == '\t' || ch =='\n'){
            if(number != 0){
                numbers[number_cnt] = number;
                number_cnt++;
            }
            if(ch == '\n'){
                break;
            }
            number = 0;
            pos++;
            continue;
        }
        number *= 16;
        number += getnumber(ch);
        pos++;
    }
    fwrite(&number_cnt, sizeof(uint64_t), 1, fout);
    fwrite(numbers, sizeof(uint64_t), number_cnt, fout);
}

int main(int argc, char **argv){
    if(argc != 3){
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "wb");
    assert(fin != nullptr);
    assert(fout != nullptr);
    uint64_t rec_len = 0;
    fwrite(&rec_len, sizeof(rec_len), 1, fout);
    char *line = nullptr;
    size_t len;
    auto now = std::chrono::system_clock::now();
    while(true){
        getline(&line, &len, fin);
        if(feof(fin)){
            break;
        }
        process_line(line, fout);
        rec_len++;
    }
    auto end = std::chrono::system_clock::now();
    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count() << std::endl;
    fseek(fout, 0, SEEK_SET);
    fwrite(&rec_len, sizeof(uint64_t), 1, fout);
    fclose(fin);
    fclose(fout);
    return 0;
}