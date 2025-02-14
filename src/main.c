#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msk.h"

// :ray
typedef struct {
	vec3_t origin, direction;
} ray_t;

typedef struct {
	f64	   t;
	vec3_t point;
	vec3_t normal;
	bool   is_hit, is_front_face;
} hit_t;

typedef enum {
	SPHERE,
} hittable_type_t;

typedef struct {
	hittable_type_t type;

	vec3_t center;
	f64	   radius;
} sphere_t;

hit_t hit_sphere(sphere_t s, ray_t r, f64 mint, f64 maxt) {
	vec3_t oc = vecmath(s.center - r.origin);

	f64 a = vec3_len2(r.direction);
	// f64 b = -2. * vec3_dot(r.direction, oc);
	f64 h = vec3_dot(r.direction, oc);
	f64 c = vec3_len2(oc) - s.radius * s.radius;

	f64 discriminant = h * h - a * c;

	if (discriminant < 0) {
		return (hit_t){.is_hit = false};
	}

	f64 sqrt_ = sqrt(discriminant);

	// for the closest point in range
	// f64 root = (-b +- sqrt_) / 2 * a;
	f64 root = (h - sqrt_) / a;

	if (root <= mint || maxt <= root) {
		root = (h + sqrt_) / a;

		if (root <= mint || maxt <= root) {
			return (hit_t){.is_hit = false};  // no hit in range
		}
	}

	hit_t hit = {.is_hit = true};
	hit.t = root;
	hit.point = vecmath(r.origin + r.direction * root);

	vec3_t outward_normal = vecmath((hit.point - s.center) / s.radius);
	hit.is_front_face = vec3_dot(r.direction, outward_normal);
	hit.normal = hit.is_front_face ? outward_normal : vec3_neg(outward_normal);

	return hit;
}

typedef union {
	hittable_type_t type;
	sphere_t		sphere;
} hittable_t;

hit_t hit_hittable(hittable_t h, ray_t r, f64 mint, f64 maxt) {
	switch (h.type) {
		case SPHERE:
			return hit_sphere(h.sphere, r, mint, maxt);
	}
	unreachable;
}

typedef view_of(hittable_t) hittable_view_t;

hit_t hit_many(hittable_view_t hs, ray_t r, f64 mint, f64 maxt) {
	hit_t result = {0};

	for (u64 i = 0; i < hs.count; i++) {
		hit_t this_hit = hit_hittable(hs.items[i], r, mint, maxt);
		if (this_hit.is_hit) {
			result = this_hit;
			maxt = this_hit.t;
		}
	}

	return result;
}

f64 rand_f64() {
	f64 val = rand();
	f64 max = RAND_MAX;
	return val / max;
}

vec3_t vec3_rand(f64 min, f64 max) {
	f64 d = max - min;
	return (vec3_t){
		.x = rand_f64() * d + min,
		.y = rand_f64() * d + min,
		.z = rand_f64() * d + min,
	};
}

vec3_t vec3_rand_unit() {
	while (true) {
		vec3_t vec = vec3_rand(-1, 1);
		f64	   len2 = vec3_len2(vec);
		if (-1e160 < len2 && len2 <= 1) {
			return vec3_div(vec, sqrt(len2));
		}
	}
}
vec3_t vec3_rand_hemisphere(vec3_t normal) {
	vec3_t rand = vec3_rand_unit();
	f64	   sign = vec3_dot(rand, normal);
	return sign < 0 ? vec3_neg(rand) : rand;
}

vec3_t gamma_correction(vec3_t v) {
	return (vec3_t){
		.x = v.x > 0 ? sqrt(v.x) : 0,
		.y = v.y > 0 ? sqrt(v.y) : 0,
		.z = v.z > 0 ? sqrt(v.z) : 0,
	};
}

vec3_t ray_color(ray_t ray, hittable_view_t world, i32 max_bouces) {
	if (max_bouces <= 0) {
		return (vec3_t){0};
	}

	hit_t hit = hit_many(world, ray, 0.00001, 10);
	if (hit.is_hit) {
		ray_t next_ray = {
			.origin = hit.point,
			.direction = vec3_rand_hemisphere(hit.normal),
		};
		vec3_t color = ray_color(next_ray, world, max_bouces - 1);
		return vecmath(color * 0.5);
	}

	vec3_t direction = vec3_norm(ray.direction);

	f64 a = (direction.y + 1.) / 2.;

	vec3_t white = {1, 1, 1};
	vec3_t blue = {0.5, 0.7, 1.0};

	f64 a_inv = 1. - a;
	return vecmath(white * a_inv + blue * a);
}

int main(void) {
	context_t ctx = context_default();

	f64 aspect_ratio = 16.0 / 9.0;
#define width 256

	u64 height = width / aspect_ratio;
	height = (height < 1) ? 1 : height;

	// Camera
	vec3_t camera_center = {0, 0, 1};
	vec3_t focal_length = {0, 0, 2.0};

	// Viewport
	f64 viewport_height = 2.0;
	f64 viewport_width = viewport_height * ((f64)width / height);

	vec3_t viewport_u = {viewport_width, 0, 0};
	vec3_t viewport_v = {0, -viewport_height, 0};

	vec3_t pix_delta_u = vecmath(viewport_u / width);
	vec3_t pix_delta_v = vecmath(viewport_v / height);

	vec3_t viewport_upper_left = vecmath(camera_center - focal_length - (viewport_u + viewport_v) / 2);
	vec3_t pix00_location = vecmath(viewport_upper_left + (pix_delta_u + pix_delta_v) / 2);

	// World
	hittable_t world[] = {
		((hittable_t){.sphere = {SPHERE, (vec3_t){0, 0, -1}, 0.5}}),
		((hittable_t){.sphere = {SPHERE, (vec3_t){-1, 0, -1}, 0.4}}),
		((hittable_t){.sphere = {SPHERE, (vec3_t){0, -100.5, -1}, 100}}),
	};
	hittable_view_t world_view = {.items = world, .count = sizeof(world)/sizeof(hittable_t)};

	image_t* img = image_create(ctx, width, height);

	// render the image

	for (u64 i = 0; i < height; i++) {
		for (u64 j = 0; j < width; j++) {
			vec3_t pix_center = pix00_location;
			vec3p_add(&pix_center, vec3_mul(pix_delta_u, i));
			vec3p_add(&pix_center, vec3_mul(pix_delta_v, j));

			vec3_t color = {0};
			u64	   rays = 0;

			const f64 ysamples = 20.;
			const f64 xsamples = 20.;

			for (i64 ry = 0; ry < 20; ry++) {
				for (i64 rx = 0; rx < 20; rx++) {
					vec3_t center = pix_center;
					vec3p_add(&center, vec3_mul(pix_delta_u, ry / ysamples - 0.5));
					vec3p_add(&center, vec3_mul(pix_delta_v, rx / xsamples - 0.5));

					vec3_t ray_dir = vec3_sub(center, camera_center);

					vec3_t pix_color = ray_color(
						(ray_t){
							.origin = camera_center,
							.direction = ray_dir,
						},
						world_view, 100);

					vec3p_add(&color, pix_color);
					rays++;
				}
			}
			vec3p_div(&color, rays);
			color = gamma_correction(color);

			img->data[i * img->w + j] = vec3_to_color(color);
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
