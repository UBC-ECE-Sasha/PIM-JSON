import argparse
import csv
import numpy as np
import matplotlib.pyplot as plt

def parse_single_result(f, dpu_num, t_num, writer):
    line = f.readline()
    candidate_p = 0.0
    sparser_time = 0.0
    dpu_time = 0.0
    speedup = 0.0
    print(dpu_num)
    dpu_num = int(dpu_num)
    raw_filter_time_cpu = 0.0
    raw_filter_time_dpu = 0.0
    pure_raw_filter_time_dpu = 0.0
    dpu_launch = 0.0
    dpu_copy_in = 0.0
    dpu_copy_out = 0.0
    dpu_preprocess = 0.0
    dpu_parse = 0.0
    # max_speed_raw = []
    state = 0
    while "host process (parser)" not in line:
        line = f.readline()
        
        if "RapidJSON with Sparser plus DPU" in line:
            continue
        # 7
        if "RapidJSON with Sparser" in line and state == 0:
            raw_filter_time_cpu = float(line.split()[7])
            raw_filter_time_cpu = float("{:.4f}".format(raw_filter_time_cpu))
            print("raw filter cpu time is: "+ str(raw_filter_time_cpu))
            state = 1
            continue            
        
        if "RapidJSON with Sparser" in line and state == 1:
            sparser_time = float(line.split()[7])
            sparser_time = float("{:.4f}".format(sparser_time))
            state = 1
            continue

        if "sparser passed" in line:
            """           
            if dpu_num == 128:
                print("-------------------------------------------")
                num = 40007
            elif dpu_num == 256:
                num = 80501
            elif dpu_num == 512:
                num = 161401
            elif dpu_num == 1024:
                num = 319801
            elif dpu_num == 2048:
                num = 632001
            else:
                num = 632001
            """
            # num = 8021122
            num = 813791
            t_num = num
            candidate_p = float(line.split()[3])/(int(t_num))
            candidate_p = float("{:.4f}".format(candidate_p))
            print(candidate_p)
            continue

        if "stats process time" in line:
#            raw_filter_time_cpu = float(line.split()[3])
            continue
        

        if "host preprocess took" in line:
            raw_filter_time_dpu += float(line.split()[3])
            dpu_time += float(line.split()[3])
            pure_raw_filter_time_dpu += float(line.split()[3])
            dpu_preprocess = float(line.split()[3])
            continue

        if "transferring memory" in line:
            raw_filter_time_dpu += float(line.split()[2])
            dpu_time += float(line.split()[2])
            dpu_copy_in = float(line.split()[2])
            continue

        if "dpu launch" in line: 
            raw_filter_time_dpu += float(line.split()[3])
            dpu_time += float(line.split()[3])
            pure_raw_filter_time_dpu += float(line.split()[3])
            dpu_launch = float(line.split()[3])
            continue

        if "dpu copy back records" in line:
            raw_filter_time_dpu += float(line.split()[5])
            dpu_time += float(line.split()[5])
            dpu_copy_out = float(line.split()[5])
            continue

        if "host process (parser)" in line:
            dpu_time += float(line.split()[3])
            dpu_parse =  float(line.split()[3])
            continue
    speedup = float("{:.4f}".format(sparser_time/dpu_time))
    raw_speedup = float("{:.4f}".format(raw_filter_time_cpu/raw_filter_time_dpu))
    raw_pure_speedup = float("{:.4f}".format(raw_filter_time_cpu/pure_raw_filter_time_dpu))
    print("------raw speedup " + str(candidate_p) + " " + str(dpu_num) +" " + str(raw_speedup))
    print("------overall speedup" + str(speedup))
    print("------pure raw filtering speedup" + str(raw_pure_speedup))
    # writer.writerow({'version': candidate_p,'time': speedup, 'raw_filer': raw_pure_speedup, 'parallelism': dpu_num})   
    writer.writerow({'candidate_p': dpu_num, 'cpu_host_parse': sparser_time, 'dpu_launch': dpu_launch, 'dpu_copyin': dpu_copy_in,  'dpu_copyout': dpu_copy_out , 'dpu_preprocess': dpu_preprocess, 'dpu_parse': dpu_parse})



