/*
 * This is a bitmap compressor for the Bornhack Badge 2019
 * Copyright (C) 2019  Emil Renner Berthing
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "display.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

struct bitmap {
	unsigned int width;
	unsigned int height;
	uint8_t data[];
};

struct bitmap *
read_bmp(FILE *f)
{
	uint8_t buf[32];
	struct bitmap *ret;
	unsigned int bfSize;
	unsigned int bfOffBits;
	unsigned int biSize;
	unsigned int biWidth;
	int biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	unsigned int biCompression;
	unsigned int width;
	unsigned int height;
	unsigned int padding;

	if (fread(buf, 1, 18, f) < 18U) {
		fprintf(stderr, "Error reading bitmap: %s\n",
				strerror(ferror(f)));
		return NULL;
	}

	if (buf[0] != 'B' || buf[1] != 'M') {
		fprintf(stderr, "Error reading bitmap: not a BMP file\n");
		return NULL;
	}

	bfSize = (((unsigned int)buf[2]) <<  0)
	       | (((unsigned int)buf[3]) <<  8)
	       | (((unsigned int)buf[4]) << 16)
	       | (((unsigned int)buf[5]) << 24);

	fprintf(stderr, "bfSize = %u\n", bfSize);

	bfOffBits = (((unsigned int)buf[10]) <<  0)
	          | (((unsigned int)buf[11]) <<  8)
	          | (((unsigned int)buf[12]) << 16)
	          | (((unsigned int)buf[13]) << 24);

	fprintf(stderr, "bfOffBits = %u\n", bfOffBits);

	biSize = (((unsigned int)buf[14]) <<  0)
	       | (((unsigned int)buf[15]) <<  8)
	       | (((unsigned int)buf[16]) << 16)
	       | (((unsigned int)buf[17]) << 24);
	fprintf(stderr, "biSize = %u\n", biSize);
	if (biSize < 40) {
		fprintf(stderr, "Error reading bitmap: old OS/2 header not supported\n");
		return NULL;
	}
	if (12 + biSize >= bfOffBits) {
		fprintf(stderr, "Error reading bitmap: bfOffBits points into header\n");
		return NULL;
	}

	if (fread(buf, 1, 16, f) < 16U) {
		fprintf(stderr, "Error reading bitmap: %s\n",
				strerror(ferror(f)));
		return NULL;
	}

	biWidth = (((unsigned int)buf[0]) <<  0)
	        | (((unsigned int)buf[1]) <<  8)
	        | (((unsigned int)buf[2]) << 16)
	        | (((unsigned int)buf[3]) << 24);
	biHeight = (((unsigned int)buf[4]) <<  0)
	         | (((unsigned int)buf[5]) <<  8)
	         | (((unsigned int)buf[6]) << 16)
	         | (((unsigned int)buf[7]) << 24);
	fprintf(stderr, "biWidth x biHeight = %d x %d\n", biWidth, biHeight);

	biPlanes = (((uint16_t)buf[8]) <<  0)
	         | (((uint16_t)buf[9]) <<  8);
	if (biPlanes != 1) {
		fprintf(stderr, "Error reading bitmap: biPlanes != 1\n");
		return NULL;
	}

	biBitCount = (((uint16_t)buf[10]) <<  0)
	           | (((uint16_t)buf[11]) <<  8);
	fprintf(stderr, "biBitCount = %u\n", biBitCount);
	if (biBitCount != 24) {
		fprintf(stderr, "Error reading bitmap: only 24bit bitmaps supported\n");
		return NULL;
	}

	biCompression = (((unsigned int)buf[12]) <<  0)
	              | (((unsigned int)buf[13]) <<  8)
	              | (((unsigned int)buf[14]) << 16)
	              | (((unsigned int)buf[15]) << 24);
	fprintf(stderr, "biCompression = %u\n", biCompression);
	if (biCompression != 0) {
		fprintf(stderr, "Error reading bitmap: only uncompressed bitmaps supported\n");
		return NULL;
	}

	/* read from f until we reach the pixel data */
	bfOffBits -= 18 + 16;
	while (bfOffBits > 0) {
		size_t chunk = (bfOffBits < ARRAY_SIZE(buf)) ? bfOffBits : ARRAY_SIZE(buf);

		if (fread(buf, 1, chunk, f) < chunk) {
			fprintf(stderr, "Error reading bitmap: %s\n",
					strerror(ferror(f)));
			return NULL;
		}
		bfOffBits -= chunk;
	}

	width = biWidth;
	height = (biHeight < 0) ? -biHeight : biHeight;
	if (bfOffBits + 3 * width * height > bfSize) {
		fprintf(stderr, "Error reading bitmap: file too short\n");
		return NULL;
	}

	ret = malloc(sizeof(ret->width) + sizeof(ret->height)
			+ 3 * width * height);
	if (!ret) {
		fprintf(stderr, "Error allocating bitmap: %s\n",
				strerror(errno));
		return NULL;
	}
	ret->width  = width;
	ret->height = height;
	padding = (3 * width) % 4;
	if (padding > 0)
		padding = 4 - padding;

	while (biHeight != 0) {
		uint8_t *p;
		unsigned int i;

		if (biHeight < 0) {
			p = &ret->data[3 * biWidth * ((int)height + biHeight)];
			biHeight += 1;
		} else {
			biHeight -= 1;
			p = &ret->data[3 * biWidth * biHeight];
		}

		if (fread(p, 1, 3 * width, f) < 3 * width) {
			fprintf(stderr, "Error reading bitmap: %s\n",
					strerror(ferror(f)));
			return NULL;
		}

		if (padding > 0 && fread(buf, 1, padding, f) < padding) {
			fprintf(stderr, "Error reading bitmap: %s\n",
					strerror(ferror(f)));
			return NULL;
		}

		/* flip bgr -> rgb */
		for (i = width; i; i--) {
			p[0] ^= p[2];
			p[2] ^= p[0];
			p[0] ^= p[2];
			p += 3;
		}
	}

	return ret;
}

