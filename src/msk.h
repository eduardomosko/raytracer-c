#ifndef MSK_H
#define MSK_H

#include <byteswap.h>
#include <stdbool.h>
#include <stdint.h>

// :number :int :float
typedef uint8_t	 u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef double f64;
typedef float  f32;

#define U8_MAX ((1 << 8) - 1)
#define U16_MAX ((1 << 16) - 1)
#define U32_MAX ((1 << 32) - 1)
#define U64_MAX ((1 << 64) - 1)

#define null (void*)(0)

// :fmt
u64 fmt_u64_to_buf(u64 v, u8* buf, u64 len);
u64 fmt_u8_to_buf(u8 v, u8* buf, u64 len);
u64 fmt_str_to_buf(const char* src, u8* buf, u64 len);

// :mem
u64 mcpy(const u8* src, u8* buf, u64 len);

// :str
static inline u64 slen(const char* str) {
	u64 i = 0;
	while (*str++) {
		i++;
	}
	return i;
}

// :error
typedef enum {
	NO_ERROR = 0,
	WRITE_ERROR,

	TGA_WIDTH_TOO_LARGE,
	TGA_HEIGHT_TOO_LARGE,
} error_t;

#define error error_t

#define try if ((
#define or_return )) return

#define STR2(str) STR(str)
#define STR(str) #str
#define STR_WITH_LEN(str) str, slen(str)

#define or_error(message) )) {                                                           \
		write(1, STR_WITH_LEN("[ERROR] " __FILE__ ":" STR2(__LINE__) " " message "\n")); \
		exit(1);                                                                         \
	};                                                                                   \
	do {                                                                                 \
	} while (0)

#define or_errorf(message, ...) )) {                                             \
		printf("[ERROR] %s:%d: " message "\n", __FILE__, __LINE__, __VA_ARGS__); \
		exit(1);                                                                 \
	};                                                                           \
	do {                                                                         \
	} while (0)

// :endian
static inline u16 cpu_to_be16(u16 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return __bswap_16(v);
#else
	return v;
#endif
}
static inline u32 cpu_to_be32(u32 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return __bswap_32(v);
#else
	return v;
#endif
}
static inline u64 cpu_to_be64(u64 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return __bswap_64(v);
#else
	return v;
#endif
}
static inline u16 cpu_to_le16(u16 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return v;
#else
	return __bswap_16(v);
#endif
}
static inline u32 cpu_to_le32(u32 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return v;
#else
	return __bswap_32(v);
#endif
}
static inline u64 cpu_to_le64(u64 v) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return v;
#else
	return __bswap_64(v);
#endif
}

#endif	// MSK_H
