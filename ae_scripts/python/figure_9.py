# %%
import numpy as np
import matplotlib.pyplot as plt

def read_numbers(file_path):
    with open(file_path, 'r') as file:
        data = file.readlines()
    return [int(line.strip()) for line in data if 1 <= int(line.strip()) <= 10]

def plot_cdf(ax, numbers, title, color):
    values, base = np.histogram(numbers, bins=11, range=(0, 11), density=True)
    cumulative = np.cumsum(values)
    ax.fill_between(base[:-1], cumulative, step='pre', alpha=0.4, color=color)
    ax.set_title(title)
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 1)
    ax.grid(True)

numbers1 = read_numbers('./cdf/shp.txt')
numbers2 = read_numbers('./cdf/me.txt')


# %%
fig, axs = plt.subplots(1, 2, figsize=(8, 3))

plot_cdf(axs[0], numbers1, 'SHP', 'r')
plot_cdf(axs[1], numbers2, 'ME(r=10%)', 'b')

plt.tight_layout(pad=3.0)
fig.supxlabel('valid embeddings per read operation')
fig.supylabel('Percentage of Total')
plt.show()
plt.savefig("./figures/figure9.png")

