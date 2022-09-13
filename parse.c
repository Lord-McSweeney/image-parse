
/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include "../../resources/coloredtext.h"

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;
int isgreyscale = 0;
int big = 0;
int skips = 1;
int bigset = 0;
int skipsset = 0;

void read_png_file(char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}


void write_png_file(char* file_name)
{
        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
                abort_("[write_png_file] File %s could not be opened for writing", file_name);


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);

        /* cleanup heap allocation */
        for (y=0; y<height; y++)
                free(row_pointers[y]);
        free(row_pointers);

        fclose(fp);
}


void process_file(void)
{
        int hasalpha = 0;
        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA)
                hasalpha = 1;

        for (y=0; y<height; y+=skips) {
                png_byte* row = row_pointers[y];
                for (x=0; x<width; x+=skips) {
                        if (hasalpha) {
                          png_byte* ptr = &(row[x*4]);
                          int greyscale = (ptr[0] + ptr[1] + ptr[2]) / (256 - ptr[3]);
                          if (isgreyscale) {
                            //printf("Pixel had value: %d\n", greyscale);
                            if (greyscale > 573) {
                              printColoredText(COLORLIGHTWHITE, "W");
                            } else if (greyscale > 450) {
                              printColoredText(COLORWHITE, "W");
                            } else if (greyscale > 191) {
                              printColoredText(COLORLIGHTBLACK, "W");
                            } else {
                              printColoredText(COLORBLACK, "W");
                            }
                          } else {
                            if (ptr[1] - ptr[0] < 100 && ptr[1] - ptr[2] > 20 && ptr[0] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTYELLOW, "W");
                            } else if (ptr[1] - ptr[0] < 100 && ptr[1] - ptr[2] > 20 && ptr[0] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORYELLOW, "W");
                            } else if (ptr[0] - ptr[1] > 20 && ptr[0] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTRED, "W");
                            } else if (ptr[0] - ptr[1] > 20 && ptr[0] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORRED, "W");
                            } else if (ptr[1] - ptr[0] > 20 && ptr[1] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTGREEN, "W");
                            } else if (ptr[1] - ptr[0] > 20 && ptr[1] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORGREEN, "W");
                            } else if (ptr[2] - ptr[1] > 20 && ptr[2] - ptr[0] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTBLUE, "W");
                            } else if (ptr[2] - ptr[1] > 20 && ptr[2] - ptr[0] > 20 && greyscale <= 384) {
                              printColoredText(COLORBLUE, "W");
                            } else if (ptr[2] - ptr[0] < 100 && ptr[2] - ptr[1] > 20 && ptr[0] - ptr[1] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTMAGENTA, "W");
                            } else if (ptr[2] - ptr[0] < 100 && ptr[2] - ptr[1] > 20 && ptr[0] - ptr[1] > 20 && greyscale <= 384) {
                              printColoredText(COLORMAGENTA, "W");
                            } else if (ptr[2] - ptr[1] < 100 && ptr[1] - ptr[0] > 20 && ptr[2] - ptr[0] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTCYAN, "W");
                            } else if (ptr[2] - ptr[1] < 100 && ptr[1] - ptr[0] > 20 && ptr[2] - ptr[0] > 20 && greyscale <= 384) {
                              printColoredText(COLORCYAN, "W");
                            } else if (greyscale > 573) {
                              printColoredText(COLORLIGHTWHITE, "W");
                            } else if (greyscale > 450) {
                              printColoredText(COLORWHITE, "W");
                            } else if (greyscale > 191) {
                              printColoredText(COLORLIGHTBLACK, "W");
                            } else if (greyscale >= 0) {
                              printColoredText(COLORBLACK, "W");
                            } else {
                              printf("Parse error!");
                            }
                          }
                        //printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
                        //       x, y, ptr[0], ptr[1], ptr[2], ptr[3]);

                        /* set red value to 0 and green value to the blue one */
                        } else {
                          png_byte* ptr = &(row[x*3]);
                          int greyscale = (ptr[0] + ptr[1] + ptr[2]);
                          //printf("%d", greyscale);
                          if (isgreyscale) {
                            //printf("Pixel had value: %d\n", greyscale);
                            if (greyscale > 573) {
                              printColoredText(COLORLIGHTWHITE, "W");
                            } else if (greyscale > 450) {
                              printColoredText(COLORWHITE, "W");
                            } else if (greyscale > 191) {
                              printColoredText(COLORLIGHTBLACK, "W");
                            } else {
                              printColoredText(COLORBLACK, "W");
                            }
                          } else {
                            if (ptr[1] - ptr[0] < 100 && ptr[1] - ptr[2] > 20 && ptr[0] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTYELLOW, "W");
                            } else if (ptr[1] - ptr[0] < 100 && ptr[1] - ptr[2] > 20 && ptr[0] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORYELLOW, "W");
                            } else if (ptr[0] - ptr[1] > 20 && ptr[0] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTRED, "W");
                            } else if (ptr[0] - ptr[1] > 20 && ptr[0] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORRED, "W");
                            } else if (ptr[1] - ptr[0] > 20 && ptr[1] - ptr[2] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTGREEN, "W");
                            } else if (ptr[1] - ptr[0] > 20 && ptr[1] - ptr[2] > 20 && greyscale <= 384) {
                              printColoredText(COLORGREEN, "W");
                            } else if (ptr[2] - ptr[1] > 20 && ptr[2] - ptr[0] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTBLUE, "W");
                            } else if (ptr[2] - ptr[1] > 20 && ptr[2] - ptr[0] > 20 && greyscale <= 384) {
                              printColoredText(COLORBLUE, "W");
                            } else if (ptr[2] - ptr[0] < 100 && ptr[2] - ptr[1] > 20 && ptr[0] - ptr[1] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTMAGENTA, "W");
                            } else if (ptr[2] - ptr[0] < 100 && ptr[2] - ptr[1] > 20 && ptr[0] - ptr[1] > 20 && greyscale <= 384) {
                              printColoredText(COLORMAGENTA, "W");
                            } else if (ptr[2] - ptr[1] < 100 && ptr[1] - ptr[0] > 20 && ptr[2] - ptr[0] > 20 && greyscale > 384) {
                              printColoredText(COLORLIGHTCYAN, "W");
                            } else if (ptr[2] - ptr[1] < 100 && ptr[1] - ptr[0] > 20 && ptr[2] - ptr[0] > 20 && greyscale <= 384) {
                              printColoredText(COLORCYAN, "W");
                            } else if (greyscale > 573) {
                              printColoredText(COLORLIGHTWHITE, "W");
                            } else if (greyscale > 450) {
                              printColoredText(COLORWHITE, "W");
                            } else if (greyscale > 191) {
                              printColoredText(COLORLIGHTBLACK, "W");
                            } else if (greyscale >= 0) {
                              printColoredText(COLORBLACK, "W");
                            } else {
                              printf("Parse error!");
                            }
                          }
                        }
                }
                if (big) {
                  getchar();
                } else {
                  printf("\n");
                }
        }
}
/*
Options:
--test, --color-test, and -t does not parse images and instead runs a color test
--greyscale and -g show the image in greyscale
--big and -b act like the more command- pressing enter will display the image lines line-by-line.
--skips [num] and -s [num] choose the number of lines to skip. --skips 2, for example, will reduce the content displayed on the screen by 75%.

If neither option --skips nor --big is used, they will be configured automatically.
*/

