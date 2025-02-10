#include "msk.h"

// :fmt
u64 fmt_u64_to_buf(u64 v, u8* buf, u64 len) {
	u64	 i = 0;
	u64	 cut = 10000000000000000000u;
	bool started = false;

	while (cut != 0 && i < len) {
		u8 digit = v / cut;
		v %= cut;
		cut /= 10;

		if (digit != 0) {
			started = true;
		}
		if (started) {
			buf[i++] = digit + '0';
		}
	}
	if (i == 0) {
		buf[i++] = '0';
	}
	return i;
}

u64 fmt_u8_to_buf(u8 v, u8* buf, u64 len) {
	u64	 i = 0;
	u8	 cut = 100u;
	bool started = false;

	while (cut != 0 && i < len) {
		u8 digit = v / cut;
		v %= cut;
		cut /= 10;

		if (digit != 0) {
			started = true;
		}
		if (started) {
			buf[i++] = digit + '0';
		}
	}
	if (i == 0) {
		buf[i++] = '0';
	}
	return i;
}

u64 fmt_str_to_buf(const char* src, u8* buf, u64 len) {
	u64 i = 0;
	while (i < len && *src != '\0') {
		buf[i++] = *src++;
	}
	return i;
}

// :mem
u64 mcpy(const u8* src, u8* buf, u64 len) {
	u64 i = 0;
	while (i < len) {
		buf[i] = src[i];
		i++;
	}
	return i;
}
