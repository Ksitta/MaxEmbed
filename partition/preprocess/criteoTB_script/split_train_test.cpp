#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

int main(int argc, char *argv[]){
    if(argc != 4){
        std::cout << "Usage: " << argv[0] << " <input_file> <output_path> <reindex>" << std::endl;
        return 1;
    }
    std::string outpath = argv[2];
    FILE *fin = fopen(argv[1], "rb");
    bool is_reindex = std::stoi(argv[3]);
    FILE *fout_train = fopen((outpath + ".train").c_str(), "wb");
    FILE *fout_test = fopen((outpath + ".test").c_str(), "wb");
    if(fin == NULL || fout_train == NULL || fout_test == NULL){
        std::cout << "Error opening files" << std::endl;
        return 1;
    }
    size_t edge_count;
    size_t node_count;
    fread(&edge_count, 8, 1, fin);
    fread(&node_count, 8, 1, fin);
    std::vector<uint32_t> reindex(node_count);
    for(uint32_t i = 0; i < node_count; i++){
        reindex[i] = i;
    }
    std::mt19937 rng;
    rng.seed(1);
    std::shuffle(reindex.begin(), reindex.end(), rng);
    const size_t max_copy_size = 1000000;
    size_t train_len = edge_count * 0.8;
    size_t test_len = edge_count - train_len;
    fwrite(&train_len, 8, 1, fout_train);
    fwrite(&node_count, 8, 1, fout_train);
    
    uint64_t *buffer = new uint64_t[max_copy_size];
    
    size_t copy_size;
    while(train_len > 0){
        copy_size = train_len > max_copy_size ? max_copy_size : train_len;
        fread(buffer, 8, copy_size, fin);
        fwrite(buffer, 8, copy_size, fout_train);
        train_len -= copy_size;
    }
    size_t test_offset;
    size_t zero = 0;

    fread(&test_offset, 8, 1, fin);
    fwrite(&test_offset, 8, 1, fout_train);

    fwrite(&test_len, 8, 1, fout_test);
    fwrite(&node_count, 8, 1, fout_test);
    fwrite(&zero, 8, 1, fout_test);
    while(test_len > 0){
        size_t copy_size = test_len > max_copy_size ? max_copy_size : test_len;
        fread(buffer, 8, copy_size, fin);
        for(size_t i = 0; i < copy_size; i++){
            buffer[i] -= test_offset;
        }
        fwrite(buffer, 8, copy_size, fout_test);
        test_len -= copy_size;
    }

    delete [] buffer;
    uint32_t *data_buffer = new uint32_t[max_copy_size];
    size_t train_data_len = test_offset;
    while(train_data_len > 0){
        size_t copy_size = train_data_len > max_copy_size ? max_copy_size : train_data_len;
        fread(data_buffer, 4, copy_size, fin);
        if(is_reindex){
            for(size_t i = 0; i < copy_size; i++){
                data_buffer[i] = reindex[data_buffer[i]];
            }
        }
        fwrite(data_buffer, 4, copy_size, fout_train);
        train_data_len -= copy_size;
    }

    while(true){
        size_t read_size = fread(data_buffer, 4, max_copy_size, fin);
        if(read_size == 0){
            break;
        }
        if(is_reindex){
            for(size_t i = 0; i < copy_size; i++){
                data_buffer[i] = reindex[data_buffer[i]];
            }
        }
        fwrite(data_buffer, 4, read_size, fout_test);
    }

    delete[] data_buffer;

    fclose(fin);
    fclose(fout_train);
    fclose(fout_test);
    return 0;
}