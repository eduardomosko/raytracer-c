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

f64	 sqrt(f64);	 // avoid including math.h
void exit(i32);	 // avoid including stdlib.h

// :fmt
u64 fmt_u64_to_buf(u64 v, u8* buf, u64 len);
u64 fmt_u8_to_buf(u8 v, u8* buf, u64 len);
u64 fmt_str_to_buf(const char* src, u8* buf, u64 len);

// :mem :str
u64 mcpy(const u8* src, u8* buf, u64 len);
u64 slen(const char* str);

// :allocator
typedef enum {
	AR_ALLOC,
	AR_REALLOC,
	AR_FREE,
} allocator_request_t;

typedef struct {
	void (*alloc)(void* state, allocator_request_t request, u64 size, void** data);
	void* state;
} allocator_t;

void		malloc_allocator_alloc(void* state, allocator_request_t request, u64 size, void** data);
allocator_t allocator_malloc_create();

void* allocator_alloc(allocator_t allocator, u64 size);
void  allocator_dealloc(allocator_t allocator, void* data);

// :context :ctx
typedef struct {
	allocator_t allocator;
} context_t;

context_t context_default();

void* alloc(context_t ctx, u64 size);
void  dealloc(context_t ctx, void* data);

// :error
typedef enum {
	NO_ERROR = 0,
	WRITE_ERROR,

	TGA_WIDTH_TOO_LARGE,
	TGA_HEIGHT_TOO_LARGE,
} error_t;

// :linalg
typedef struct {
	f64 x, y, z;
} vec3_t;

static inline f64* vec3_data(vec3_t* a) {
	return (f64*)a;
}

static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
	return (vec3_t){.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
	return (vec3_t){.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

static inline vec3_t vec3_neg(vec3_t a) {
	return (vec3_t){.x = -a.x, .y = -a.y, .z = -a.z};
}

static inline vec3_t vec3_mul(vec3_t a, f64 s) {
	return (vec3_t){.x = a.x * s, .y = a.y * s, .z = a.z * s};
}

static inline vec3_t vec3_div(vec3_t a, f64 s) {
	return vec3_mul(a, 1. / s);
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
	return (vec3_t){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static inline f64 vec3_dot(vec3_t a, vec3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline f64 vec3_len2(vec3_t a) {
	return vec3_dot(a, a);
}

static inline f64 vec3_len(vec3_t a) {
	return sqrt(vec3_len2(a));
}

static inline vec3_t vec3_norm(vec3_t a) {
	return vec3_div(a, vec3_len(a));
}

static inline void vec3p_add(vec3_t* a, vec3_t b) {
	*a = vec3_add(*a, b);
}

static inline void vec3p_sub(vec3_t* a, vec3_t b) {
	*a = vec3_sub(*a, b);
}

static inline void vec3p_neg(vec3_t* a) {
	*a = vec3_neg(*a);
}

static inline void vec3p_mul(vec3_t* a, f64 s) {
	*a = vec3_mul(*a, s);
}

static inline void vec3p_div(vec3_t* a, f64 s) {
	vec3p_mul(a, 1. / s);
}

static inline void vec3p_cross(vec3_t* a, vec3_t b) {
	*a = vec3_cross(*a, b);
}

static inline void vec3p_norm(vec3_t* a) {
	vec3_div(*a, vec3_len(*a));
}

static inline f64 clamp_f64(f64 v, f64 min, f64 max) {
	return v > max ? max : v < min ? min : v;
}

// :color
typedef struct {
	u8 r, g, b;
} color_t;

static inline color_t vec3_to_color(vec3_t a) {
	const f64 max = 0.9999;
	return (color_t){
		// clamp prevents overflow
		.r = (u8)(clamp_f64(a.x, 0, max) * 256),
		.g = (u8)(clamp_f64(a.y, 0, max) * 256),
		.b = (u8)(clamp_f64(a.z, 0, max) * 256),
	};
}

// :image

typedef struct {
	color_t* data;
	u64		 w, h;

	allocator_t _allocator;
} image_t;

image_t* image_create(context_t ctx, u64 w, u64 h);
void	 image_destroy(image_t* img);
error_t	 image_write_ppm(image_t* img, int fd);
error_t	 image_write_tga(image_t* img, int fd);

// :error :macro
#define try if ((
#define or_return )) return

#define STR2(str) STR(str)
#define STR(str) #str
#define STR_WITH_LEN(str) str, slen(str)

#define or_fail(message) )) {                                                            \
		write(1, STR_WITH_LEN("[ERROR] " __FILE__ ":" STR2(__LINE__) " " message "\n")); \
		exit(1);                                                                         \
	}                                                                                    \
	do {                                                                                 \
	} while (0)

#define or_failf(message, ...) )) {                                             \
		printf("[ERROR] %s:%d " message "\n", __FILE__, __LINE__, __VA_ARGS__); \
		exit(1);                                                                \
	};                                                                          \
	do {                                                                        \
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
