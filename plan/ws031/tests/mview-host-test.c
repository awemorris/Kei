/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Host test of the mview model reader, camera and input translation.
 *
 * It reads the committed test fixture and, when present, the converted qs40
 * model, feeds malformed models to the reader, and checks that the camera
 * frames the model and that a reset restores the first view bit for bit.
 */

#include "../../../userland/tests/mview/model.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * One malformed model and the text its refusal must contain.
 *
 * The body replaces model.txt in a scratch directory that also holds a copy
 * of the fixture's textures.
 */
struct bad_case {
	const char *name;
	const char *body;
	const char *reason;
};

/* The number of failed checks; the test passes when it stays zero. */
static int failures;

/* The number of checks performed, reported with the verdict. */
static int checks;

/* The malformed models; each must be refused with its stated reason. */
static const struct bad_case bad_cases[] = {
	{ "empty", "", "missing 'mview 1'" },
	{ "version", "mview 2\n", "expected '1'" },
	{ "keyword", "mview 1\nsphere 1\n", "unknown keyword" },
	{ "texture-order", "mview 1\ntexture 1 tex/0.pam 8 8\n", "out of order" },
	{ "texture-escape", "mview 1\ntexture 0 ../x.pam 8 8\n", "plain relative path" },
	{ "texture-absolute", "mview 1\ntexture 0 /etc/passwd 8 8\n", "plain relative path" },
	{ "material-texture", "mview 1\nmaterial 0 m texture 3 alpha opaque cull back color 1 1 1 1\n", "undeclared texture" },
	{ "material-alpha", "mview 1\nmaterial 0 m texture - alpha glass cull back color 1 1 1 1\n", "unknown alpha" },
	{ "material-color", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 2 1 1\n", "outside 0..1" },
	{ "vertex-outside", "mview 1\nv 0 0 0 0 0 1 0 0\n", "outside a mesh" },
	{ "vertex-nan", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 1 triangles 0\nv nan 0 0 0 0 1 0 0\n", "finite" },
	{ "vertex-extra", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 1 triangles 0\nv 0 0 0 0 0 1 0 0\nv 0 0 0 0 0 1 0 0\n", "more vertices" },
	{ "index-range", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 3\n", "outside the mesh" },
	{ "index-sign", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 -2\n", "not a decimal" },
	{ "triangle-material", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 1 0 1 2\n", "undeclared material" },
	{ "mesh-short", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\n", "vertices" },
	{ "mesh-count", "mview 1\nmesh a vertices 99999999999 triangles 1\n", "exceeds" },
	{ "no-triangles", "mview 1\nmaterial 0 m texture - alpha opaque cull back color 1 1 1 1\n", "no triangles" },
	{ "bounds-inverted", "mview 1\nbounds 1 0 0 0 1 1\n", "minimum exceeds" },
	{ "texture-size", "mview 1\ntexture 0 tex/0.pam 16 8\nmaterial 0 m texture 0 alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 2\n", "differs" },
	{ "texture-missing", "mview 1\ntexture 0 tex/9.pam 8 8\nmaterial 0 m texture 0 alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 2\n", "tex/9.pam" },
	{ "texture-truncated", "mview 1\ntexture 0 tex/short.pam\nmaterial 0 m texture 0 alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 2\n", "truncated" },
	{ "texture-format", "mview 1\ntexture 0 tex/gray.pam\nmaterial 0 m texture 0 alpha opaque cull back color 1 1 1 1\nmesh a vertices 3 triangles 1\nv 0 0 0 0 0 1 0 0\nv 1 0 0 0 0 1 0 0\nv 0 1 0 0 0 1 0 0\nt 0 0 1 2\n", "not an 8-bit" }
};

static void check(int condition, const char *what);
static void test_fixture(const char *directory);
static void test_bad_cases(const char *fixture, const char *scratch);
static void test_camera(const char *directory);
static void test_qs40(const char *directory);
static int write_file(const char *path, const char *body, size_t bytes);
static int copy_file(const char *source, const char *destination);
static int project(const struct mview_push *push, const float point[3], float clip[4]);

/*
 * Runs every check and reports a one-line verdict.
 */
int
main(
	int argc,
	char **argv)
{
	/* The fixture, a scratch directory and the optional real model are given. */
	if (argc != 4) {
		fprintf(stderr, "usage: mview-host-test FIXTURE_DIR SCRATCH_DIR QS40_DIR\n");
		return 2;
	}

	/* Keeps the check log in order with the viewer's own flushed lines. */
	setvbuf(stdout, NULL, _IOLBF, 0);

	/* Each group of checks is independent of the others. */
	test_fixture(argv[1]);
	test_bad_cases(argv[1], argv[2]);
	test_camera(argv[1]);
	test_qs40(argv[3]);

	/* A failed check has already been printed. */
	if (failures != 0) {
		printf("MVIEW HOST FAIL checks=%d failures=%d\n", checks, failures);
		return 1;
	}

	/* Succeeded: every check held. */
	printf("MVIEW HOST PASS checks=%d\n", checks);
	return 0;
}

/* Counts one check and prints it when it fails. */
static void
check(
	int condition,
	const char *what)
{
	/* Every check is counted, passed or not. */
	checks++;
	if (condition == 0) {
		failures++;
		printf("FAIL: %s\n", what);
	}

	/* Succeeded: the check is recorded. */
	return;
}

/* Checks the committed fixture's counts, groups, rebasing and texels. */
static void
test_fixture(
	const char *directory)
{
	struct mview_model model;
	uint32_t index;
	int status;
	int rebased;

	/* The fixture is a well-formed model and must load. */
	status = mview_model_load(&model, directory);
	if (status != 0)
		printf("fixture error: %s\n", model.error);

	check(status == 0, "fixture loads");
	if (status != 0) {
		mview_model_free(&model);
		return;
	}

	/* Counts match the fixture's text. */
	check(model.texture_count == 2U, "fixture has 2 textures");
	check(model.material_count == 5U, "fixture has 5 materials");
	check(model.mesh_count == 2U, "fixture has 2 meshes");
	check(model.vertex_count == 28U, "fixture has 28 vertices");
	check(model.triangle_count == 14U, "fixture has 14 triangles");

	/* Groups are opaque (0, 4), cutout (1), blend (2); unused material 3 is skipped. */
	check(model.group_count == 4U, "fixture has 4 draw groups");
	check(model.groups[0].material == 0U && model.groups[0].first == 0U && model.groups[0].count == 12U, "group 0 is material 0");
	check(model.groups[1].material == 4U && model.groups[1].first == 12U && model.groups[1].count == 6U, "group 1 is material 4");
	check(model.groups[2].material == 1U && model.groups[2].first == 18U && model.groups[2].count == 12U, "group 2 is material 1");
	check(model.groups[3].material == 2U && model.groups[3].first == 30U && model.groups[3].count == 12U, "group 3 is material 2");

	/* The floor mesh's local indices 0..3 were rebased onto vertices 24..27. */
	rebased = 1;
	for (index = 12U; index < 18U; index++) {
		if (model.indices[index] < 24U || model.indices[index] > 27U)
			rebased = 0;
	}

	check(rebased, "second mesh indices are rebased");
	check(model.indices[12] == 24U && model.indices[13] == 25U && model.indices[14] == 26U, "floor triangle keeps its winding");

	/* Materials carry their parsed attributes. */
	check(model.materials[1].alpha == MVIEW_ALPHA_CUTOUT && model.materials[1].cull == MVIEW_CULL_NONE, "material 1 is cutout, cull none");
	check(model.materials[2].texture == -1 && model.materials[2].alpha == MVIEW_ALPHA_BLEND, "material 2 is untextured blend");
	check(model.materials[2].color[3] == 0.5f, "material 2 alpha is 0.5");

	/* The bounds come from the file. */
	check(model.minimum[1] == -0.02f && model.maximum[0] == 1.0f, "bounds are read");

	/* The first texel of texture 0 is the red marker; texture 1 has transparent texels. */
	check(model.textures[0].width == 8U && model.textures[0].height == 8U, "texture 0 is 8x8");
	check(memcmp(model.textures[0].pixels, "\xff\x00\x00\xff", 4) == 0, "texture 0 top-left texel is red");
	check(model.textures[1].pixels[(1U * 4U + 0U) * 4U + 3U] == 0U, "texture 1 has a transparent texel");
	check(model.textures[1].pixels[(0U * 4U + 0U) * 4U + 3U] == 255U, "texture 1 has an opaque texel");

	/* Every normal was normalized. */
	check(fabsf(model.vertices[0].normal[2] - 1.0f) < 1.0e-6f, "normal is unit length");

	/* Freeing twice is harmless. */
	mview_model_free(&model);
	mview_model_free(&model);
	check(model.vertices == NULL, "free clears the model");

	/* Succeeded: the fixture checks are recorded. */
	return;
}

/* Feeds every malformed model to the reader and checks the stated refusal. */
static void
test_bad_cases(
	const char *fixture,
	const char *scratch)
{
	struct mview_model model;
	char path[1024];
	char source[1024];
	char long_line[2048];
	char what[512];
	static const char short_pam[] = "P7\nWIDTH 4\nHEIGHT 4\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\nabcd";
	static const char gray_pam[] = "P7\nWIDTH 1\nHEIGHT 1\nDEPTH 1\nMAXVAL 255\nTUPLTYPE GRAYSCALE\nENDHDR\na";
	uint32_t index;
	int status;

	/* The scratch directory holds a copy of the fixture textures and two broken ones. */
	snprintf(path, sizeof(path), "%s/tex", scratch);
	mkdir(scratch, 0755);
	mkdir(path, 0755);
	snprintf(source, sizeof(source), "%s/tex/0.pam", fixture);
	snprintf(path, sizeof(path), "%s/tex/0.pam", scratch);
	status = copy_file(source, path);
	check(status == 0, "scratch texture copied");

	snprintf(path, sizeof(path), "%s/tex/short.pam", scratch);
	write_file(path, short_pam, sizeof(short_pam) - 1U);
	snprintf(path, sizeof(path), "%s/tex/gray.pam", scratch);
	write_file(path, gray_pam, sizeof(gray_pam) - 1U);

	/* Each case must be refused, with its reason in the error text. */
	for (index = 0U; index < sizeof(bad_cases) / sizeof(bad_cases[0]); index++) {
		snprintf(path, sizeof(path), "%s/model.txt", scratch);
		write_file(path, bad_cases[index].body, strlen(bad_cases[index].body));
		status = mview_model_load(&model, scratch);
		snprintf(what, sizeof(what), "bad case %s is refused", bad_cases[index].name);
		check(status != 0, what);
		snprintf(what, sizeof(what), "bad case %s reports '%s' (got '%s')", bad_cases[index].name, bad_cases[index].reason, model.error);
		check(strstr(model.error, bad_cases[index].reason) != NULL, what);
		mview_model_free(&model);
	}

	/* An over-long line is refused rather than split. */
	snprintf(path, sizeof(path), "%s/model.txt", scratch);
	memset(long_line, 'x', sizeof(long_line));
	memcpy(long_line, "mview 1\n", 8U);
	long_line[sizeof(long_line) - 1U] = '\n';
	write_file(path, long_line, sizeof(long_line));
	status = mview_model_load(&model, scratch);
	check(status != 0 && strstr(model.error, "longer than") != NULL, "over-long line is refused");
	mview_model_free(&model);

	/* Succeeded: every malformed model has been tried. */
	return;
}

/* Checks the initial fit, the projection and bit-exact reset. */
static void
test_camera(
	const char *directory)
{
	struct mview_model model;
	struct mview_camera camera;
	struct mview_camera initial;
	struct mview_input input;
	struct mview_push first;
	struct mview_push push;
	float corner[3];
	float clip[4];
	float center[3];
	float screen[3][2];
	float area;
	uint32_t index;
	int inside;
	int status;

	/* The camera is fitted to the fixture at the harness size. */
	status = mview_model_load(&model, directory);
	if (status != 0)
		printf("fixture error: %s\n", model.error);

	check(status == 0, "fixture loads for the camera test");
	if (status != 0) {
		mview_model_free(&model);
		return;
	}

	mview_camera_fit(&camera, model.minimum, model.maximum, 640U, 480U);
	initial = camera;
	mview_camera_push(&camera, 640U, 480U, &first);

	/* The bounds centre projects to the middle of the screen. */
	center[0] = camera.center[0];
	center[1] = camera.center[1];
	center[2] = camera.center[2];
	project(&first, center, clip);
	check(fabsf(clip[0] / clip[3]) < 1.0e-5f && fabsf(clip[1] / clip[3]) < 1.0e-5f, "centre projects to the screen centre");

	/* Every corner of the bounds is inside the view volume. */
	inside = 1;
	for (index = 0U; index < 8U; index++) {
		corner[0] = (index & 1U) ? model.maximum[0] : model.minimum[0];
		corner[1] = (index & 2U) ? model.maximum[1] : model.minimum[1];
		corner[2] = (index & 4U) ? model.maximum[2] : model.minimum[2];
		project(&first, corner, clip);
		if (fabsf(clip[0]) > clip[3] || fabsf(clip[1]) > clip[3] || clip[2] < 0.0f || clip[2] > clip[3])
			inside = 0;
	}

	check(inside, "initial view contains the whole model");

	/* The model's top is drawn above its bottom: +Y is up on the screen. */
	corner[0] = camera.center[0];
	corner[1] = model.maximum[1];
	corner[2] = camera.center[2];
	project(&first, corner, clip);
	check(clip[1] / clip[3] < 0.0f, "model top is in the upper half of the screen");

	/* The front (+Z) is nearer to the viewer than the back. */
	corner[1] = camera.center[1];
	corner[2] = model.maximum[2];
	project(&first, corner, clip);
	center[2] = model.minimum[2];
	project(&first, center, push.clip);
	check(clip[2] / clip[3] < push.clip[2] / push.clip[3], "front is nearer than back");

	/*
	 * The front face's first triangle, counter-clockwise in the model, has a
	 * positive Vulkan framebuffer area, so VK_FRONT_FACE_COUNTER_CLOCKWISE
	 * treats it as front-facing.
	 */
	area = 0.0f;
	for (index = 0U; index < 3U; index++) {
		project(&first, model.vertices[model.indices[index]].position, clip);
		screen[index][0] = (clip[0] / clip[3] + 1.0f) * 320.0f;
		screen[index][1] = (clip[1] / clip[3] + 1.0f) * 240.0f;
	}

	for (index = 0U; index < 3U; index++)
		area += screen[index][0] * screen[(index + 1U) % 3U][1] - screen[(index + 1U) % 3U][0] * screen[index][1];

	check(-0.5f * area > 0.0f, "model front faces are counter-clockwise in the framebuffer");

	/* Input changes the view; R restores it bit for bit. */
	mview_input_init(&input, &camera, "host", 480U);
	mview_input_position(&input, 100.0, 100.0);
	mview_input_button(&input, 0x110U, 1);
	mview_input_motion(&input, 137.0, 81.0);
	mview_input_button(&input, 0x110U, 0);
	check(camera.yaw == 18.5f && camera.pitch == -9.5f, "left drag orbits");
	mview_input_button(&input, 0x111U, 1);
	mview_input_motion(&input, 150.0, 90.0);
	mview_input_button(&input, 0x111U, 0);
	check(camera.pan[0] > 0.0f && camera.pan[1] < 0.0f, "right drag pans with the cursor");
	mview_input_axis(&input, 0U, -45.0, -3);
	check(fabsf(camera.distance * 1.1f * 1.1f * 1.1f - initial.distance) < 1.0e-4f, "three wheel-up notches zoom in three steps");
	mview_input_axis(&input, 0U, 15.0, 0);
	mview_input_axis(&input, 0U, -15.0, 0);
	check(input.motion_events == 0U, "a coalesced drag is flushed before other events");
	mview_input_key(&input, 105U, 1);
	mview_input_key(&input, 105U, 0);
	mview_input_key(&input, 13U, 1);
	check(input.changed != 0 && input.quit == 0, "keys change the view");
	mview_input_key(&input, 19U, 1);
	check(memcmp(&camera, &initial, sizeof(camera)) == 0, "R restores the camera bit for bit");
	mview_camera_push(&camera, 640U, 480U, &push);
	check(memcmp(&push, &first, sizeof(push)) == 0, "R restores the transforms bit for bit");

	/*
	 * The per-pixel shading's scene block: projection * view * model is the push block's clip transform, the
	 * normal matrix its normal rotation; one directional and two point lights in front of the model.
	 */
	{
		struct mview_scene scene;
		struct mview_scene again;
		float view_model[16];
		float product[16];
		float largest;
		float error;
		uint32_t row;
		uint32_t column;
		uint32_t k;
		int normals;

		mview_camera_scene(&camera, 640U, 480U, &scene);
		for (column = 0U; column < 4U; column++) {
			for (row = 0U; row < 4U; row++) {
				view_model[column * 4U + row] = 0.0f;
				for (k = 0U; k < 4U; k++)
					view_model[column * 4U + row] += scene.view[k * 4U + row] * scene.model[column * 4U + k];
			}
		}
		largest = 0.0f;
		error = 0.0f;
		for (column = 0U; column < 4U; column++) {
			for (row = 0U; row < 4U; row++) {
				product[column * 4U + row] = 0.0f;
				for (k = 0U; k < 4U; k++)
					product[column * 4U + row] += scene.projection[k * 4U + row] * view_model[column * 4U + k];
				if (fabsf(first.clip[column * 4U + row]) > largest)
					largest = fabsf(first.clip[column * 4U + row]);
				if (fabsf(product[column * 4U + row] - first.clip[column * 4U + row]) > error)
					error = fabsf(product[column * 4U + row] - first.clip[column * 4U + row]);
			}
		}
		check(error <= 1.0e-5f * largest, "projection * view * model is the clip transform");
		normals = 1;
		for (column = 0U; column < 3U; column++) {
			for (row = 0U; row < 3U; row++) {
				if (scene.normal[column * 4U + row] != first.normal[column * 4U + row])
					normals = 0;
			}
		}
		check(normals && scene.normal[15] == 1.0f, "the normal matrix is the normal rotation");
		check(scene.light_position[0][3] == 0.0f && scene.light_position[1][3] == 1.0f && scene.light_position[2][3] == 1.0f,
		      "one directional and two point lights");
		check(scene.light_position[1][2] > -camera.distance && scene.light_position[2][2] > -camera.distance &&
		      scene.light_factors[1][1] > 0.0f && scene.light_factors[2][2] > 0.0f,
		      "the point lights are in front of the model and fade with distance");
		mview_camera_orbit(&camera, 30.0f, 10.0f);
		mview_camera_scene(&camera, 640U, 480U, &again);
		check(memcmp(&again, &scene, sizeof(scene)) != 0, "the scene block follows the view");
		mview_camera_reset(&camera);
		mview_camera_scene(&camera, 640U, 480U, &again);
		check(memcmp(&again, &scene, sizeof(scene)) == 0, "R restores the scene block bit for bit");
		check(sizeof(struct mview_scene) == 416U, "the scene block is the shaders' std140 layout (416 bytes)");
	}

	/* Pitch is clamped and yaw wraps. */
	mview_camera_orbit(&camera, 725.0f, 200.0f);
	check(camera.pitch == 89.0f && camera.yaw == 5.0f, "pitch clamps and yaw wraps");

	/* Zoom is clamped at both ends. */
	mview_camera_zoom(&camera, 1000);
	check(camera.distance <= camera.radius * 20.0f + 1.0e-4f, "zoom out is bounded");
	mview_camera_zoom(&camera, -1000);
	check(camera.distance >= camera.radius * 0.05f - 1.0e-6f, "zoom in is bounded");

	/* Q and Escape request the end of the run. */
	input.quit = 0;
	mview_input_key(&input, 16U, 1);
	check(input.quit != 0, "Q quits");
	input.quit = 0;
	mview_input_key(&input, 1U, 1);
	check(input.quit != 0, "Escape quits");

	/* Succeeded: the camera checks are recorded. */
	mview_model_free(&model);
	return;
}

/* Loads the converted model when present and checks its published counts. */
static void
test_qs40(
	const char *directory)
{
	struct mview_model model;
	char path[1024];
	uint32_t index;
	uint32_t draws;
	int status;

	/* The converted model may not be present yet. */
	snprintf(path, sizeof(path), "%s/model.txt", directory);
	status = access(path, R_OK);
	if (status != 0) {
		printf("qs40: not present, skipped\n");
		return;
	}

	/* The real model must load in full. */
	status = mview_model_load(&model, directory);
	if (status != 0)
		printf("qs40 error: %s\n", model.error);

	check(status == 0, "qs40 loads");
	if (status == 0) {
		draws = 0U;
		for (index = 0U; index < model.group_count; index++)
			draws += model.groups[index].count;

		printf("qs40: meshes=%u vertices=%u triangles=%u materials=%u textures=%u groups=%u\n",
		       model.mesh_count, model.vertex_count, model.triangle_count,
		       model.material_count, model.texture_count, model.group_count);
		printf("qs40: bounds %.3f %.3f %.3f .. %.3f %.3f %.3f\n",
		       (double)model.minimum[0], (double)model.minimum[1], (double)model.minimum[2],
		       (double)model.maximum[0], (double)model.maximum[1], (double)model.maximum[2]);
		check(draws == model.triangle_count * 3U, "qs40 groups cover every triangle");
	}

	/* Succeeded: the real model checks are recorded. */
	mview_model_free(&model);
	return;
}

/* Replaces a file's contents. */
static int
write_file(
	const char *path,
	const char *body,
	size_t bytes)
{
	FILE *file;
	size_t written;

	/* Truncates and writes the whole body. */
	file = fopen(path, "wb");
	if (file == NULL)
		return -1;

	written = fwrite(body, 1U, bytes, file);
	fclose(file);
	if (written != bytes)
		return -1;

	/* Succeeded: the file holds the body. */
	return 0;
}

/* Copies one small file. */
static int
copy_file(
	const char *source,
	const char *destination)
{
	static char buffer[65536];
	FILE *file;
	size_t bytes;

	/* Reads the whole source, which is a small fixture file. */
	file = fopen(source, "rb");
	if (file == NULL)
		return -1;

	bytes = fread(buffer, 1U, sizeof(buffer), file);
	fclose(file);

	/* Writes it to the destination. */
	return write_file(destination, buffer, bytes);
}

/* Applies the clip transform the vertex shader applies. */
static int
project(
	const struct mview_push *push,
	const float point[3],
	float clip[4])
{
	uint32_t row;

	/* clip = x * c0 + y * c1 + z * c2 + c3, as in the shader. */
	for (row = 0U; row < 4U; row++) {
		clip[row] = point[0] * push->clip[row] +
			    point[1] * push->clip[4U + row] +
			    point[2] * push->clip[8U + row] +
			    push->clip[12U + row];
	}

	/* Succeeded: the point is in clip space. */
	return 0;
}
