
#include "json_facade.h"
#include "demo_queries.h"
#include "common.h"

#include "sparser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include "dpu_host.h"
#include "dpu_common.h"
}
#include <assert.h>
// This is some data we pass to the callback. In this example, we pass a JSON query
// so our JSON parser can parse the record, and a count to update the number of matching
// records.
struct callback_data {
	long count;
	json_query_t query;
};

#define ENABLE_CALIBRATE 1
#define DEBUG 0


int _rapidjson_parse_callback(const char *line, void *query) {
  if (!query) return false;
	struct callback_data *data = (struct callback_data *)query;
  char *newline = (char *)strchr(line, '\n');
  // Last one?
  if (!newline) {
    newline = (char *)(line + strlen(line) + 1);
  }
  char tmp = *newline;
  *newline = '\0';
	int passed = rapidjson_engine(data->query, line, NULL);
	if (passed) {
		data->count++;
	}
  *newline = tmp;
	return passed;
}

double bench_rapidjson_engine(char *data, long length, json_query_t query, int queryno) {
  bench_timer_t s = time_start();
  long doc_index = 1;
  long matching = 0;

  char *ptr = data;
  char *line;

	double count = 0;

  while ((line = strsep(&ptr, "\n")) != NULL) {
    if (rapidjson_engine(query, line, NULL) == JSON_PASS) {
      matching++;
    }

		count++;

		if (ptr)
			*(ptr - 1) = '\n';

    doc_index++;
  }

  double elapsed = time_stop(s);
  printf("RapidJSON:\t\t\x1b[1;33mResult: %ld (Execution Time: %f seconds)\x1b[0m\n", matching, elapsed);
  return elapsed;
}

double bench_sparser_engine(char *data, long length, json_query_t jquery, ascii_rawfilters_t *predicates, int queryno) {
  bench_timer_t s = time_start();

	struct callback_data cdata;
	cdata.count = 0;
  cdata.query = jquery;

  // XXX Generate a schedule
  sparser_query_t *query = sparser_calibrate(data, length, '\n', predicates, _rapidjson_parse_callback, &cdata);

  // XXX Apply the search.
  sparser_stats_t *stats = sparser_search(data, length, '\n', query, _rapidjson_parse_callback, &cdata);

  double elapsed = time_stop(s);
  printf("RapidJSON with Sparser:\t\x1b[1;33mResult: %ld (Execution Time: %f seconds)\x1b[0m\n", cdata.count, elapsed);
  printf("stats callback_passed %d\n", stats->callback_passed);
  	
  free(stats);
  free(query);
  return elapsed;
}


int process_return_buffer(char* buf, uint32_t length,struct callback_data* cdata){
	int n_succeed = 0;
	int record_count = 0;

	/// Last byte in the data.
	char *input_last_byte = buf + length; //-1;
	// Points to the end of the current record.
	char *current_record_end;
	// Points to the start of the current record.
	char *current_record_start;

	current_record_start = buf;
	while (current_record_start < input_last_byte) {
		record_count++;
			current_record_end = (char *)memchr(current_record_start, '\n',
          input_last_byte - current_record_start);		
			if (!current_record_end) {
				current_record_end = input_last_byte;
			}
			size_t record_length = current_record_end - current_record_start;

			// magic number
			if( record_length < MAX_RECORD_SIZE+1) {
				#if DEBUG
				printf("cpu record length %d\n", record_length);
				for (int k=0; k< record_length; k++){
						char c;
						c= current_record_start[k];
						putchar(c);
				}
				printf("\n");
				#endif				
				if (_rapidjson_parse_callback(current_record_start, cdata)) {
						n_succeed++;
				}	
			} 			
				
			
			// Update to point to the next record. The top of the loop will update the remaining variables.
			current_record_start = current_record_end + 1;
	}

	return n_succeed;
}



void bench_dpu_sparser_engine(char *data, long length, json_query_t jquery, ascii_rawfilters_t *predicates, int queryno) {
  bench_timer_t s = time_start();
  struct callback_data cdata;
	cdata.count = 0;
  	cdata.query = jquery;	
	// char* ret_buf;

	int parse_suceed =0;

	// XXX Generate a schedule
	#if ENABLE_CALIBRATE 
	sparser_query_t *query = sparser_calibrate(data, length, '\n', predicates, _rapidjson_parse_callback, &cdata);
	#elif
	sparser_query_t *query = sparser_new_query();
	memset(query, 0, sizeof(sparser_query_t));	
	query->count = 1;
	memcpy(query->queries[query->count], "abaa", 4);
		// printf("dpu searched query is %s query count %d\n", query->queries[0], query->count);
		// get the filtered buffer
	#endif

	// get the return buffer array ready
	uint8_t *ret_bufs[NR_DPUS];
	uint32_t records_len[NR_DPUS];

	int i=0;
	for (i=0; i<NR_DPUS; i++) {
		ret_bufs[i] = (uint8_t *) malloc(RETURN_RECORDS_SIZE);
	}

	// process the records
	//multi_dpu_test(data, length, ret_bufs);
	multi_dpu_test(data, length, ret_bufs, records_len);

	//process the return buffer
	for (i=0; i<NR_DPUS; i++) {
		parse_suceed += process_return_buffer((char*)ret_bufs[i], records_len[i], &cdata);
	}

	double elapsed = time_stop(s);
  	printf("RapidJSON with Sparser plus DPU:\t\x1b[1;33mResult: %ld (Execution Time: %f seconds)\x1b[0m\n", parse_suceed, elapsed);

#if 0
  ret_buf = (char*) malloc(RETURN_RECORDS_SIZE);

	dpu_test(data, query->queries[0], ret_buf);
	double elapsed = time_stop(s);
	/// Last byte in the data.
	char *input_last_byte = ret_buf + RETURN_RECORDS_SIZE - 1;
	// Points to the end of the current record.
	char *current_record_end;
	// Points to the start of the current record.
	char *current_record_start;

	current_record_start = ret_buf;
	while (current_record_start < input_last_byte) {
		record_count++;
			current_record_end = (char *)memchr(current_record_start, '\n',
          input_last_byte - current_record_start);		
			if (!current_record_end) {
				current_record_end = input_last_byte;
			}
			size_t record_length = current_record_end - current_record_start;
			#if DEBUG
			printf("cpu record length %d\n", record_length);
			for (int k=0; k< record_length; k++){
					char c;
					c= current_record_start[k];
					putchar(c);
			}
			printf("\n");
			#endif 			
			if (_rapidjson_parse_callback(current_record_start, &cdata)) {
					parse_suceed++;
			}					
			
			// Update to point to the next record. The top of the loop will update the remaining variables.
			current_record_start = current_record_end + 1;
	}
#endif
}


