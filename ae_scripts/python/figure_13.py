# %%
import pandas as pd
import matplotlib.pyplot as plt
from merge_data import merge_result

for each in ["alibaba", "avazu", "criteo", "criteoTB"]:
    merge_result(f"./exp2/result_{each}_no_cache", f"./merged_results/result_{each}_no_cache.csv")

file_paths = [
    './merged_results/result_alibaba_no_cache.csv',
    './merged_results/result_avazu_no_cache.csv',
    './merged_results/result_criteo_no_cache.csv',
    './merged_results/result_criteoTB_no_cache.csv'
]

# Create a list to hold the dataframes
datasets = []
for path in file_paths:
    df = pd.read_csv(path)
    df_sorted = df.sort_values(by='replication_ratio')
    datasets.append(df_sorted)

# Plotting all datasets in a 2x2 grid
fig, axes = plt.subplots(nrows=2, ncols=2, figsize=(14, 10))

titles = ['Alibaba', 'Avazu', 'Criteo', 'CriteoTB']
for ax, data, title in zip(axes.flatten(), datasets, titles):
    ax.plot(data['replication_ratio'], data['throughput'], marker='o')
    ax.set_title(f'{title}')
    ax.set_xlabel('Replication Ratio')
    ax.set_ylabel('Throughput')
    ax.grid(True)

plt.tight_layout()
plt.savefig("./figures/figure13.png")


# %%



