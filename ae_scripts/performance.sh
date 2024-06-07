echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null
sudo sync > /dev/null
sudo sysctl -w vm.drop_caches=3 > /dev/null
