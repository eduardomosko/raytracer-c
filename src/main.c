#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "msk.h"

// Allocator
typedef enum {
	AR_ALLOC,
	AR_REALLOC,
	AR_FREE,
} allocator_request_t;

typedef struct {
	void (*alloc)(void* state, allocator_request_t request, u64 size, void** data);
	void* state;
} allocator_t;

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

void allocator_dealloc(allocator_t allocator, void* data) {
	allocator.alloc(allocator.state, AR_FREE, 0, &data);
}

// Context system
typedef struct {
	allocator_t allocator;
} context_t;

context_t context_default() {
	return (context_t){
		.allocator = allocator_malloc_create(),
	};
}

void* alloc(context_t ctx, u64 size) {
	return allocator_alloc(ctx.allocator, size);
}

void dealloc(context_t ctx, void* data) {
	return allocator_dealloc(ctx.allocator, data);
}

// Image writing procedures
typedef struct {
	u8 r, g, b;
} color_t;

typedef struct {
	color_t* data;
	u64		 w, h;

	allocator_t _allocator;
} image_t;

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

error image_write_ppm(image_t* img, int fd) {
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
				try((u64)write(fd, buffer, i)) != i or_return WRITE_ERROR;
				i = 0;
			}
		}
	}

	if (i > 0) {
		try((u64)write(fd, buffer, i)) != i or_return WRITE_ERROR;
	}

	return NO_ERROR;
}

error image_write_tga(image_t* img, int fd) {
	try img->w > U16_MAX or_return TGA_WIDTH_TOO_LARGE;
	try img->h > U16_MAX or_return TGA_HEIGHT_TOO_LARGE;

	const u8 TOP_TO_BOTTOM = (1 << 5);
	const u8 RIGHT_TO_LEFT = (1 << 4);

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
		try(u64) write(fd, buffer, i) != i or_return WRITE_ERROR;
	}

	if (i != 0) {
		try(u64) write(fd, buffer, i) != i or_return WRITE_ERROR;
	}

	return NO_ERROR;
}

int main(void) {
	context_t ctx = context_default();

	image_t* img = image_create(ctx, 1000, 1000);

	for (u64 y = 0; y < img->h; y++) {
		for (u64 x = 0; x < img->w; x++) {
			color_t* c = &img->data[x + y * img->w];
			c->r = 255;
		}
	}

	for (u64 y = 25; y < img->h - 25; y++) {
		for (u64 x = 25; x < img->w - 25; x++) {
			color_t* c = &img->data[x + y * img->w];
			c->r = 0;
			c->b = 255;
		}
	}

	for (u64 y = 50; y < img->h - 50; y++) {
		for (u64 x = 50; x < img->w - 50; x++) {
			color_t* c = &img->data[x + y * img->w];
			c->b = 0;
			c->g = 255;
		}
	}

	/* create ppm */ {
		const int fd = open("output.ppm", O_CREAT | O_WRONLY, 0644);

		try image_write_ppm(img, fd) or_error("failed writing ppm");
		close(fd);
	}

	/* create tga */ {
		const int fd = open("output.tga", O_CREAT | O_WRONLY, 0644);

		try image_write_tga(img, fd) or_error("failed writing tga");
		close(fd);
	}

	image_destroy(img);
	// write(STDOUT_FILENO, "-\n-\n-\n", 6);

	return 0;
}
