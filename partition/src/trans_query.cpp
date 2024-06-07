#include <iostream>
#include <fstream>
#include <vector>

using size_type = uint64_t;
using data_type = uint32_t;

int main(int argc, char **argv){
    if(argc < 3){
        std::cout << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
    FILE *fin = fopen(argv[1], "r");
    if(fin == NULL){
        std::cout << "File not found" << std::endl;
        return 1;
    }
    FILE *fout = fopen(argv[2], "wb");
    if(fout == NULL){
        std::cout << "Open failed" << std::endl;
        return 1;
    }
    char *line = (char *)malloc(1 << 20);
    size_t len = 0;
    std::vector<data_type> all_data;
    std::vector<size_type> index;
    size_type query_cnt = 0;
    index.push_back(0);
    getline(&line, &len, fin);
    int temp;
    size_type node_cnt;
    size_type record_len;
    size_type pins;
    sscanf(line, "%d%ld%ld%ld", &temp, &node_cnt, &record_len, &pins);
    while(getline(&line, &len, fin) != -1){
        data_type key;
        char *ptr = line; 
        while (sscanf(ptr, "%d", &key) > 0) {
            all_data.push_back(key);
            while (*ptr != ' ' && *ptr != '\0') {
                ptr++;
            }
            if (*ptr == ' ') {
                ptr++;
            }
        }
        index.push_back(all_data.size());
    }
    query_cnt = index.size() - 1;
    fwrite(&query_cnt, sizeof(size_type), 1, fout);
    fwrite(&node_cnt, sizeof(size_type), 1, fout);
    fwrite(index.data(), sizeof(size_type), index.size(), fout);
    fwrite(all_data.data(), sizeof(data_type), all_data.size(), fout);
    fclose(fin);
    fclose(fout);
    return 0;
}
