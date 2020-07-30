# PIM-JSON
JSON to binary conversion in memory

## strstr/
This is a base implementation of recognizing records and search pattern string host-dpu program  
DPU code in this folder is slightly out-dated, the latest code is in sparser/

## sparser/
This is integrating the dpu implementation of strstr to the existing sparser code base  
RapidJson is used for the host to do actual parsing  

To run the original sparser code:  
git submodule update --init --recursive  
cd demo-repl/  
make  
./bench ${json file name}  

To run the code integrating with DPU:  
cd dpu-demo/  
make clean; make  
./bench ${json file name}  