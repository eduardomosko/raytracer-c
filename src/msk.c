#include <stdlib.h>
#include <unistd.h>

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

// :str
u64 slen(const char* str) {
	u64 i = 0;
	while (*str++) {
		i++;
	}
	return i;
}

// :allocator
void malloc_allocator_alloc(void* state, allocator_request_t request, u64 size, void** data) {
	(void)(state);
	switch (request) {
		case AR_ALLOC: {
			*data = calloc(1, size);
		}; break;
		case AR_REALLOC: {
			*data = realloc(*data, size);
		}; break;
		case AR_FREE: {
			free(*data);
			*data = null;
		}; break;
	}
}

allocator_t allocator_malloc_create() {
	return (allocator_t){
		.state = null,
		.alloc = malloc_allocator_alloc,
	};
}

void* allocator_alloc(allocator_t allocator, u64 size) {
	void* out = null;
	allocator.alloc(allocator.state, AR_ALLOC, size, &out);
	return out;
}

void* allocator_ralloc(allocator_t allocator, void* data, u64 size) {
	allocator.alloc(allocator.state, AR_REALLOC, size, &data);
	return data;
}

void allocator_dealloc(allocator_t allocator, void* data) {
	allocator.alloc(allocator.state, AR_FREE, 0, &data);
}

context_t context_default() {
	return (context_t){
		.allocator = allocator_malloc_create(),
	};
}

void* alloc(context_t ctx, u64 size) {
	return allocator_alloc(ctx.allocator, size);
}

void* ralloc(context_t ctx, void* data, u64 size) {
	return allocator_ralloc(ctx.allocator, data, size);
}

void dealloc(context_t ctx, void* data) {
	return allocator_dealloc(ctx.allocator, data);
}

// :image
image_t* image_create(context_t ctx, u64 w, u64 h) {
	image_t* img = alloc(ctx, sizeof(image_t) + (w * h * sizeof(color_t)));
	img->w = w;
	img->h = h;
	img->data = (color_t*)(img + 1);
	img->_allocator = ctx.allocator;
	return img;
}

void image_destroy(image_t* img) {
	allocator_dealloc(img->_allocator, img);
}

error_t image_write_ppm(image_t* img, int fd) {
	const u64 cap = 1024;
	u8		  buffer[cap];
	u64		  i = 0;

	// write ppm header
	i += fmt_str_to_buf("P3\n", buffer + i, cap - i);
	i += fmt_u64_to_buf(img->w, buffer + i, cap - i);
	i += fmt_str_to_buf(" ", buffer + i, cap - i);
	i += fmt_u64_to_buf(img->h, buffer + i, cap - i);
	i += fmt_str_to_buf("\n255\n", buffer + i, cap - i);  // color depth

	for (u64 y = 0; y < img->h; y++) {
		for (u64 x = 0; x < img->w; x++) {
			color_t c = img->data[x + y * img->w];

			// one row per pixel
			i += fmt_u8_to_buf(c.r, buffer + i, cap - i);
			i += fmt_str_to_buf(" ", buffer + i, cap - i);
			i += fmt_u8_to_buf(c.g, buffer + i, cap - i);
			i += fmt_str_to_buf(" ", buffer + i, cap - i);
			i += fmt_u8_to_buf(c.b, buffer + i, cap - i);
			i += fmt_str_to_buf("\n", buffer + i, cap - i);

			// if less remaining than max needed, flush
			if (cap - i < 3 * 3 + 3) {
				try i != (u64)write(fd, buffer, i) or_return WRITE_ERROR;
				i = 0;
			}
		}
	}

	if (i > 0) {
		try i != (u64)write(fd, buffer, i) or_return WRITE_ERROR;
	}

	return NO_ERROR;
}

error_t image_write_tga(image_t* img, int fd) {
	try img->w > U16_MAX or_return TGA_WIDTH_TOO_LARGE;
	try img->h > U16_MAX or_return TGA_HEIGHT_TOO_LARGE;

	const u8 TOP_TO_BOTTOM = (1 << 5);
	(void)(TOP_TO_BOTTOM);

	const u8 RIGHT_TO_LEFT = (1 << 4);
	(void)(RIGHT_TO_LEFT);

	struct {
		u8 id_length;
		u8 color_map_type;
		u8 image_type;
		u8 color_map_spec[5];

		// image spec
		u16 x_origin;
		u16 y_origin;
		u16 width;
		u16 height;
		u8	pix_depth;
		u8	img_descriptor;
	} header = {
		.id_length = 0,			// no id
		.color_map_type = 0,	// no color map
		.image_type = 2,		// uncompressed true-color image
		.color_map_spec = {0},	// no color map

		// image spec
		.x_origin = 0,
		.y_origin = 0,
		.width = cpu_to_le16(img->w),
		.height = cpu_to_le16(img->h),
		.pix_depth = 24,
		.img_descriptor = TOP_TO_BOTTOM,
	};

	// write header
	try write(fd, &header, sizeof(header)) != sizeof(header) or_return WRITE_ERROR;

	const u64 cap = 512 * 3;
	u8		  buffer[cap];

	u64 i = 0;
	for (u64 px = 0; px < img->w * img->h;) {
		for (i = 0; i < cap; i += 3) {
			// data is BRG, not RGB
			buffer[i + 0] = img->data[px].b;
			buffer[i + 1] = img->data[px].g;
			buffer[i + 2] = img->data[px].r;
			px++;
		}

		// flush
		try i != (u64)write(fd, buffer, i) or_return WRITE_ERROR;
	}

	if (i != 0) {
		try i != (u64)write(fd, buffer, i) or_return WRITE_ERROR;
	}

	return NO_ERROR;
}
