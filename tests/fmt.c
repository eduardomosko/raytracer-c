#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#define FT_TEST_DEBUG
#define FT_TEST_MAIN
#include "ft_test.h"

#include "../src/msk.h"

FT_TEST(fmt_u8_to_buf) {
	const u64 cap = 32;
	u8		  out[cap];

	memset(out, 0, cap);
	fmt_u64_to_buf(20u, out, cap);
	FT_EQ(str, "20", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(255u, out, cap);
	FT_EQ(str, "255", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(127u, out, cap);
	FT_EQ(str, "127", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(0u, out, cap);
	FT_EQ(str, "0", (char*)out);
}

FT_TEST(fmt_u64_to_buf) {
	const u64 cap = 32;
	u8		  out[cap];

	memset(out, 0, cap);
	fmt_u64_to_buf(20, out, cap);
	FT_EQ(str, "20", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(1234567, out, cap);
	FT_EQ(str, "1234567", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(18446744073709551615u, out, cap);
	FT_EQ(str, "18446744073709551615", (char*)out);

	memset(out, 0, cap);
	fmt_u64_to_buf(0, out, cap);
	FT_EQ(str, "0", (char*)out);
}

FT_TEST(fmt_u64_to_buf_cap) {
	const u64 cap = 12;
	u64		  len = 1;
	u8		  out[cap];
	u8		  ref[cap];

	len = 1;
	memset(out, 15, cap);
	memset(ref, 15, cap);
	memcpy(ref, "20", len);
	u64 written = fmt_u64_to_buf(20, out, len);

	FT_EQ(size_t, written, 1);
	FT_EQ(buffer, ref, out, .size = cap);

	len = 5;
	memset(out, 16, cap);
	memset(ref, 16, cap);
	memcpy(ref, "1234567", len);
	written = fmt_u64_to_buf(1234567, out, len);

	FT_EQ(size_t, written, len);
	FT_EQ(buffer, ref, out, .size = cap);

	len = 10;
	memset(out, 17, cap);
	memset(ref, 17, cap);
	memcpy(ref, "18446744073709551615", len);
	written = fmt_u64_to_buf(18446744073709551615u, out, len);

	FT_EQ(size_t, written, len);
	FT_EQ(buffer, ref, out, .size = cap);
}

FT_TEST(fmt_u64_to_buf_less_than_cap) {
	const u64 cap = 12;
	u64		  len = 1;
	u8		  out[cap];
	u8		  ref[cap];

	len = 2;
	memset(out, 15, cap);
	memset(ref, 15, cap);
	memcpy(ref, "20", len);
	u64 written = fmt_u64_to_buf(20, out, cap);

	FT_EQ(size_t, written, 2);
	FT_EQ(buffer, ref, out, .size = cap);

	len = 7;
	memset(out, 16, cap);
	memset(ref, 16, cap);
	memcpy(ref, "1234567", len);
	written = fmt_u64_to_buf(1234567, out, cap);

	FT_EQ(size_t, written, 7);
	FT_EQ(buffer, ref, out, .size = cap);
}

FT_TEST(fmt_str_to_buf_less_than_cap) {
	const u64 cap = 12;
	u8		  out[cap];
	u8		  ref[cap];
	u64		  len = 1;
	char*	  data;

	len = 12;
	data = "abcdeF\0bad data";
	memset(out, 15, cap);
	memset(ref, 15, cap);
	memcpy(ref, data, strlen(data));
	u64 written = fmt_str_to_buf(data, out, len);

	FT_EQ(size_t, written, strlen(data));
	FT_EQ(buffer, ref, out, .size = cap);
}

FT_TEST(fmt_str_to_buf_more_than_cap) {
	const u64 cap = 12;
	u8		  out[cap];
	u8		  ref[cap];
	u64		  len = 1;
	char*	  data;

	len = 12;
	data = "abcdeddddddddddddddddF\0bad data";
	memset(out, 15, cap);
	memset(ref, 15, cap);
	memcpy(ref, data, len);
	u64 written = fmt_str_to_buf(data, out, len);

	FT_EQ(size_t, written, len);
	FT_EQ(buffer, ref, out, .size = cap);
}
