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