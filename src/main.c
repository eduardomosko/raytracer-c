#include <fcntl.h>
#include <unistd.h>

#include "msk.h"

// :ray
typedef struct {
	vec3_t origin, direction;
} ray_t;

color_t ray_color(ray_t ray) {
	vec3_t direction = vec3_norm(ray.direction);

	f64 a = (direction.y + 1.) / 2.;

	vec3_t white = {1, 1, 1};
	vec3_t blue = {0.5, 0.7, 1.0};

	vec3p_mul(&white, 1. - a);
	vec3p_mul(&blue, a);
	return vec3_to_color(vec3_add(white, blue));
}

#define vecmath(expression) ((vec3_t){})
#define vecmath_f64(expression) ((f64)(0))

int main(void) {
	context_t ctx = context_default();

	f64 aspect_ratio = 16.0 / 9.0;
	u64 width = 400;

	u64 height = width / aspect_ratio;
	height = (height < 1) ? 1 : height;

	// Camera
	vec3_t camera_center = {0};
	f64	   focal_length = 1.0;

	// Viewport
	f64 viewport_height = 2.0;
	f64 viewport_width = viewport_height * ((f64)width / height);

	vec3_t viewport_u = {viewport_width, 0, 0};
	vec3_t viewport_v = {0, -viewport_height, 0};

	vec3_t pix_delta_u = vecmath(viewport_u / width);
	vec3_t pix_delta_v = vecmath(viewport_v / height);

	// vec3_t viewport_upper_left = camera_center;
	// vec3p_sub(&viewport_upper_left, (vec3_t){0, 0, focal_length});
	// vec3p_sub(&viewport_upper_left, vec3_div(viewport_u, 2));
	// vec3p_sub(&viewport_upper_left, vec3_div(viewport_v, 2));

	vec3_t viewport_upper_left = vecmath(camera_center - ((vec3_t){0, 0, focal_length}) - (viewport_u + viewport_v) / 2);

	vec3_t pix00_location = vec3_add(viewport_upper_left, vec3_div(vec3_add(pix_delta_u, pix_delta_v), 2));

	image_t* img = image_create(ctx, width, height);

	// render the image
	for (u64 i = 0; i < img->w; i++) {
		for (u64 j = 0; j < img->h; j++) {
			vec3_t pix_center = pix00_location;
			vec3p_add(&pix_center, vec3_mul(pix_delta_u, i));
			vec3p_add(&pix_center, vec3_mul(pix_delta_v, j));

			vec3_t ray_dir = vec3_sub(pix_center, camera_center);

			color_t pix_color = ray_color((ray_t){
				.origin = camera_center,
				.direction = ray_dir,
			});

			img->data[i + j * img->w] = pix_color;
		}
	}

	/* create tga */ {
		const int fd = open("output.tga", O_CREAT | O_WRONLY, 0644);

		try image_write_tga(img, fd) or_fail("failed writing tga");
		close(fd);
	}

	image_destroy(img);
	// write(STDOUT_FILENO, "-\n-\n-\n", 6);

	return 0;
}
