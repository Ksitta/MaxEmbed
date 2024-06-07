# %%
import matplotlib.pyplot as plt
import pandas as pd
# Load the additional CSV files
file_paths = [
    './merged_results/result_alibaba_algo.csv',
    './merged_results/result_amazon_m2_algo.csv',
    './merged_results/result_avazu_algo.csv',
]
dataframes = [pd.read_csv(file_path) for file_path in file_paths]

# Titles for each plot
titles = ['Alibaba', 'Amazon M2', 'Avazu']

# Function to normalize and sort data
def normalize_and_sort(data):
    methods = data['method'].unique()
    normalized_data = pd.DataFrame()

    for method in methods:
        method_data = data[data['method'] == method].copy()
        normalization_factor = method_data[method_data['replication_ratio'] == 0]['eff_bw'].values[0]
        method_data['normalized_eff_bw'] = method_data['eff_bw'] / normalization_factor
        normalized_data = pd.concat([normalized_data, method_data], ignore_index=True)

    sorted_data = pd.DataFrame()

    for method in methods:
        method_data = normalized_data[normalized_data['method'] == method].sort_values(by='replication_ratio')
        sorted_data = pd.concat([sorted_data, method_data], ignore_index=True)

    return sorted_data

normalized_dataframes = [normalize_and_sort(df) for df in dataframes]

fig, axs = plt.subplots(1, 3, figsize=(18, 4))

for i, (ax, data, title) in enumerate(zip(axs, normalized_dataframes, titles)):
    methods = data['method'].unique()
    for method in methods:
        method_data = data[data['method'] == method]
        ax.plot(method_data['replication_ratio'], method_data['normalized_eff_bw'], marker='o', label=method)
    ax.set_title(title)
    ax.grid(True)
    if i == 0:
        ax.set_ylabel('Normalized Effective Bandwidth')
    if i == len(axs) - 1:
        ax.legend()

fig.suptitle('Normalized Effective Bandwidth vs. Replication Ratio for Each Method')
fig.supxlabel('Replication Ratio')
plt.show()
plt.savefig("./figures/figure14.png")

# %%



