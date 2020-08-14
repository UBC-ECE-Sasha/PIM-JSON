# PIM-JSON
JSON to binary conversion in memory

## sparser/sparser_dpu/
This is integrating the dpu implementation of strstr to the existing sparser code base  
RapidJson is used for the host to do actual parsing  


To run the code integrating with DPU:  
Checkout the submodules included in the repo
cd sparser/sparser_dpu/
make clean; make  
./bench ${json file name} ${Query number}

A evaluation script is used for modifing number of DPUs and number of tasklets
To test the code on the hardware, don't forget to set the $SIMULATOR flag in the makefile to 0

### sparser/sparser_dpu/dpu_host.c
copy data back and forward for the host to the DPU with distributing the work for each tasklets

### sparser/sparser_dpu/dpu/strstr_dpu_nowrite.c
strstr implemented with DPU