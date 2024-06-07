# /bin/bash
mkdir -p cdf

sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_0.bin -c 0 -n 64 -b 32 -d 64 -t 20 -o cdf/shp.txt > /dev/null
sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_0.1.bin -c 0 -n 64 -b 32 -d 64 -t 20 -o cdf/me.txt > /dev/null
