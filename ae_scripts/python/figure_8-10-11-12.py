
import pandas as pd
import matplotlib.pyplot as plt

from merge_data import merge_result

rename_dict = {
	"result_alibaba.csv": "Alibaba iFashion",
 	"result_avazu.csv": "Avazu",
 	"result_criteo.csv": "Criteo",
	"result_criteoTB.csv":"CriteoTB",
	"result_amazon_m2.csv": "Amazon M2",
}

for each in ["alibaba", "avazu", "criteo", "amazon_m2", "criteoTB"]:
    merge_result(f"./exp1/result_{each}", f"./merged_results/result_{each}.csv")

data_source = './merged_results/'

df_all = pd.DataFrame()
for each in ["result_alibaba.csv", "result_avazu.csv", "result_criteo.csv", "result_amazon_m2.csv", "result_criteoTB.csv"]:
    df = pd.read_csv(data_source + each)
    df['dataset'] = rename_dict[each]
    df_all = pd.concat([df_all, df])
    
df_all.drop(columns=['method'], inplace=True)
# %%
# bandwidth
df_exp = df_all.query("cache_ratio == 0.1")
temp_df = df_exp.groupby(['dataset', 'replication_ratio']).aggregate(['mean', 'max']) ['eff_bw']
assert (temp_df['mean'] == temp_df['max']).all()

temp_df = temp_df[['mean']]
temp_df.rename(columns={'mean': 'bandwidth'}, inplace=True)

temp_df = temp_df['bandwidth'].unstack()
temp_df.columns.name = None
temp_df = temp_df.reset_index()

temp_df.set_index('dataset', inplace=True)
for each in temp_df.columns[1:]:
	temp_df[each] = temp_df[each] / temp_df[0.0]

temp_df[0.0] = temp_df[0.0] / temp_df[0.0]
temp_df = temp_df * 100
temp_df = temp_df.reset_index()
temp_df.columns = ['dataset', 'SHP', 'ME(r=10%)', 'ME(r=20%)', 'ME(r=40%)', 'ME(r=80%)']

average_values = temp_df.groupby('dataset').mean()

average_values.plot(kind='bar', figsize=(8,3), rot=0)

plt.title('Effective bandwidth under different replication ratios')
plt.ylabel('Normalized effective bandwidth (%)')
plt.xlabel('Datasets')
plt.legend(title='Dataset', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.ylim(80, 125)
plt.tight_layout()
plt.savefig('./figures/figure8.png')

# %%
df_exp = df_all.query("cache_ratio == 0.1")

temp_df = df_exp.groupby(['dataset', 'replication_ratio']).aggregate(['mean', 'max']) ['throughput']
assert (temp_df['mean'] == temp_df['max']).all()

temp_df = temp_df[['mean']]
temp_df.rename(columns={'mean': 'throughput'}, inplace=True)

temp_df = temp_df['throughput'].unstack()
temp_df.columns.name = None

temp_df = temp_df.reset_index()

temp_df = temp_df.set_index('dataset')
for each in temp_df.columns[1:]:
	temp_df[each] = temp_df[each] / temp_df[0.0]
temp_df[0.0] = temp_df[0.0] / temp_df[0.0]

temp_df = temp_df  * 100
temp_df = temp_df.reset_index()
temp_df.columns = ['dataset', 'SHP', 'ME(r=10%)', 'ME(r=20%)', 'ME(r=40%)', 'ME(r=80%)']

average_values = temp_df.groupby('dataset').mean()

average_values.plot(kind='bar', figsize=(8,3), rot=0)

plt.title('Throughput of end-to-end evaluation')
plt.ylabel('Normalized Throughput (%)')
plt.xlabel('Datasets')
plt.legend(title='Dataset', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.ylim(80, 125)
plt.tight_layout()
# plt.show()
plt.savefig('./figures/figure10.png')


# %%
df_exp = df_all.query("cache_ratio == 0.1")

temp_df = df_exp.groupby(['dataset', 'replication_ratio']).aggregate(['mean', 'max']) ['latency']
assert (temp_df['mean'] == temp_df['max']).all()

temp_df = temp_df[['mean']]
temp_df.rename(columns={'mean': 'latency'}, inplace=True)

temp_df = temp_df['latency'].unstack()
temp_df.columns.name = None

temp_df = temp_df.reset_index()

temp_df = temp_df.set_index('dataset')
for each in temp_df.columns[1:]:
	temp_df[each] = temp_df[each] / temp_df[0.0]
temp_df[0.0] = temp_df[0.0] / temp_df[0.0]

temp_df = temp_df  * 100
temp_df = temp_df.reset_index()
temp_df.columns = ['dataset', 'SHP', 'ME(r=10%)', 'ME(r=20%)', 'ME(r=40%)', 'ME(r=80%)']
# display(temp_df)
# temp_df.to_csv("output/draw_latency.csv", index=False)
average_values = temp_df.groupby('dataset').mean()

average_values.plot(kind='bar', figsize=(8, 3), rot=0)

plt.title('Latency of end-to-end evaluation')
plt.ylabel('Normalized Latency(%)')
plt.xlabel('Datasets')
plt.legend(title='Dataset', bbox_to_anchor=(1.05, 1), loc='upper left')
plt.ylim(80, 110)
plt.tight_layout()
plt.show()
plt.savefig('./figures/figure11.png')



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
