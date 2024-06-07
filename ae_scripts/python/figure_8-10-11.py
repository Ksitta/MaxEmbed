# %%
import pandas as pd
import matplotlib.pyplot as plt


# %%
rename_dict = {
	"result_alibaba.csv": "Alibaba iFashion",
 	"result_avazu.csv": "Avazu",
 	"result_criteo.csv": "Criteo",
	"result_criteoTB.csv":"CriteoTB",
	"result_amazon_m2.csv": "Amazon M2",
}

data_source = './merged_results/'


# %%
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