"""
def parse_single_result_v2(f, dpu_num, writer):
    line = f.readline()
    candidate_p = 0.0
    sparser_time = 0.0
    dpu_time = 0.0
    speedup = 0.0
    # print(dpu_num)
    while "host process (parser)" not in line:
        line = f.readline()
        if "RapidJSON with Sparser plus DPU" in line:
            continue
        # 7
        if "RapidJSON with Sparser" in line:
            sparser_time = float(line.split()[7])
            sparser_time = float("{:.4f}".format(sparser_time))
            
            continue

        if "sparser passed" in line:
            if dpu_num == 128:
                num = 40007
            elif dpu_num == 256:
                num = 80501
            elif dpu_num == 512:
                num = 161401
            elif dpu_num == 1024:
                num = 319801
            elif dpu_num == 2048:
                num = 632001
            
            # candidate_p = float(line.split()[3])/(8021122)
            candidate_p = float(line.split()[3])/(num)
            candidate_p = float("{:.4f}".format(candidate_p))
            # print(candidate_p)
            continue

        if "host preprocess took" in line:
            dpu_time += float(line.split()[3])
            continue

        if "transferring memory" in line:
            dpu_time += float(line.split()[2])
            continue

        if "dpu launch" in line: 
            dpu_time += float(line.split()[3])
            continue

        if "dpu copy back records" in line:
            dpu_time += float(line.split()[5])
            continue

        if "host process (parser)" in line:
            dpu_time += float(line.split()[3])
            continue
    speedup = float("{:.4f}".format(dpu_time))
    print(speedup)
    writer.writerow({'version': candidate_p,'time': speedup, 'parallelism': dpu_num})   
"""


def parse_result(file):
    f_r = open(file, 'r')
    f_csv = open('yelp_cost.csv', mode='w')
    fieldnames = ['version', 'time', 'raw_filer',  'parallelism']
    fieldnames = ['candidate_p', 'cpu_host_parse', 'dpu_launch', 'dpu_copyin',  'dpu_copyout', 'dpu_preprocess', 'dpu_parse']
    writer = csv.DictWriter(f_csv, fieldnames=fieldnames)
    dpu_num = 0
    cnt = 0
    line = f_r.readline()
    writer.writerow({'candidate_p': 'candidate_p', 'cpu_host_parse':'cpu_host_parse', 'dpu_launch':'dpu_launch', 'dpu_copyin':'dpu_copyin',  'dpu_copyout':'dpu_copyout' , 'dpu_preprocess':'dpu_preprocess' , 'dpu_parse':'dpu_parse'})
    
    # writer.writerow({'version': 'version','time': 'time', 'raw_filer':'raw_filer','parallelism': 'parallelism'})
    # writer.writerow({'version': 'host','time': 1, 'raw_filer': 1,'parallelism': 1})

    while(True):

        if "####" and "start" in line:
            dpu_num = line.split()[1]
            t_num = 1820005
            print("---------------------------new-------------------------")
            
        if "Reading data...done!" in line:
            parse_single_result(f_r, dpu_num, t_num, writer)
            
        if "-------------------------------" in line:
            print("-----------------new file---------------------------")
        line = f_r.readline()
        cnt +=1
        if cnt >140:
            break
    

    # lines = f_r.readlines()
    # print(len(lines))
    # dpu_num = 0
    # # with open('eval_out.csv', mode='w') as csv_file:
    # #     fieldnames = ['version', 'time', 'parallelism']
    # #     writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        
    # print(len(lines))
    # for line in lines: 
    #     print(line)
    #     if "####" and "start" in line:
    #         dpu_num = line.split()[1]
    #         print(dpu_num)
    #         print('y')
                # start of a dpu set  
        # for each new data
            # process and write to the csv file

def parse_mmresult(file):
    f_r = open(file, 'r')
    line = f_r.readline()
    data =[]
    cnt =0
    while(True):

        if "Display DPU Logs" in line:
            # start
            line = f_r.readline()
            print("1")
            while("dpu copy back records" not in line):
                if("DPU#" in line):
                    # new dpu 
                    for i in range(17):
                        max_inst = 0
                        if("Tasklet" in line):
                            instructions = int(line.split()[5])
                            if(instructions > max_inst):
                                max_inst = instructions
                                num_bytes = int(line.split()[6])
                        line = f_r.readline()
                        print(line)

                    data.append((max_inst, num_bytes))

        line = f_r.readline()
        cnt +=1
        if cnt > 60:
            break
    return data
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="manual edit data")
    parser.add_argument("--file", "-f", type=str, required=True)
    parser.add_argument("--mm", "-m", type=int, required=True)

    args = parser.parse_args()
    if args.mm != 0:
        data = parse_mmresult(args.file)
        # print(data[0])
        print(len(data))
        ins_per_byte = []
        ins = []
        byte_s = []
        for d in data:
            ins_per_byte.append(d[0]/d[1])
            ins.append(d[0])
            byte_s.append(d[1])
        p = plt.plot(ins_per_byte)
        plt.xlabel('number of parallelism')
        plt.ylabel('instructions per byte')
        plt.savefig('plot.png')
        print("average instruction per byte")
        print(np.average(ins_per_byte))
        print("sum instruction per byte")
        print(np.sum(ins)/np.sum(byte_s))
        print("sum")
        print(np.sum(ins))
        # print(np.max(ins))
        # print(np.min(ins))        
        # matplotlib.pyplot.show()
        
    else:
        parse_result(args.file)