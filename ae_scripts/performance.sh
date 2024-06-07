echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
sudo sync
sudo sysctl -w vm.drop_caches=3
