#ifndef DECODE_H
#define DECODE_H
#include<stdio.h>
#include "types.h" // Contains user defined types

#define MAX_SECRET_BUF_SIZE 1
#define MAX_IMAGE_BUF_SIZE (MAX_SECRET_BUF_SIZE * 8)
#define MAX_FILE_SUFFIX_DECODE 4

typedef struct _DecodeInfo
{
    /* Destination Image info */ 
    char *dest_image_fname;
    FILE *fptr_dest_image;

    /* output File Info */       
    char *output_fname;  
    FILE *fptr_output;
    char extn_output_file[MAX_FILE_SUFFIX_DECODE]; 
    long size_output_file;

} DecodeInfo;

/* Decoding function prototype */

/* Read and validate Decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);

/* Perform the decoding */
Status do_decoding(DecodeInfo *decInfo);

/* Get File pointers for i/p and o/p files */
Status open_files_for_decoding(DecodeInfo *decInfo);

/* Skip bmp image header */
Status skip_bmp_header(FILE *fptr_dest_image);

/* Store Magic String */
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo);

/* Decode extenstion size */
Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo); 

/* Decode secret file extenstion */
Status decode_secret_file_extn(char *file_extn, DecodeInfo *decInfo);

/* Decode secret file size */
Status decode_secret_file_size(long *file_size, DecodeInfo *decInfo);

/* Decode secret file data*/
Status decode_secret_file_data(DecodeInfo *decInfo);

/* Decode int from LSB*/
Status decode_int_from_lsb(int *size, char *image_buffer); //collecting 32 bytes of data

/* Decode byte from LSB*/
Status decode_byte_from_lsb(char *data, char *image_buffer); // collecting 8 bytes of data  

#endif