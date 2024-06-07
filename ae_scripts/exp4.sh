# /bin/bash
if [ ! -d exp4 ]; then
    mkdir -p exp4
    cd exp4
    sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_0.bin -c 0 -n 64 -b 32 -d 64 -t 20 -o shp.txt > /dev/null
    sudo ${client_path} ${dataset_path}/criteo/criteo_bin.test ${dataset_path}/criteo/mapping/ME_16_0.1.bin -c 0 -n 64 -b 32 -d 64 -t 20 -o me.txt > /dev/null
    cd ..
fi