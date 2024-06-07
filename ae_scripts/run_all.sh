# /bin/bash

set -e

project_path=/home/frw/asplos_ae
spdk_path=/home/frw/spdk
dataset_path=/home/frw/dataset
client_path=${project_path}/build/client/client

if [ $# -ne 1 ]; then
    echo "Usage: $0 <path_to_log>"
    echo ""
    echo "If you specify an existing directory"
    echo "we will use the log files in the directory to draw figures."
    echo ""
    echo "If you specify a non-existing directory"
    echo "we will run the experiments and save the log files in the directory."
    echo ""
    echo "We have provided a log file in the log folder."
    echo "You can use it to draw figures."
    exit 1
fi

set -x

if [ ! -d $1 ]; then
    mkdir -p $1
    cd $1
    sudo ${spdk_path}/scripts/setup.sh
    source ${project_path}/ae_scripts/performance.sh
    source ${project_path}/ae_scripts/main_evaluation.sh
    source ${project_path}/ae_scripts/different_algorithm.sh
    source ${project_path}/ae_scripts/cdf.sh
    source ${project_path}/ae_scripts/no_cache.sh
else
    echo "Directory $1 already exists. Using exist log to draw figures."
    cd $1
fi

source ${project_path}/ae_scripts/plot_all.sh