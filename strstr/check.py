import re

file1 = open('out.log', 'r')
lines = file1.readlines() 

# with open('business.json', "r") as f:
ids = []
selected_ids = []
for line in lines:
    if("CHECK" in line):
        res = [int(i) for i in line.split() if i.isdigit()]
        if(len(res) <2):
            continue
        
        ids.append(res[1])

    if "SELECT" in line:
        sel = [int(i) for i in line.split() if i.isdigit()]
        if(len(sel) <2):
            continue

        selected_ids.append(sel[1])

ids.sort()
#print(ids)
print("parse the following  reocrods")
print(len(ids))

selected_ids.sort()
print("parse the following  reocrods matched the strstr")
print(len(selected_ids))
#print(selected_ids)

seen = set()
uniq = [x for x in ids if x not in seen and not seen.add(x)]   

print("parse the following unique reocrods")
print(len(seen))

sel_seen = set()
uniq = [x for x in selected_ids if x not in sel_seen and not sel_seen.add(x)]   
print("parse the following unique selected reocrods")
print(len(sel_seen))


file2 = open('tweets_backup_2.json', 'r')
lines = file2.readlines()

actual_nums = []
sel_records = []
for line in lines:
    num = int(line[20])*10000 + int(line[21])*1000 + int(line[22])*100 +int(line[23])*10 +int(line[24])
    if(num < 1120):
        actual_nums.append(num)
        if "rump" in line:
            sel_records.append(num)
            #print(line)



print("actual length of the records")
print(len(actual_nums))
seen_2 = set()
uniq = [x for x in actual_nums if x not in seen_2 and not seen_2.add(x)]   
print(len(seen_2))

print(seen_2.difference(seen))

print("actual length of the selected records")
print(len(sel_records))
seen_sel_tr = set(sel_records)
print(seen_sel_tr.difference(sel_seen))

file2.close()
file1.close()


