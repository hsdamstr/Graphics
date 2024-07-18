/* Copyright (c) 2014--2020 Scott Kuhl. All rights reserved.
 * License: This code is licensed under a 3-clause BSD license. See
 * the file named "LICENSE" for a full copy of the license.
 */

/** @file
 * @author Scott Kuhl
 */

#include "windows-compat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "imageio.h"  // also includes wand/MagickWand.h


/** Converts an ImageMagick StorageType enum into the size of that
 * storage type in bytes. This function is intended to be used inside
 * of imageio.c only.
 *
 * @param t The input StorageType to get the size of.  @return The size
 * of the StorageType in bytes.
 */
static int imageio_type_to_bytes(StorageType t)
{
	switch(t)
	{
		case CharPixel:    return sizeof(char);
		case DoublePixel:  return sizeof(double);
		case FloatPixel:   return sizeof(float);
		//case IntegerPixel: return sizeof(int); // Removed in IM 7(?)
		case LongPixel:    return sizeof(long);
		case ShortPixel:   return sizeof(short);
		default:
			fprintf(stderr, "imageio_type_to_bytes: Wrong type\n");
			return 0;
	}
}



#define ThrowWandException(wand) \
{ \
  char \
    *description; \
 \
  ExceptionType \
    severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
}
// NOTE: "exit(-1); \" could be added as the last line to make this
// function exit.


void* imagein(imageio_info *iio_info)
{
	if(IMAGEIO_DEBUG)
	{
		printf("imagein  %s: Requested %d bit/channel %s\n", iio_info->filename,
		       8*imageio_type_to_bytes(iio_info->type), iio_info->map);
		printf("imagein  %s: Reading file...\n", iio_info->filename);
	}
	
	// initialize
	MagickWandGenesis();
	MagickWand *magick_wand = NewMagickWand();

	// ask imagemagick to read image.
	MagickBooleanType status = MagickReadImage(magick_wand,iio_info->filename);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return NULL;
	}

	// If the image we loaded has no alpha channel, make sure that we
	// set the image as opaque.
	if(MagickGetImageAlphaChannel(magick_wand) == MagickFalse)
	{
#if IMAGEMAGICK_LEGACY  // set in cmakelists.txt, set when imagemagick version < 7
		MagickSetImageOpacity(magick_wand, 1);
#else
		MagickSetImageAlpha(magick_wand, 1);
#endif
	}

	/* If BottomLeftOrientation is used, we don't need to flip the
	 * image to get the first pixel to be the bottom left corner. */
	OrientationType ot = MagickGetImageOrientation(magick_wand);
	if(IMAGEIO_DEBUG)
	{
		if(ot == BottomLeftOrientation)
			printf("imagein  %s: Image orientation: bottom left\n", iio_info->filename);
		else if(ot == TopLeftOrientation)
			printf("imagein  %s: Image orientation: top left\n", iio_info->filename);
		else if(ot == UndefinedOrientation)
			printf("imagein  %s: Image orientation: undefined\n", iio_info->filename);
		else
			printf("imagein  %s: Image orientation: other\n", iio_info->filename);
	}
	
#if IMAGEMAGICK_LEGACY
	// ImageMagick 6 (and early versions of ImageMagick 7(?)), loads
	// some tga files with TopLeftOrientation but then claims that the
	// orientation is BottomLeft (causing us to fail to flip the
	// image). Newer imagemagick versions load with
	// BottomLeftOrientation AND sets BottomLeftOrientation correctly
	// (no flip is necessary in this case). This "hack/fix" might
	// break other file formats, but should fix loading tga files in
	// ImageMagick 6.
	// 
    // https://imagemagick.org/discourse-server/viewtopic.php?f=3&t=34757
	// https://github.com/ImageMagick/ImageMagick/commit/2e35b56aedcbb8ccb432bbac0d83c15ab4a3fff6
	//
	// NOTE: This was eventually fixed in imagemagick 6 in 6.9.12-16
	// (2021-06-19). It also appears fixed in Ubuntu 21.04.
	
	/*
	status = MagickSetImageOrientation(magick_wand, UndefinedOrientation);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return NULL;
	}
	ot = MagickGetImageOrientation(magick_wand);
	*/
