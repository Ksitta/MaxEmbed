# /bin/bash

rep_ratios=(0 0.1 0.2 0.4 0.8)

if [ ! -d exp3 ]; then
    mkdir -p exp3
    cd exp3

    mkdir -p result_criteo_no_cache

    for rep_ratio in ${rep_ratios[@]}
    do
    sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_${rep_ratio}.bin -c 0 -n 64 -b 32 -d 64 -t 20 > result_criteo_no_cache/ME_16_${rep_ratio}_0.txt
    done

    mkdir -p result_avazu_no_cache

    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/avazu/avazu_bin.test ${dataset_path}/avazu/mapping/ME_16_${rep_ratio}.bin -c 0 -n 64 -b 32 -d 64 -t 0 > result_avazu_no_cache/ME_16_${rep_ratio}_0.txt
    done

    mkdir -p result_alibaba_no_cache

    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/alibaba/alibaba_bin.test ${dataset_path}/alibaba/mapping/ME_16_${rep_ratio}.bin -c 0 -n 64 -b 16 -d 64 -t 20 > result_alibaba_no_cache/ME_16_${rep_ratio}_0.txt
    done

    mkdir -p result_criteoTB_no_cache

    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/criteoTB/criteoTB_bin.test ${dataset_path}/criteoTB/mapping/ME_16_${rep_ratio}.bin -c 0 -n 64 -b 32 -d 64 -t 40 > result_criteoTB_no_cache/ME_16_${rep_ratio}_0.txt
    done

    cd ..
fi
