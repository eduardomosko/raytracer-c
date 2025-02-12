#include <fcntl.h>
#include <unistd.h>

#include "msk.h"

// :ray
typedef struct {
	vec3_t origin, direction;
} ray_t;

bool hit_sphere(vec3_t center, f64 radius, ray_t r) {
	vec3_t oc = vecmath(center - r.origin);

	f64 a = vec3_len2(r.direction);
	f64 b = -2. * vec3_dot(r.direction, oc);
	f64 c = vec3_len2(oc) - radius * radius;

	f64 discriminant = b * b - 4 * a * c;
	return discriminant >= 0;
}

color_t ray_color(ray_t ray) {
	if (hit_sphere((vec3_t){0, 0, -1}, 0.5, ray)) {
		return (color_t){255, 0, 0};
	}

	vec3_t direction = vec3_norm(ray.direction);

	f64 a = (direction.y + 1.) / 2.;

	vec3_t white = {1, 1, 1};
	vec3_t blue = {0.5, 0.7, 1.0};

	f64 a_inv = 1. - a;
	return vec3_to_color(vecmath(white * a_inv + blue * a));
}

int main(void) {
	context_t ctx = context_default();

	f64 aspect_ratio = 16.0 / 9.0;
	u64 width = 400;

	u64 height = width / aspect_ratio;
	height = (height < 1) ? 1 : height;

	// Camera
	vec3_t camera_center = {0};
	vec3_t focal_length = {0, 0, 1.0};

	// Viewport
	f64 viewport_height = 2.0;
	f64 viewport_width = viewport_height * ((f64)width / height);

	vec3_t viewport_u = {viewport_width, 0, 0};
	vec3_t viewport_v = {0, -viewport_height, 0};

	vec3_t pix_delta_u = vecmath(viewport_u / width);
	vec3_t pix_delta_v = vecmath(viewport_v / height);

	vec3_t viewport_upper_left = vecmath(camera_center - focal_length - (viewport_u + viewport_v) / 2);
	vec3_t pix00_location = vecmath(viewport_upper_left + (pix_delta_u + pix_delta_v) / 2);

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