struct bitstream {
	dp_bitstream_data_t *p;
	dp_bitstream_data_t mask;
};

static void
bitstream_init(struct bitstream *bs, dp_bitstream_data_t *p)
{
	*p = 0;
	bs->p = p;
	bs->mask = ((dp_bitstream_data_t)1) << (8*sizeof(dp_bitstream_data_t) - 1);
}

static void
bitstream_push(struct bitstream *bs, int v)
{
	if (v)
		*bs->p |= bs->mask;
	bs->mask >>= 1;
	if (bs->mask == 0)
		bitstream_init(bs, bs->p + 1);
}

static void
bitstream_puts(struct bitstream *bs, int v)
{
	int r = v <= 0;
	unsigned int q = r ? -v : v-1;

	for (; q; q--)
		bitstream_push(bs, 1);
	bitstream_push(bs, 0);
	bitstream_push(bs, r);
}

static void
bitstream_put(struct bitstream *bs, unsigned int v)
{
	unsigned int mask;

	if (v == 0) {
		bitstream_push(bs, 0);
		return;
	}

	for (mask = 0x80000000U; mask; mask >>= 1) {
		if (v & mask)
			break;
	}
	bitstream_push(bs, 1);
	for (mask >>= 1; mask; mask >>= 1) {
		bitstream_push(bs, 1);
		bitstream_push(bs, v & mask);
	}
	bitstream_push(bs, 0);
}

static struct dp_cimage *
compress(struct bitmap *bmp, size_t *len)
{
	struct dp_cimage *out;
	const uint8_t *p = bmp->data;
	const uint8_t *end = p + 3 * bmp->width * bmp->height;
	struct bitstream bs;
	int old = 0;
	int current;

	out = malloc(sizeof(out->width) + sizeof(out->height)
			+ 3 * bmp->width * bmp->height);
	if (!out)
		return NULL;
	out->width = bmp->width;
	out->height = bmp->height;
	bitstream_init(&bs, out->data);

	current  = *p++; current <<= 8;
	current |= *p++; current <<= 8;
	current |= *p++;
	current &= 0xfcfcfc;

	while (current >= 0) {
		unsigned int run = 0;
		int next = -1;
		int8_t d;

		while (p < end) {
			next  = *p++; next <<= 8;
			next |= *p++; next <<= 8;
			next |= *p++;
			next &= 0xfcfcfc;

			if (next != current)
				break;

			run++;
		}

		d = ((current >> 16) & 0xfc) - ((old >> 16) & 0xfc);
		bitstream_puts(&bs, d / 4);
		d = ((current >>  8) & 0xfc) - ((old >>  8) & 0xfc);
		bitstream_puts(&bs, d / 4);
		d = ((current >>  0) & 0xfc) - ((old >>  0) & 0xfc);
		bitstream_puts(&bs, d / 4);
		bitstream_put(&bs, run);

		old = current;
		current = next;
	}

	if (len) {
		if (bs.mask == ((dp_bitstream_data_t)1) << (8*sizeof(dp_bitstream_data_t) - 1))
			*len = sizeof(dp_bitstream_data_t)*(bs.p - out->data);
		else
			*len = sizeof(dp_bitstream_data_t)*(bs.p - out->data)
			     + sizeof(dp_bitstream_data_t);
	}

	return out;
}

