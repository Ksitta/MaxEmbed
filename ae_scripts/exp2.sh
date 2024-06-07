# /bin/bash

rep_ratios=(0 0.1 0.2 0.4 0.8)
if [ ! -d exp2 ]; then
    mkdir -p exp2
    cd exp2

    mkdir -p result_alibaba_algo
    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/alibaba/alibaba_bin.test ${dataset_path}/alibaba/mapping/ME_16_${rep_ratio}.bin -c 0.1 -n 64 -b 16 -d 64 -t 20 > result_alibaba_algo/ME_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/alibaba/alibaba_bin.test ${dataset_path}/alibaba/mapping/RPP_16_${rep_ratio}.bin -c 0.1 -n 64 -b 16 -d 64 -t 20 > result_alibaba_algo/RPP_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/alibaba/alibaba_bin.test ${dataset_path}/alibaba/mapping/FPR_16_${rep_ratio}.bin -c 0.1 -n 64 -b 16 -d 64 -t 20 > result_alibaba_algo/FPR_16_${rep_ratio}_0.1.txt
    done

    mkdir -p result_avazu_algo
    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/avazu/avazu_bin.test ${dataset_path}/avazu/mapping/ME_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 0 > result_avazu_algo/ME_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/avazu/avazu_bin.test ${dataset_path}/avazu/mapping/RPP_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 0 > result_avazu_algo/RPP_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/avazu/avazu_bin.test ${dataset_path}/avazu/mapping/FPR_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 0 > result_avazu_algo/FPR_16_${rep_ratio}_0.1.txt
    done

    mkdir -p result_amazon_m2_algo
    for rep_ratio in ${rep_ratios[@]}
    do
        sudo ${client_path} ${dataset_path}/amazon_m2/amazon_m2_bin.test ${dataset_path}/amazon_m2/mapping/ME_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 20 > result_amazon_m2_algo/ME_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/amazon_m2/amazon_m2_bin.test ${dataset_path}/amazon_m2/mapping/RPP_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 20 > result_amazon_m2_algo/RPP_16_${rep_ratio}_0.1.txt
        sudo ${client_path} ${dataset_path}/amazon_m2/amazon_m2_bin.test ${dataset_path}/amazon_m2/mapping/FPR_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 20 > result_amazon_m2_algo/FPR_16_${rep_ratio}_0.1.txt
    done

    cd ..
fi