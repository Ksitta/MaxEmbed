import argparse
import multiprocessing
import numpy as np

def split_item(item):
    result = item.split()
    result = [int(x) for x in result]
    result = np.array(result)
    return result

def read_data(filename: str, select_num: int, skip_first_line):
    raw_file = open(filename, "r")
    if skip_first_line:
        raw_file.readline()
    data = raw_file.read().splitlines()
    if select_num > 0:
        data = data[:select_num]

    with multiprocessing.Pool() as pool:
        results = pool.map(split_item, data)

    return results

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=str, required=True, help="Input file path")
    parser.add_argument("--output", type=str, required=True, help="Output file path")
    parser.add_argument("--binary_output", type=int, default=1, help="Binary output")
    parser.add_argument("--select_num", type=int, default=0, help="Select number of records")
    parser.add_argument("--train_ratio", type=float, default=0.8, help="Train ratio")
    parser.add_argument("--skip_first_line", type=int, default=0, help="Skip first line of input file")
    
    args = parser.parse_args()
    print("Input file", args.input)
    print("Output file", args.output)
    print("Binary output", bool(args.binary_output))
    print("Select ", args.select_num, "records")
    print("Train ratio", args.train_ratio)
    print("Skip first line", bool(args.skip_first_line))
    return args

def write_data(args, vetex, indexes, offsets, results):
    indexes = indexes.astype(np.int64)
    offsets = offsets.astype(np.int32)

    train_len = int(len(results) * args.train_ratio)

    train_indexes = indexes[:offsets[train_len]]
    train_offsets = offsets[:train_len + 1]

    valid_indexes = indexes[offsets[train_len]:]
    valid_offsets = offsets[train_len:]

    assert indexes.dtype == np.int64
    assert offsets.dtype == np.int32

    assert len(train_offsets) == train_len + 1
    assert len(valid_offsets) == len(results) - train_len + 1

    real_len = 0
    for i in range(len(train_offsets) - 1):
        real_len += (train_offsets[i+1] - train_offsets[i] != 0)

    if args.binary_output:
        output_file = open(args.output + ".train", "wb")
        output_file.write(len(vetex).to_bytes(8, byteorder="little"))
        output_file.write(train_len.to_bytes(8, byteorder="little"))
        output_file.write(train_offsets.tobytes())
        output_file.write(train_indexes.tobytes())
    else:
        output_file = open(args.output + ".train", "w")
        output_file.write(str(0) + " ")
        output_file.write(str(len(vetex)) + " ")
        output_file.write(str(real_len) + " ")
        output_file.write(str(train_offsets[-1]) + "\n")
        for i in range(len(train_offsets) - 1):
            if(train_offsets[i+1] - train_offsets[i] == 0):
                continue
            output_file.write(" ".join([str(x) for x in train_indexes[train_offsets[i]:train_offsets[i+1]]]) + "\n")
        
    if args.train_ratio != 1:
        valid_offsets -= valid_offsets[0]
        if args.binary_output:
            valid_output_file = open(args.output + ".test", "wb")
            valid_output_file.write(len(vetex).to_bytes(8, byteorder="little"))
            valid_output_file.write((len(results) - train_len).to_bytes(8, byteorder="little"))
            valid_output_file.write(valid_offsets.tobytes())
            valid_output_file.write(valid_indexes.tobytes())
        else:
            valid_output_file = open(args.output + ".test", "w")
            valid_output_file.write(str(0) + " ")
            valid_output_file.write(str(len(vetex)) + " ")
            valid_output_file.write(str(len(results) - train_len) + " ")
            valid_output_file.write(str(valid_offsets[-1]) + "\n")
            for i in range(len(valid_offsets) - 1):
                if(valid_offsets[i+1] - valid_offsets[i] == 0):
                    continue
                valid_output_file.write(" ".join([str(x) for x in valid_indexes[valid_offsets[i]:valid_offsets[i+1]]]) + "\n")
        
def preprocess(args, results):
    offsets = [0]
    cnt = 0
    for result in results:
        cnt += len(result)
        offsets.append(cnt)

    offsets = np.array(offsets)
    indexes = np.concatenate(results)
    vetex, indexes, counts = np.unique(indexes, return_inverse=True, return_counts=True)

    if args.remove_hot_ratio > 0 or args.remove_cold_cnt > 0:
        sorted_counts = np.sort(counts)
        counts_len = len(counts)
        condition = (counts[indexes] < args.remove_cold_cnt) | (counts[indexes] > sorted_counts[int(counts_len * (1 - args.remove_hot_ratio)) - 1])
        to_remove = np.nonzero(condition)[0]
        indexes = indexes[~condition]
        j = 0
        if len(to_remove) != 0:
            for i in range(len(offsets)):
                while j + 1 < len(to_remove) and to_remove[j + 1] < offsets[i]:
                    j += 1
                if to_remove[j] < offsets[i]:
                    offsets[i] -= j + 1

    vetex, indexes = np.unique(indexes, return_inverse=True)
    
    return vetex, indexes, offsets

def main(args):
    results = read_data(args.input, args.select_num, args.skip_first_line)
    vetex, indexes, offsets = preprocess(args, results)
    write_data(args, vetex, indexes, offsets, results)
    
if __name__ == "__main__":
    args = parse_args()
    main(args)