struct dp_bitstream {
	const dp_bitstream_data_t *p;
	dp_bitstream_data_t mask;
};

static void
dp_bitstream_init(struct dp_bitstream *bs, const dp_bitstream_data_t *p)
{
	bs->p = p;
	bs->mask = ((dp_bitstream_data_t)1) << (8*sizeof(dp_bitstream_data_t) - 1);
}

static dp_bitstream_data_t
dp_bitstream_pop(struct dp_bitstream *bs)
{
	dp_bitstream_data_t ret = *bs->p & bs->mask;
	bs->mask >>= 1;
	if (bs->mask == 0)
		dp_bitstream_init(bs, bs->p + 1);

	return ret;
}

static unsigned int
dp_bitstream_get(struct dp_bitstream *bs)
{
	unsigned int ret;

	if (!dp_bitstream_pop(bs))
		return 0;

	ret = 1;
	while (dp_bitstream_pop(bs)) {
		ret <<= 1;
		if (dp_bitstream_pop(bs))
			ret |= 1;
	}

	return ret;
}

static int
dp_bitstream_gets(struct dp_bitstream *bs)
{
	int ret = 0;

	while (dp_bitstream_pop(bs))
		ret++;

	if (dp_bitstream_pop(bs))
		return -ret;

	return ret + 1;
}

static int
compare(struct bitmap *bmp, struct dp_cimage *cimg)
{
	const uint8_t *p = bmp->data;
	const uint8_t *end = p + 3 * bmp->width * bmp->height;
	struct dp_bitstream bs;
	unsigned int len = cimg->width * cimg->height;
	unsigned int run = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;

	dp_bitstream_init(&bs, cimg->data);

	for (; len; len--) {
		if (run == 0) {
			r = (int)r + 4*dp_bitstream_gets(&bs);
			g = (int)g + 4*dp_bitstream_gets(&bs);
			b = (int)b + 4*dp_bitstream_gets(&bs);
			run = dp_bitstream_get(&bs);
		} else
			run--;

		if (p >= end || (*p++ & 0xfcU) != r)
			return -1;
		if (p >= end || (*p++ & 0xfcU) != g)
			return -1;
		if (p >= end || (*p++ & 0xfcU) != b)
			return -1;
	}

	return 0;
}


static void
dp_cimage_dump(struct dp_cimage *img, size_t len)
{
	size_t i;

	printf(
		"#include \"display.h\"\n"
		"\n"
		"const struct dp_cimage image = {\n"
		"\t.width = %u,\n"
		"\t.height = %u,\n"
		"\t.data = {",
			img->width, img->height);

	for (i = 0; i < len/sizeof(dp_bitstream_data_t); i++) {
		if (sizeof(dp_bitstream_data_t) == 1 && (i % 16) == 0)
			printf("\n\t\t");
		else if (sizeof(dp_bitstream_data_t) == 4 && (i % 4) == 0)
			printf("\n\t\t");

		if (sizeof(dp_bitstream_data_t) == 1)
			printf("0x%02x,", img->data[i]);
		else if (sizeof(dp_bitstream_data_t) == 4)
			printf("0x%08x,", img->data[i]);
		else
			__builtin_unreachable();
	}
	printf("\n\t},\n};\n");
}

int main(void)
{
	struct bitmap *bmp = NULL;
	struct dp_cimage *cimg = NULL;
	int ret = EXIT_FAILURE;
	size_t len;

	bmp = read_bmp(stdin);
	if (!bmp)
		goto fail;

	if (bmp->width > 240 || bmp->height > 240) {
		fprintf(stderr, "Error: image larger than 240x240 display (%ux%u).\n",
				bmp->width, bmp->height);
		goto fail;
	}

	cimg = compress(bmp, &len);
	if (!cimg) {
		fprintf(stderr, "Memory allocation failure :(\n");
		goto fail;
	}

	fprintf(stderr, "Compressed image from %u bytes to %zu bytes.\n",
			(18 * bmp->width * bmp->height + 7) / 8,
			len);

	if (compare(bmp, cimg)) {
		fprintf(stderr, "Compare failed :(\n");
		goto fail;
	}

	dp_cimage_dump(cimg, len);
	ret = EXIT_SUCCESS;
fail:
	free(bmp);
	free(cimg);
	return ret;
}