int main(int argc, char **argv) {
        if (argc < 2) {
          printf("Usage: parse image.png\nOptions:\n          --test, --color-test, and -t does not parse images and instead runs a color test\n          --greyscale and -g show the image in greyscale\n          --big and -b act like the more command- pressing enter will display the image lines line-by-line.\n          --skips [num] and -s [num] choose the number of lines to skip. --skips 2, for example, will reduce the content displayed on the screen by 75%%.\n");
          return 1;
        }
        read_png_file(argv[1]);
        if (argc > 2) {
          if (!strcmp(argv[2], "--greyscale") || !strcmp(argv[2], "-g")) {
            isgreyscale = 1;
          }
          if (!strcmp(argv[2], "--big") || !strcmp(argv[2], "-b")) {
            big = 1;
            bigset = 1;
          }
          if (!strcmp(argv[2], "--test") || !strcmp(argv[2], "-t") || !strcmp(argv[2], "--color-test")) {
            colorTest();
            return 0;
          }
          if (argc == 4 || argc == 5) {
            if (!strcmp(argv[2], "--skips") || !strcmp(argv[2], "-s")) {
              printf("using skips with skips number %d\n", atoi(argv[3]));
              skips = atoi(argv[3]);
              skipsset = 1;
            } else if (!strcmp(argv[3], "--skips") || !strcmp(argv[3], "-s")) {
              printf("using skips with skips number %d\n", atoi(argv[4]));
              skips = atoi(argv[4]);
              skipsset = 1;
            }
          }
        }
        printf("Image is %dx%d.\n", width, height);
        if (!skipsset && !bigset) {
          struct winsize w;
          ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
          int termheight = w.ws_row;
          int termwidth = w.ws_col;
          getchar();
          if (termheight && termwidth) {
            if (!(termheight > height && termwidth > width)) {
              int widthtimes = width / termwidth;
              int heighttimes = height / termheight;
              skips = widthtimes + 1;
              if (heighttimes >= 1) {
                big = 1;
              }
            }
          }
        }
        if (!strcmp(argv[1], "--test") || !strcmp(argv[1], "-t") || !strcmp(argv[1], "--color-test")) {
          colorTest();
          return 0;
        }
        process_file();

        return 0;
}
