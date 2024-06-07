#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "partition.h"
#include <cassert>
using namespace std;


int main(int argc, char*argv[]){
    if (argc != 4){
        cout << "Usage: " << argv[0] << " <input_file> <output_file> <rep_ratio>" << endl;
        return 1;
    }
    int part_cnt;
    FILE *output = fopen(argv[2], "wb");
    double tar_rep_ratio = std::stod(argv[3]);

    auto map_info = read_mapping(argv[1], part_cnt);

    int tot_cnt = 0, node_cnt = map_info.vertex_cnt;
    unordered_map<int, vector<int>> part2node;

    for(int i = 0; i < map_info.vertex_cnt; i++){
        for(int j = 0; j < map_info.cnt[i]; j++){
            int part = map_info.mapping[map_info.idx[i] + j];
            part2node[part].push_back(i);
            tot_cnt++;
        }
    }
    
    assert(tot_cnt == map_info.total_cnt);
    double rep_ratio = (double)(tot_cnt - map_info.vertex_cnt) / map_info.vertex_cnt;
    int part2node_size = part2node.size();
    int max_part_size = 0;


    printf("replication ratio: %lf\n", rep_ratio);
    printf("node cnt %d\n", node_cnt);
    printf("tot cnt %d\n", tot_cnt);
    printf("part cnt %d\n", part2node_size);
    printf("avg part size: %lf\n", (double)tot_cnt / part2node_size);

    bool part_id_consecutive = true;

    for(auto &each :part2node){
        if(each.first < 0 || each.first >= part2node_size){
            part_id_consecutive = false;
        }
        max_part_size = max(max_part_size, (int)each.second.size());
    }
    
    printf("max part size: %d\n", max_part_size);

    printf("part id consecutive: %d\n", part_id_consecutive);
    int tar_total_cnt = node_cnt * (1 + tar_rep_ratio);
    int tar_part_cnt = (tar_total_cnt + (max_part_size - 1)) / max_part_size;
    std::vector<int> new_idx;
    std::vector<int> new_cnt;
    std::vector<int> new_mapping;
    for(int i = 0; i < node_cnt; i++){
        new_idx.push_back(new_mapping.size());
        int cnt = 0;
        for(int j = 0; j < map_info.cnt[i]; j++){
            int part = map_info.mapping[map_info.idx[i] + j];
            if(part < tar_part_cnt){
                new_mapping.push_back(part);
                cnt++;
            }
        }
        new_cnt.push_back(cnt);
    }
    uint64_t new_node_cnt = node_cnt;
    uint64_t new_total_cnt = new_mapping.size();
    fwrite(&new_node_cnt, sizeof(uint64_t), 1, output);
    fwrite(&new_total_cnt, sizeof(uint64_t), 1, output);
    fwrite(new_idx.data(), sizeof(int), new_idx.size(), output);
    fwrite(new_cnt.data(), sizeof(int), new_cnt.size(), output);
    fwrite(new_mapping.data(), sizeof(int), new_mapping.size(), output);
    fclose(output);
    munmap(map_info.raw_ptr, map_info.file_size);
    return 0;
}