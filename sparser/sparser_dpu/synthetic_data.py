import argparse
import json


def manual_data(percentage, file):
    f_r = open(file, 'r') 
    f_w = open('manual_data.json', 'w')
    lines = f_r.readlines() 
    line_num = 0
    total_num = len(lines)

    for line in lines:
        if(line_num > total_num*percentage):
            break
        data = json.loads(line)
        data['text'] += ' aabaa'
        print(data)
        s = json.dumps(data)
        f_w.writelines(s+'\n')
        line_num +=1
    return 0



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="manual edit data")
    parser.add_argument("--percentage", "-p", type=float, required=True)
    parser.add_argument("--file", "-f", type=str, required=True)
    args = parser.parse_args()
    print(args.percentage)
    manual_data(args.percentage, args.file)