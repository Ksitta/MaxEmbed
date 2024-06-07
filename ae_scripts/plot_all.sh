# /bin/bash
mkdir -p merged_results

python ${project_path}/ae_scripts/python/merge_data.py

mkdir -p figures

python ${project_path}/ae_scripts/python/figure_8-10-11.py
python ${project_path}/ae_scripts/python/figure_9.py
python ${project_path}/ae_scripts/python/figure_12.py
python ${project_path}/ae_scripts/python/figure_13.py
python ${project_path}/ae_scripts/python/figure_14.py
