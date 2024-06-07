import sys
import random

input_path = sys.argv[1]
output_path = sys.argv[2]

random.seed(1)

with open(input_path, 'r') as f:
    f.readline()
    lines = f.read().splitlines()

reqs = []
item_sets = set()

for each_line in lines:
    items = each_line.split('\t')
    single_req = []
    single_req += items[0].split()
    single_req.append(items[1])
    item_sets.update(single_req)
    reqs.append(single_req)
    
reordered_map = {}
items_list = list(item_sets)
random.shuffle(items_list)

for idx, item in enumerate(items_list):
    reordered_map[item] = idx

for idx, req in enumerate(reqs):
    reqs[idx] = [reordered_map[item] for item in req]
    
with open(output_path, 'w+') as f:
    for req in reqs:
        f.write(' '.join(map(str, req)) + '\n')