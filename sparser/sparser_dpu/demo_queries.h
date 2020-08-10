#ifndef _DEMO_QUERIES_H_
#define _DEMO_QUERIES_H_

#include "rapidjson_engine.h"
#include "../dpu_common.h"

// The RapidJSON query engine.
json_query_engine_t rapidjson_engine = json_query_rapidjson_execution_engine;

typedef json_query_t (*zakir_query_t)();
typedef const char **(*sparser_zakir_query_preds_t)(int *);

// callback_info struct that is passed in as a void *
// to individual filters
typedef struct callback_info {
    unsigned long ptr;
    json_query_t query;
    long count;
    long capacity;
} callback_info_t;

// Structs for storing projected fields of queries (e.g., zakir_q9_proj_t)
// need to be packed, so that UnsafeRow reads them correctly
#pragma pack(1)
#ifndef QUERY
#define QUERY NULL
#endif


// ************************ DEMO QUERY 2 **************************

const char *DEMO_QUERY1_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE name contains \"Yoga\" AND name contains \"Fitness\"";

const char *DEMO_QUERY2_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE name contains \"aabaa\"";

const char *DEMO_QUERY3_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"Trump\" and text contains \"China\"" ;

const char *DEMO_QUERY4_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"aabaa\"";
// 6GB 6.5%
const char *DEMO_QUERY5_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"delicious\" and text contains \"good\"";
// 6GB 15%
const char *DEMO_QUERY6_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"delivery\" and text contains \"good\"";

const char *DEMO_QUERY7_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"delicious\"";
// 6GB 9%
const char *DEMO_QUERY8_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"delicious\" AND text contains \"delivery\"";
// 6GB 1.2%
const char *DEMO_QUERY9_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"burgers\" AND text contains \"good\"";
// 6GB 2%
const char *DEMO_QUERY10_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"pizza\" AND text contains \"delivery\"";
// 6GB 0.1%
const char *DEMO_QUERY11_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"delicious\" AND text contains \"steak\"";

const char *DEMO_QUERY12_STR = "\n\
SELECT count(*)\n\
FROM tweets\n\
WHERE text contains \"aaba\"";


json_passed_t demo_q1_text(const char *value, void *) {
    return strstr(value, "Yoga") && strstr(value, "Fitness") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q2_text(const char *value, void *) {
    return strstr(value, "aabaa") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q3_text(const char *value, void *) {
    return strstr(value, "Trump") && strstr(value, "China") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q4_text(const char *value, void *) {
    return strstr(value, "aabaa") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q5_text(const char *value, void *) {
    return strstr(value, "delicious") && strstr(value, "good") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q6_text(const char *value, void *) {
    return strstr(value, "delivery") && strstr(value, "good")  ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q7_text(const char *value, void *) {
    return strstr(value, "delicious") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q8_text(const char *value, void *) {
    return strstr(value, "delicious") && strstr(value, "delivery") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q9_text(const char *value, void *) {
    return strstr(value, "burgers") && strstr(value, "good") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q10_text(const char *value, void *) {
    return strstr(value, "pizza") && strstr(value, "delivery") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q11_text(const char *value, void *) {
    return strstr(value, "delicious") && strstr(value, "steak") ? JSON_PASS : JSON_FAIL;
}

json_passed_t demo_q12_text(const char *value, void *) {
    return strstr(value, XSTR(QUERY)) ? JSON_PASS : JSON_FAIL;
}


json_query_t demo_query1() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "name", demo_q1_text);
    return query;
}

json_query_t demo_query2() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "name", demo_q2_text);
    return query;
}

json_query_t demo_query3() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q3_text);
    return query;
}

json_query_t demo_query4() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q4_text);
    return query;
}

json_query_t demo_query5() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q5_text);
    return query;
}

json_query_t demo_query6() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q6_text);
    return query;
}

json_query_t demo_query7() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q7_text);
    return query;
}

json_query_t demo_query8() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q8_text);
    return query;
}

json_query_t demo_query9() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q9_text);
    return query;
}

json_query_t demo_query10() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q10_text);
    return query;
}

json_query_t demo_query11() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q11_text);
    return query;
}

json_query_t demo_query12() {
    json_query_t query = json_query_new();
    json_query_add_string_filter(query, "text", demo_q12_text);
    return query;
}

static const char **sparser_demo_query1(int *count) {
    static const char *_1 = "Yoga";
    static const char *_2 = "Fitness";
    static const char *predicates[] = {_1, _2, NULL};

    *count = 2;
    return predicates;
}

static const char **sparser_demo_query2(int *count) {
    static const char *_1 = "aabaa";
    static const char *predicates[] = {_1, NULL};
    *count = 1;
    return predicates;
}

static const char **sparser_demo_query3(int *count) {
    static const char *_1 = "Trump";
    static const char *_2 = "China";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 1;
    return predicates;
}

static const char **sparser_demo_query4(int *count) {
    static const char *_1 = "aabaa";
    static const char *predicates[] = {_1, NULL};
    *count = 1;
    return predicates;
}

static const char **sparser_demo_query5(int *count) {
    static const char *_1 = "delicious";
    static const char *_2 = "good";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 2;
    return predicates;
}

static const char **sparser_demo_query6(int *count) {
    static const char *_1 = "delivery";
    static const char *_2 = "good";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 1;
    return predicates;
}

static const char **sparser_demo_query7(int *count) {
    static const char *_1 = "delicious";
    static const char *predicates[] = {_1, NULL};
    *count = 1;
    return predicates;
}

static const char **sparser_demo_query8(int *count) {
    static const char *_1 = "delicious";
    static const char *_2 = "delivery";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 2;
    return predicates;
}

static const char **sparser_demo_query9(int *count) {
    static const char *_1 = "burgers";
    static const char *_2 = "good";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 2;
    return predicates;
}

static const char **sparser_demo_query10(int *count) {
    static const char *_1 = "pizza";
    static const char *_2 = "delivery";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 2;
    return predicates;
}

static const char **sparser_demo_query11(int *count) {
    static const char *_1 = "delicious";
    static const char *_2 = "steak";
    static const char *predicates[] = {_1, _2, NULL};
    *count = 2;
    return predicates;
}

static const char **sparser_demo_query12(int *count) {
    static const char *_1 = XSTR(QUERY);
    static const char *predicates[] = {_1, NULL};
    *count = 1;
    return predicates;
}

// ************** All the queries we want to test **************
const zakir_query_t demo_queries[] = {demo_query1, demo_query2, demo_query3, demo_query4, demo_query5, demo_query6, demo_query7, demo_query8, demo_query9, demo_query10, demo_query11, demo_query12, NULL};
const sparser_zakir_query_preds_t sdemo_queries[] = { sparser_demo_query1, sparser_demo_query2, sparser_demo_query3, sparser_demo_query4, sparser_demo_query5, sparser_demo_query6, sparser_demo_query7, sparser_demo_query8, sparser_demo_query9, sparser_demo_query10, sparser_demo_query11, sparser_demo_query12, NULL};
const char *demo_query_strings[] = { DEMO_QUERY1_STR, DEMO_QUERY2_STR, DEMO_QUERY3_STR, DEMO_QUERY4_STR, DEMO_QUERY5_STR, DEMO_QUERY6_STR, DEMO_QUERY7_STR, DEMO_QUERY8_STR, DEMO_QUERY9_STR, DEMO_QUERY10_STR, DEMO_QUERY11_STR, DEMO_QUERY12_STR, NULL };

#endif
