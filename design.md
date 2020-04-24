## Sparse Implementation in DPU

The following functions are the core of the Sparser algorithm


#### sparser_calibrate - host
Returns a search query given a sample input and a set of predicates. The returned search query attempts to jointly minimize the search time and false positive rate.

#### sparser_search - dpu
This performs a simple sparser search given the query and input buffer. It only searches for one occurance of the query string in each record. Records are assumed to be delimited by newline.


In dpu, given a list of json records seperate by '\n', it should return the pointer of the starting records that contains the given search string

Implementation:
given a block of memory, divide memory to the maximum number of tasklet by the '\n', implement memmem to find if each record matches with the search string return list of pointers back to the host

In my experiment, this step is able to cut records need to be parsed from 9000 to 20 records, that's where the significant performance gain come from

#### Json parser host -> dpu



#### DPU strstr 
On the dpu, given records stored in the wram, the dpu modules need to extract each records by the deliminator '\n', then doing a strstr operation to filter the reecords by the keywords.  Previously,
a simple algorithm was attempted without taking care of each records individually. It simply ignore the deliminator and search for the keyword.  This method becomes complex when invovles with the deliminator to differentiate records or recognize records offset during implementation.  

A new method is being experiment here.  The idea is to use one tasklet to "parse" the records from wram to mram.  Then multiple consumer tasklets will do a "strstr" on the records given in the preallocated cache(mram).  

The experiment is done with a DPU application without host connection, to run the experiment:
dpu-upmem-dpurte-clang -DNR_TASKLETS=5 -o strstr_sych strstr_sych.c
Use dpu-lldb to load the excutable 

However, since upmem tasklet does not support any signal like sychronization, an inefficiency is observed as 
we could have way more consumer tasklets compares to producer thread(1). 

Next step: open to suggestions, need to figure out a way to create a signal-like method, so that whenever a tasklet's cache has been written, it's able to wake up otherwise it shoule halt. 
