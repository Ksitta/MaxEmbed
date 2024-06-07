# %%
import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd

data_alibaba = pd.read_csv('./merged_results/result_alibaba.csv')
data_avazu = pd.read_csv('./merged_results/result_avazu.csv')
data_criteo = pd.read_csv('./merged_results/result_criteo.csv')
data_criteoTB = pd.read_csv('./merged_results/result_criteoTB.csv')

# sns.set_theme("poster")
fig, axes = plt.subplots(2, 2, figsize=(16, 12))

datasets = [(data_alibaba, 'Alibaba'), (data_avazu, 'Avazu'), (data_criteo, 'Criteo'), (data_criteoTB, 'CriteoTB')]
axes = axes.flatten()

lines = []
labels = []
custom_ticks = [0.01, 0.02, 0.03, 0.05, 0.08, 0.1, 0.2, 0.4]

for ax, (data, title) in zip(axes, datasets):
    replication_ratios = data['replication_ratio'].unique()
    replication_ratios.sort()
    
    for ratio in replication_ratios:
        subset = data[data['replication_ratio'] == ratio]
        line, = ax.plot(subset['cache_ratio'], subset['throughput'], marker='o', label=f'replication_ratio={ratio}')
        if title == 'Alibaba':
            lines.append(line)
            if ratio == 0:
                labels.append(f'SHP')
            else:
                labels.append(f'ME(r={int(ratio*100)}%)')
    ax.set_xticks(custom_ticks)
    ax.set_xticklabels([str(tick) for tick in custom_ticks])
    ax.set_xscale('log')
    ax.set_xlabel(f'{title}')
    ax.grid(True, which="both", ls="--")

plt.tight_layout(rect=[0, 0, 1, 0.96])
fig.legend(lines, labels, loc='upper center', ncol=5, fontsize='large')
fig.supxlabel('Cache Ratio', ha='center', va='center')
fig.supylabel('Throughput', ha='center', va='center')
plt.savefig('./figures/figure12.png')



# %%