#endif
	if(ot != BottomLeftOrientation)
	{
		// orient/flip the image so that it is TopLeft orientation.
		status = MagickAutoOrientImage(magick_wand);
		if(status == MagickFalse)
		{
			ThrowWandException(magick_wand);
			return NULL;
		}

		// Since we actually want BottomLeft, flip the image
		// vertically.
		status = MagickFlipImage(magick_wand);
		if(status == MagickFalse)
		{
			ThrowWandException(magick_wand);
			return NULL;
		}
	}

	// Get information about image.
	iio_info->width  = MagickGetImageWidth(magick_wand);
	iio_info->height = MagickGetImageHeight(magick_wand);
	char *comment = MagickGetImageProperty(magick_wand, "comment");
	if(comment)
		iio_info->comment = strdup(comment);
	else
		iio_info->comment = NULL;
	iio_info->quality = MagickGetImageCompressionQuality(magick_wand);

	// Allocate our own array to store image.
	char *image = malloc(iio_info->width*iio_info->height*imageio_type_to_bytes(iio_info->type)*strlen(iio_info->map));
	if(image == NULL)
	{
		printf("imagein: Insufficient memory to read image file %s\n", iio_info->filename);
		return NULL;
	}

	

	
	// Export pixels into our array.
	status = MagickExportImagePixels(magick_wand, 0, 0, iio_info->width, iio_info->height,
	                                 iio_info->map, iio_info->type, image);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return NULL;
	}


	// Cleanup
	magick_wand = DestroyMagickWand(magick_wand);
	MagickWandTerminus();

	return image;
}

int imageout(const imageio_info *iio_info, void* array)
{
	if(IMAGEIO_DEBUG)
	{
		printf("imageout %s: Trying to write file with %lu bit/channel.\n", iio_info->filename, iio_info->depth);
		printf("imageout %s: You provided %d bit/channel %s.\n",
		       iio_info->filename, 8*imageio_type_to_bytes(iio_info->type), iio_info->map);
		printf("imageout %s: Dimensions: %lu x %lu\n", iio_info->filename, iio_info->width, iio_info->height);
	}
	
	// initialize
	MagickWandGenesis();
	MagickWand *magick_wand = NewMagickWand();
	MagickBooleanType status = MagickConstituteImage(magick_wand, iio_info->width, iio_info->height, iio_info->map, iio_info->type, array);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return 0; // fail
	}

	status = MagickFlipImage(magick_wand);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return 0; // fail
	}
	
	if(iio_info->comment)
		MagickSetImageProperty(magick_wand, "comment", iio_info->comment);

	MagickSetImageCompressionQuality(magick_wand, iio_info->quality);
	MagickSetImageDepth(magick_wand, iio_info->depth);
	status = MagickWriteImage(magick_wand, iio_info->filename);
	if(status == MagickFalse)
	{
		ThrowWandException(magick_wand);
		return 0; // fail
	}

	magick_wand = DestroyMagickWand(magick_wand);
	MagickWandTerminus();
	return 1; // success
}



/** Converts a string into an image with the string written on it with
 * a default font. This function returns an RGBA array that represents
 * the pixels. This method is useful to add labels or text overlays in
 * OpenGL programs.
 *
 * @param label The text that you want written on a texture (all on one line).
 * @param width A pointer that will be filled in with the width of the resulting image.
 * @param height A pointer that will be filled in with the height of the resulting image.
 * @param color The color of the text rendered onto the image.
 * @param bgcolor The background color of the image (can be transparent).
 * @param pointsize The size of the font. A larger pointsize will result in larger textures.
 * @return An RGBA array in row-major order that contains the data in the image.
 */
