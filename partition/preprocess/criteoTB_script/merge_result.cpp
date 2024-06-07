#include <iostream>
#include <stdio.h>
#include <chrono>
#include <assert.h>
#include <vector>
#include <unordered_map>

int main(int argc, char** argv){
    if(argc != 5){
        std::cout << "Usage: " << argv[0] << " <start> <end> <input_file_path> <output_file>" << std::endl;
        return 1;
    }
    int start_file = atoi(argv[1]);
    int end_file = atoi(argv[2]);
    FILE *fout = fopen(argv[4], "wb");
    std::string input_path = argv[3];
    std::vector<FILE*> files;
    std::vector<uint64_t> lens;
    for(int i = start_file; i < end_file; i++){
        FILE *f = fopen((input_path + std::to_string(i)).c_str(), "rb");
        files.push_back(f);
    }
    uint64_t total_rec = 0;
    uint64_t node_cnt = 0;
    for(auto each : files){
        uint64_t rec;
        fread(&rec, sizeof(uint64_t), 1, each);
        total_rec += rec;
        lens.push_back(rec);
    }
    fwrite(&total_rec, sizeof(uint64_t), 1, fout);
    fwrite(&node_cnt, sizeof(uint64_t), 1, fout);
    std::vector<uint64_t> all_index(total_rec + 1);
    fwrite(all_index.data(), sizeof(uint64_t), total_rec + 1, fout);
    std::unordered_map<uint64_t, uint32_t> key;
    uint64_t *buffer = new uint64_t[1 << 20];
    uint32_t *reindex = new uint32_t[1 << 20]; 
    uint64_t now_len = 0;
    uint64_t sum = 0;
    for(int i = 0; i < files.size(); i++){
        uint64_t len = lens[i];
        FILE *file = files[i];
        for(uint64_t j = 0; j < len; j++){
            if(now_len % 1000000 == 0){
                printf("now: %ld %lf\n", now_len, double(now_len) / total_rec);
            }
            uint64_t cnt;
            fread(&cnt, sizeof(uint64_t), 1, file);
            fread(buffer, sizeof(uint64_t), cnt, file);
            for(int k = 0; k < cnt; k++){
                uint32_t new_key;
                auto pos = key.find(buffer[k]);
                if(pos == key.end()){
                    new_key = node_cnt;
                    key[buffer[k]] = node_cnt;
                    node_cnt++;
                } else {
                    new_key = pos->second;
                }
                reindex[k] = new_key;
            }
            fwrite(reindex, sizeof(uint32_t), cnt, fout);
            sum += cnt;
            now_len++;
            all_index[now_len] = sum;
        }
    }
    fseek(fout, sizeof(uint64_t), SEEK_SET);
    fwrite(&node_cnt, sizeof(uint64_t), 1, fout);
    fwrite(all_index.data(), sizeof(uint64_t), total_rec + 1, fout);
    return 0;
}