import os, sys

def merge_result(base: str, outfile: str):
    files = os.listdir(base)
    with open(outfile, "w") as outf:
        outf.write(f"method,replication_ratio,cache_ratio,SSD_bw,eff_bw,throughput,latency,cache,index,ssd,model\n")
        for filename in sorted(files):
            # print(filename)
            try:
                with open(os.path.join(base, filename)) as f:            
                    data = f.readlines()
                    result = data[-1]
            except:
                continue
            names = filename.split("_")
            outf.write(f"{names[0]},{names[2]},{names[3][:-4]},{result}")
            
for dataset in ["amazon_m2", "avazu", "alibaba"]:
    merge_result(f"./result_{dataset}_algo", f"./merged_results/result_{dataset}_algo.csv")
    
for dataset in ["amazon_m2", "avazu", "alibaba", "criteo", "criteoTB"]:
    merge_result(f"./result_{dataset}", f"./merged_results/result_{dataset}.csv")
    
for dataset in ["avazu", "alibaba", "criteo", "criteoTB"]:
    merge_result(f"./result_{dataset}_no_cache", f"./merged_results/result_{dataset}_no_cache.csv")
    