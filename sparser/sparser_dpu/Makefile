#CC=${UPMEM_HOME}/bin/clang++
CFLAGS=-std=c++11 -march=native -Wall -Wno-unused-function -lstdc++
INCLUDES=-I../sparser -I../json -I../json/rapidjson/include -I .. -I .
NR_TASKLETS ?= 4
NR_DPUS ?= 2
QUERY ?= aaba
SIMULATOR ?= 1
HOST_FLAGS := -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} -DSIMULATOR=${SIMULATOR}
#DPU_FLAGS:= -I/home/larry/workspace/upmem-2020.2.1-Linux-x86_64/include/dpu #-L/home/larry/workspace/upmem-2020.2.1-Linux-x86_64/lib -ldpu
DPU_FLAGS:= `dpu-pkg-config --cflags --libs dpu`
# dpu
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)

BUILDDIR ?= build
DPU_TARGET := ${BUILDDIR}/sparser_dpu

.PHONY: all all_dpu dpu debug asm clean

all: dpu dpu_host sparser
	$(CC)  -o bench dpu_host.o query_driver.o  ${DPU_FLAGS} ${HOST_FLAGS} -lstdc++

sparser:
	cc -g $(INCLUDES) -O3 $(CFLAGS) -DNR_DPUS=${NR_DPUS} -DNR_TASKLETS=${NR_TASKLETS} -DQUERY=${QUERY} -c -o query_driver.o  query_driver.cpp

dpu:
	mkdir -p $(BUILDDIR)
	dpu-upmem-dpurte-clang -Wall -Wextra -Werror -g -I ../dpu_common.h -I dpu/strstr_dpu.h -O -DNR_TASKLETS=${NR_TASKLETS} -o ${DPU_TARGET} dpu/strstr_dpu_nowrite.c

dpu_host:
	$(CC) -Wall -Wextra -Werror -g -I ../ -I . -I ../../../PIM-common/common/include/ -O -std=c11 ${DPU_FLAGS} ${HOST_FLAGS} -c -o dpu_host.o dpu_host.c

debug:
	$(CC) $(CFLAGS) -O0 -g -DDEBUG $(INCLUDES) bench.cpp -o bench

asm:
	$(CC) $(CFLAGS) $(INCLUDES) bench.cpp -S

clean:
	rm -rf bench bench.dSYM *.s
	rm -rf build/*
	rm -rf dpu_host.o query_driver.o
