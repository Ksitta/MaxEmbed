# /bin/bash

rep_ratios=(0 0.1 0.2 0.4 0.8)
cache_ratios=(0.01 0.02 0.03 0.05 0.08 0.1 0.2 0.4)


mkdir -p result_alibaba

for rep_ratio in ${rep_ratios[@]}
do
for cache_ratio in ${cache_ratios[@]} 
do
    sudo ${client_path} ${dataset_path}/alibaba/alibaba_bin.test ${dataset_path}/alibaba/mapping/ME_16_${rep_ratio}.bin -c ${cache_ratio} -n 64 -b 16 -d 64 -t 20 > result_alibaba/ME_16_${rep_ratio}_${cache_ratio}.txt
done
done

mkdir -p result_avazu

for rep_ratio in ${rep_ratios[@]}
do
for cache_ratio in ${cache_ratios[@]} 
do
    sudo ${client_path} ${dataset_path}/avazu/avazu_bin.test ${dataset_path}/avazu/mapping/ME_16_${rep_ratio}.bin -c ${cache_ratio} -n 64 -b 32 -d 64 -t 0 > result_avazu/ME_16_${rep_ratio}_${cache_ratio}.txt
done
done

mkdir -p result_criteo

for rep_ratio in ${rep_ratios[@]}
do
for cache_ratio in ${cache_ratios[@]} 
do
    sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_${rep_ratio}.bin -c ${cache_ratio} -n 64 -b 32 -d 64 -t 20 > result_criteo/ME_16_${rep_ratio}_${cache_ratio}.txt
done
done

mkdir -p result_criteoTB

for rep_ratio in ${rep_ratios[@]}
do
for cache_ratio in ${cache_ratios[@]} 
do
    sudo ${client_path} ${dataset_path}/criteoTB/criteoTB_bin.test ${dataset_path}/criteoTB/mapping/ME_16_${rep_ratio}.bin -c ${cache_ratio} -n 64 -b 32 -d 64 -t 40 > result_criteoTB/ME_16_${rep_ratio}_${cache_ratio}.txt
done
done

mkdir -p result_amazon_m2

for rep_ratio in ${rep_ratios[@]}
do
sudo ${client_path} ${dataset_path}/amazon_m2/amazon_m2_bin.test ${dataset_path}/amazon_m2/mapping/ME_16_${rep_ratio}.bin -c 0.1 -n 64 -b 32 -d 64 -t 20 > result_amazon_m2/ME_16_${rep_ratio}_0.1.txt
done