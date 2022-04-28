/*
 * Copyright (c) 2012 Arvin Schnell <arvin.schnell@gmail.com>
 * Copyright (c) 2012 Rob Clark <rob@ti.com>
 * Copyright (c) 2020 Antonin Stefanutti <antonin.stefanutti@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "drm-common.h"

#include "decode.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "libcamera_wrap.h" 

libcameraAppHandle libcamera=NULL;

static const struct egl *egl;
static const struct gbm *gbm;
static const struct drm *drm;

static const char *shortopts = "Ac:D:f:hm:p:v:xb:B:g:G";

static const struct option longopts[] = {
		{"atomic",      no_argument,       0, 'A'},
		{"count",       required_argument, 0, 'c'},
		{"device",      required_argument, 0, 'D'},
		{"format",      required_argument, 0, 'f'},
		{"help",        no_argument,       0, 'h'},
		{"modifier",    required_argument, 0, 'm'},
		{"perfcntr",    required_argument, 0, 'p'},
		{"vmode",       required_argument, 0, 'v'},
		{"surfaceless", no_argument,       0, 'x'},
		{"background-image", required_argument,0, 'b'},
		{"background-video", required_argument,0, 'B'},
		{"chromakey-image",required_argument,0,'g'},
		{"chromakey-camera",no_argument,0,'G'},
		{0,             0,                 0, 0}
};

static void usage(const char *name) {
	printf("Usage: %s [-AcDfmpvxbBgG] <shader_file>\n"
		   "\n"
		   "options:\n"
		   "    -A, --atomic             use atomic modesetting and fencing\n"
		   "    -c, --count              run for the specified number of frames\n"
		   "    -D, --device=DEVICE      use the given device\n"
		   "    -f, --format=FOURCC      framebuffer format\n"
		   "    -h, --help               print usage\n"
		   "    -m, --modifier=MODIFIER  hardcode the selected modifier\n"
		   "    -p, --perfcntr=LIST      sample specified performance counters using\n"
		   "                             the AMD_performance_monitor extension (comma\n"
		   "                             separated list)\n"
		   "    -v, --vmode=VMODE        specify the video mode in the format\n"
		   "                             <mode>[-<vrefresh>]\n"
		   "    -x, --surfaceless        use surfaceless mode, instead of GBM surface\n"
		   "    -b, --background-image=<file>   specify a background image file name  \n"
		   "    -B, --background-video=<file>   specify a video file name  \n"
		   "    -g, --chromakey-image=<file>    specify a green screen image file name \n"
			"    -G, --chromakey-camera  use libcamera as green screen video source \n",
		   name);
}

int main(int argc, char *argv[]) {
	const char *device = NULL;
	const char *shadertoy = NULL;
	const char *perfcntr = NULL;
	const char *background = NULL;
	const char *video = NULL;
	const char *chroma = NULL;
	bool bcamera = false;
	char mode_str[DRM_DISPLAY_MODE_LEN] = "";
	char *p;
	uint32_t format = DRM_FORMAT_XRGB8888;
	uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
	int atomic = 0;
	int opt;
	unsigned int len;
	unsigned int vrefresh = 0;
	unsigned int count = ~0;
	bool surfaceless = false;
	int ret;



	while ((opt = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (opt) {
			case 'A':
				atomic = 1;
				break;
			case 'c':
				count = strtoul(optarg, NULL, 0);
				break;
			case 'D':
				device = optarg;
				break;
			case 'f': {
				char fourcc[4] = "    ";
				int length = strlen(optarg);
				if (length > 0)
					fourcc[0] = optarg[0];
				if (length > 1)
					fourcc[1] = optarg[1];
				if (length > 2)
					fourcc[2] = optarg[2];
				if (length > 3)
					fourcc[3] = optarg[3];
				format = fourcc_code(fourcc[0], fourcc[1],
									 fourcc[2], fourcc[3]);
				break;
			}
			case 'h':
				usage(argv[0]);
				return 0;
			case 'm':
				modifier = strtoull(optarg, NULL, 0);
				break;
			case 'p':
				perfcntr = optarg;
				break;
			case 'v':
				p = strchr(optarg, '-');
				if (p == NULL) {
					len = strlen(optarg);
				} else {
					vrefresh = strtoul(p + 1, NULL, 0);
					len = p - optarg;
				}
				if (len > sizeof(mode_str) - 1)
					len = sizeof(mode_str) - 1;
				strncpy(mode_str, optarg, len);
				mode_str[len] = '\0';
				break;
			case 'x':
				surfaceless = true;
				break;
			case 'b':
				background = optarg;
				break;
			case 'B':
				video = optarg;
				break;
			case 'g':
				chroma = optarg;
				break;
			case 'G':
				bcamera = true;
				break;
			default:
				usage(argv[0]);
				return -1;
		}
	}

	if (argc - optind != 1) {
		usage(argv[0]);
		return -1;
	}
	shadertoy = argv[optind];

	if (atomic) {
		drm = init_drm_atomic(device, mode_str, vrefresh, count);
	} else {
		drm = init_drm_legacy(device, mode_str, vrefresh, count);
	}
	if (!drm) {
		printf("failed to initialize %s DRM\n", atomic ? "atomic" : "legacy");
		return -1;
	}

	gbm = init_gbm(drm->fd, drm->mode->hdisplay, drm->mode->vdisplay,
				format, modifier, surfaceless);
	if (!gbm) {
		printf("failed to initialize GBM\n");
		return -1;
	}

	egl = init_egl(gbm);
	if (!egl) {
		printf("failed to initialize EGL\n");
		return -1;
	}

	ret = init_shadertoy(gbm, egl, shadertoy);
	if (ret < 0) {
		return -1;
	}

	if (perfcntr) {
		init_perfcntrs(egl, perfcntr);
	}


	if (chroma)
	{
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(chroma, &width, &height, &nrChannels, 0);
		if (data)
		{
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			// définit les options de la texture actuellement liée
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);   
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    		glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);
		}
		else
		{
			printf("%s :Failed to load chroma key image\n",argv[0]);
			return -1;
		}
	}

	if (background)
	{
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(background, &width, &height, &nrChannels, 0);
		if (data)
		{
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			// définit les options de la texture actuellement liée
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);   
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    		glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);
		}
		else
		{
			printf("%s :Failed to load background image\n",argv[0]);
			return -1;
		}
	}

	if (video && !background)
	{
		if (!open_video_to_decode(video))
		{
			read_file_to_decode();
				unsigned int texture;
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);
				// définit les options de la texture actuellement liée
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);   
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	    		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, oneFrame->width, oneFrame->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, oneFrame->data[0]);
//    			glGenerateMipmap(GL_TEXTURE_2D);
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);
				// définit les options de la texture actuellement liée
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);   
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	    		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, oneFrame->width/2, oneFrame->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, oneFrame->data[1]);
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);
				// définit les options de la texture actuellement liée
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);   
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	    		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, oneFrame->width/2, oneFrame->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, oneFrame->data[2]);				
		

		}
		else
		{
			printf("%s :Failed to load video\n",argv[0]);
			return -1;
		}
	}
	
	if (bcamera)
		libcamera = createLibcameraApp();

	glClearColor(0., 0., 0., 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	return drm->run(gbm, egl);
}
