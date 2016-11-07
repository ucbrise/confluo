#ifndef TEST_UTILS_H_
#define TEST_UTILS_H_

#define kMaxKeys 2560U

bool filter_fn1(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list);

bool filter_fn2(uint64_t& record_id, const unsigned char* record,
                const uint16_t record_len, const slog::token_list& list);

#endif /* TEST_UTILS_H_ */
