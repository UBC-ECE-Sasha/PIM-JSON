import re

file1 = open('out.log', 'r')
lines = file1.readlines() 

# with open('business.json', "r") as f:
ids = []
for line in lines:
    if("CHECK" in line):
        res = [int(i) for i in line.split() if i.isdigit()]
        if(len(res) <2):
            continue
        
        ids.append(res[1])


ids.sort()
#print(ids)
print("parse the following  reocrods")
print(len(ids))

seen = set()
uniq = [x for x in ids if x not in seen and not seen.add(x)]   

print("parse the following unique reocrods")
print(len(seen))

file2 = open('tweets_backup_2.json', 'r')
lines = file2.readlines()

actual_nums = []
for line in lines:
    num = int(line[20])*10000 + int(line[21])*1000 + int(line[22])*100 +int(line[23])*10 +int(line[24])
    if(num < 1120):
        actual_nums.append(num)

print("actual length of the records")
print(len(actual_nums))
seen_2 = set()
uniq = [x for x in actual_nums if x not in seen_2 and not seen_2.add(x)]   
print(len(seen_2))

print(seen_2.difference(seen))

file2.close()
file1.close()


