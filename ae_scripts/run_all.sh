#!/bin/bash

set -e

project_path=/home/frw/asplos_ae
spdk_path=/home/frw/spdk
dataset_path=/home/frw/dataset
client_path=${project_path}/build/client/client

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: $0 <path_to_log> [option]"
    echo ""
    echo "Options:"
    echo "1 - Run performance.sh and main_evaluation.sh"
    echo "2 - Run performance.sh and different_algorithm.sh"
    echo "3 - Run performance.sh and cdf.sh"
    echo "4 - Run performance.sh and no_cache.sh"
    echo "If the second option is not provided, all scripts will be run."
    echo ""
    echo "All figures will be generated in the provided log directory."
    echo ""
    echo "We have provide log files, you can use the following command to generate figures:"
    echo "bash run_all.sh /home/frw/asplos_ae/ae_scripts/log"

    exit 1
fi

log_path=$1
option=$2

mkdir -p $log_path
cd $log_path
sudo ${spdk_path}/scripts/setup.sh > /dev/null
source ${project_path}/ae_scripts/performance.sh

if [ -z "$option" ]; then
    echo "Running exp1"
    source ${project_path}/ae_scripts/exp1.sh
    echo "Running exp2"
    source ${project_path}/ae_scripts/exp2.sh
    echo "Running exp3"
    source ${project_path}/ae_scripts/exp3.sh
    echo "Running exp4"
    source ${project_path}/ae_scripts/exp4.sh
    echo "Ploting Figure 8,10,11,12"
    python ${project_path}/ae_scripts/python/figure_8-10-11-12.py
    echo "Ploting Figure 13"
    python ${project_path}/ae_scripts/python/figure_13.py
    echo "Ploting Figure 14"
    python ${project_path}/ae_scripts/python/figure_14.py
    echo "Ploting Figure 9"
    python ${project_path}/ae_scripts/python/figure_9.py
    echo "done! All figures are generated in ${log_path}/figures."
else
    case $option in
        1)
            echo "Running exp1"
            source ${project_path}/ae_scripts/exp1.sh
            echo "Ploting Figure 8-10-11-12"
            python ${project_path}/ae_scripts/python/figure_8-10-11-12.py
            ;;
        2)
            echo "Running exp2"
            source ${project_path}/ae_scripts/exp2.sh
            echo "Ploting Figure 13"
            python ${project_path}/ae_scripts/python/figure_13.py
            ;;
        3)
            echo "Running exp3"
            source ${project_path}/ae_scripts/no_cache.sh
            echo "Ploting Figure 14"
            python ${project_path}/ae_scripts/python/figure_14.py
            ;;
        4)
            echo "Running exp4"
            source ${project_path}/ae_scripts/exp3.sh
            echo "Ploting Figure 9"
            python ${project_path}/ae_scripts/python/figure_9.py
            ;;
        *)
            echo "Invalid option. Please choose a number between 1 and 4."
            exit 1
            ;;
    esac

fi