#define OK       0
#define END_OF_FILE 1
#define TOO_LONG 2

// From https://stackoverflow.com/questions/4023895/how-to-read-string-entered-by-user-in-c
static int getLine (const char *prmpt, char *buff, size_t sz) {
	int ch, extra;

	// Get line with buffer overrun protection.
	if (prmpt != NULL) {
		printf ("%s", prmpt);
		fflush (stdout);
	}
	if (fgets (buff, sz, stdin) == NULL)
		return END_OF_FILE;

	// If it was too long, there'll be no newline. In that case, we flush
	// to end of line so that excess doesn't affect the next call.
	if (buff[strlen(buff)-1] != '\n') {
		extra = 0;
		while (((ch = getchar()) != '\n') && (ch != EOF))
			extra = 1;
		return (extra == 1) ? TOO_LONG : OK;
	}

	// Otherwise remove newline and give string back to caller.
	buff[strlen(buff)-1] = '\0';
	return OK;
}

void process_query(char *raw, long length, int query_index) {
	printf("Running query:\n ---------------------\x1b[1;31m%s\x1b[0m\n ---------------------\n",
      demo_query_strings[query_index]);

	int count = 0;
	json_query_t jquery = demo_queries[query_index]();
	const char ** preds = sdemo_queries[query_index](&count);

  // XXX First, get a set of candidate RFs.
	ascii_rawfilters_t d = decompose(preds, count);

	bench_sparser_engine(raw, length, jquery, &d, query_index);
	//bench_dpu_sparser_engine(raw, length, jquery, &d, query_index);
	// get the return buffer array ready
	 uint8_t *ret_bufs[NR_DPUS];
	 uint32_t records_len[NR_DPUS];
	 for (int i=0; i<NR_DPUS; i++) {
	 	ret_bufs[i] = (uint8_t *) malloc(RETURN_RECORDS_SIZE);
	 }
	// multi_dpu_test(raw, length, ret_bufs, records_len);
	multi_dpu_test(raw, length, ret_bufs, records_len);
	// for(int d=0; d< NR_DPUS; d++) {
	// 	if(records_len[d] !=0) {
	// 	for (int k=0; k< records_len[d]; k++){
	// 		char c;
	// 		c= ret_bufs[d][k];
	// 		putchar(c);
	// 	}
	// 	printf("\n");
	// 	}
	// }



	json_query_t query = demo_queries[query_index]();
	bench_rapidjson_engine(raw, length, query, query_index);


  free_ascii_rawfilters(&d);
}

void print_queries(int num_queries) {
	for (int i = 0; i < num_queries; i++) {
		printf("\x1b[1;34mQuery %d:\x1b[0m -------------\x1b[1;31m%s\x1b[0m\n ---------------------\n", i+1, demo_query_strings[i]);
	}
}

void print_usage(char **argv) {
  fprintf(stderr, "Usage: %s <JSON filename>\n", argv[0]);
}

int main(int argc, char **argv) {

  char *raw;
  long length;

	// Read in the data beforehand.
  const char *filename = argv[1];
  if(access(filename, F_OK) == -1 ) {
    print_usage(argv);
    exit(EXIT_FAILURE);
  }

	printf("Reading data...");
	fflush(stdout);
  length = read_all(filename, &raw);
	printf("done! read %f GB\n", (double)length / 1e9);
	fflush(stdout);

	// Get the number of queries.
	int num_queries = 0;
	while (demo_queries[num_queries]) {
		num_queries++;
	};

	while (true) {
    int rc;
    char buff[1024];
    rc = getLine ("Sparser> ", buff, sizeof(buff));
    if (rc == END_OF_FILE) {
        printf ("\nEOF\n");
				break;
    }

		if (rc == TOO_LONG) {
        printf ("Input too long [%s]\n", buff);
				continue;
    }

		if (strcmp(buff, "queries") == 0) {
			printf("Dataset: %s (%f GB)\n", filename, (double)length / 1e9);
			print_queries(num_queries);
			continue;
		}

		char *endptr;
		long query_index = strtoul(buff, &endptr, 10);
		if (endptr == buff || query_index > num_queries) {
			printf("Invalid query \"%s\"\n", buff);
			continue;
		}

		query_index--;
		process_query(raw, length, query_index);
		/* temporary fix for inf loop problem occured during testing on the hardware */
		break;
		return 0;
	}
}