char* image_label(const char *label, int* width, int* height, float color[3], float bgcolor[4], double pointsize)
{
	MagickBooleanType status;

	/* Since we are using a copy of ImageMagick that is not at the standard
	   location on CCSR, we need to tell the imagemagick library where to
	   look for config files. Without this, imagemagick prints errors about
	   fonts when you try to make text labels.
	*/
#ifdef __linux__
	setenv("MAGICK_CONFIGURE_PATH", "/home/kuhl/public-vrlab/ImageMagick/config", 1);
#endif

	if(IMAGEIO_DEBUG)
		printf("image_label: Making a label with text '%s'\n", label);


	MagickWandGenesis();
	MagickWand *magick_wand = NewMagickWand();
	PixelWand *bg_pixel_wand = NewPixelWand();
	PixelSetRed(bg_pixel_wand,   bgcolor[0]);
	PixelSetGreen(bg_pixel_wand, bgcolor[1]);
	PixelSetBlue(bg_pixel_wand,  bgcolor[2]);
	PixelSetAlpha(bg_pixel_wand, bgcolor[3]);
	PixelWand *fg_pixel_wand = NewPixelWand();
	PixelSetRed(fg_pixel_wand,   color[0]);
	PixelSetGreen(fg_pixel_wand, color[1]);
	PixelSetBlue(fg_pixel_wand,  color[2]);
	PixelSetAlpha(fg_pixel_wand, 1);

	// Get a drawing wand
	DrawingWand *drawwand = NewDrawingWand();
	//DrawSetStrokeColor(drawwand, fg_pixel_wand);
	DrawSetFillColor(drawwand, fg_pixel_wand);
	DrawSetFontSize(drawwand, pointsize);
    //DrawSetFont(drawwand, "Arial");
	DrawSetGravity(drawwand, SouthGravity);
	
	// Figure out text size

	//TypeMetric metrics;
	// DrawGetTypeMetrics(drawwand, label, MagickFalse, &metrics);
	// This method is NEW?  Not in 6.9.11 which is installed by default on Ubuntu 21.04.

	// We need some kind of image to figure out metrics.
	status = MagickNewImage(magick_wand, 16, 16, bg_pixel_wand);
	if(status == MagickFalse)
		ThrowWandException(magick_wand);

	double *metrics = MagickQueryFontMetrics(magick_wand, drawwand, label);
	if(metrics)
	{
		*width  = metrics[4];
		*height = metrics[5];
		// On ImageMagick 7, this seems to work as expected. On
		// ImageMagick 6, this puts the baseline of the text lined up
		// with the bottom of image---clipping any descenders and
		// draws too much whitespace above the text.
	}
	else
	{
		ThrowWandException(magick_wand);
		printf("image_label: Error getting metrics for '%s'\n", label);
		*width=1024; // just pick some widths...probably won't work well.
		*height=256;
	}
	
	// Make a new image with metrics as dimensions
	status = MagickNewImage(magick_wand, *width, *height, bg_pixel_wand);
	MagickSetImageDepth(magick_wand, 8);
	if(status == MagickFalse)
		ThrowWandException(magick_wand);

	// This code is necessary to get the background to be transparent.
#if IMAGEMAGICK_LEGACY  // set in cmakelists.txt, set when imagemagick version < 7
	status = MagickSetImageOpacity(magick_wand, bgcolor[3]); 
#else
	status = MagickSetImageAlpha(magick_wand, bgcolor[3]);
#endif
	if(status == MagickFalse)
		ThrowWandException(magick_wand);

	// Draw the text
	DrawAnnotation(drawwand, 0, 0, (unsigned char *) label);
	MagickDrawImage(magick_wand, drawwand);

	status = MagickFlipImage(magick_wand);
	if(status == MagickFalse)
		ThrowWandException(magick_wand);
	
	// always use RGBA, 8 bits per color channel
	char *image = malloc(*width * *height * 4);
	if(image == NULL)
	{
		printf("image_label: Insufficient memory to create label containing '%s'\n", label);
		return NULL;
	}

	status = MagickExportImagePixels(magick_wand, 0, 0, *width, *height,
	                                 "RGBA", CharPixel, image);

	if(status == MagickFalse)
		ThrowWandException(magick_wand);

	magick_wand = DestroyMagickWand(magick_wand);
	MagickWandTerminus();
	return image;
}
