#include <stdio.h>
#include "encode.h"
#include "types.h"
#include<string.h>
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
   
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}

/* Read and validate Encode args from argv */
/*This function reads and validates all command-line 
arguments required for encoding
It checks if the input image, secret file, and 
output stego image are correctly provided and valid*/
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    //check for source file 
    if(argv[2][0] != '.')   //check if any one char is there before .bmp
    {
        if(strstr(argv[2], ".bmp"))  
        {
            encInfo -> src_image_fname = argv[2]; //store file name into source file
        }
        else
        {
            return e_failure;
        }
    }
    else
    {
        return e_failure;
    }

    //check for secrete file

    if(argv[3][0] != '.')
    {
        if(strstr(argv[3], ".txt") || strstr(argv[3], ".c") || strstr(argv[3], ".sh") || strstr(argv[3], ".h"))  
        {
            encInfo -> secret_fname = argv[3]; //store file name into source file
        }
        else
        {
            return e_failure;
        }
    }
    else
    {
        return e_failure;
    }

    //check for last argument
    if(argv[4] == NULL)
    {
        encInfo -> stego_image_fname = "default.bmp"; //cant store in argv[4] because it has NULL address so store in default file
    }
    else
    {
        if(argv[4][0] != '.')   //check if any one char is there before .bmp
        {
            if(strstr(argv[4], ".bmp"))  
            {   
                encInfo -> stego_image_fname = argv[4]; //store file name into source file
            }
            else
            {
                return e_failure;
            }
        }
        else
        {
            return e_failure;
        }
    }

    return e_success;//all arguments are valid
}

/* Copy bmp image header */
/*Copies the first 54 bytes of BMP header from the 
  source BMP image to the destination stego image.*/
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    unsigned char header[54];

    //rewind the src file
    rewind(fptr_src_image);

    // Reading header from source
    fread(header, 1, 54, fptr_src_image);
    // Write header to destination
    fwrite(header, 1, 54, fptr_dest_image);
    
    if(ftell(fptr_src_image) == ftell(fptr_dest_image)) 
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

uint get_file_size(FILE *fptr) //alculates and returns the total size of a file in bytes.
{
    fseek(fptr, 0, SEEK_END);
    uint size = ftell(fptr); 
    return size;
}

/* check capacity */
/*Checks whether the source BMP image has enough
  capacity to hide the secret file and all
 required metadata.*/
Status check_capacity(EncodeInfo *encInfo)
{
    uint size = get_image_size_for_bmp(encInfo->fptr_src_image);

    if(size > ((strlen(MAGIC_STRING) + MAX_FILE_SUFFIX + sizeof(encInfo -> extn_secret_file) + sizeof(encInfo -> size_secret_file) + get_file_size(encInfo -> fptr_secret)) * 8) + 54)
    {
        return e_success;
    }

    return e_failure;
}

/* Encode a byte into LSB of image data array */
/*Encodes (hides) one byte of secret data into 
  8 bytes of image data using Least Significant Bit (LSB) method.*/
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    char bit_data;

    for(int i = 7; i >= 0; i--)
    {
        bit_data = ((data >> i) & 1);  //Get the bit
        image_buffer[7 - i] = image_buffer[7 - i] & (~1);  //clear the bit
        image_buffer[7 - i] = image_buffer[7 - i] | bit_data; //set the bit
    }

    return e_success; //if all 8 bits are encoded successfully
}

/* Store Magic String */
/*Encodes a predefined magic string(*#) into the image
 This magic string acts as an identifier or signature
 during decoding to confirm that the image actually
 contains hidden data.*/
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    //Declare array of size 8
    char arr[8];

    //Run the loop strlen(magic_string) times
    for(int i = 0; i < strlen(magic_string); i++)
    {
        //Read the 8byte of data from src file
        fread(arr, 1, 8, encInfo -> fptr_src_image);

        /* Encode a byte into LSB of image data array */
        if((encode_byte_to_lsb(magic_string[i], arr)) == e_success)
        {
            //Write the 8 byte data to destination
            fwrite(arr, 1, 8, encInfo -> fptr_stego_image);
        }
        else
        {
            return e_failure;
        }
    }
    return e_success; 
}

/* Encode function, which does the real encoding */
/*This function hides a 32-bit integer value into 32 image 
bytes — one bit per image byte.It moves from the most
 significant bit (MSB) to the least significant bit (LSB)
and stores each bit in the LSB of an image byte, 
keeping the image visually unchanged.*/
Status encode_int_to_lsb(int size, char *image_buffer) //collecting 32 bytes of data
{
    char bit_data;

    for(int i = 31; i >= 0; i--)
    {
        bit_data = ((size >> i) & 1);  //Get the bit
        image_buffer[31 - i] = image_buffer[31 - i] & (~1);  //clear the bit
        image_buffer[31 - i] = image_buffer[31 - i] | bit_data; //set the bit
    }

    return e_success; 
}

/* Encode extenstion size */
/*Encodes the size (length) of the secret file's 
  extension (like"shreedhar.txt"->4) into the BMP image data.*/
Status encode_secret_extn_file_size(int size, EncodeInfo *encInfo)
{
    //Declare the array with size 32
    char arr[32];

    //Read 32 byte of data from src file
    fread(arr, 1, 32, encInfo -> fptr_src_image);

    if((encode_int_to_lsb(strlen(encInfo -> extn_secret_file), arr)) == e_success)
    {
        //write  the 32 byte data into dest file
        fwrite(arr, 1, 32, encInfo->fptr_stego_image);
        return e_success;
    } 

    return e_failure;
}

/* Encode secret file extenstion */
/*Encodes the secret file's extension (like "shreedhar.txt")
  into the BMP image using the Least Significant Bit (LSB) method.*/
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    //Declare array of size 8
    char arr[8];

    //Run the loop strlen(file_extn) times
    for(int i = 0; i < strlen(file_extn); i++)
    {
        //Read the 8byte of data from src file
        fread(arr, 1, 8, encInfo -> fptr_src_image);

        /* Encode a byte into LSB of image data array */
        if((encode_byte_to_lsb(file_extn[i], arr)) == e_success)
        {
            //Write the 8byted data to destination
            fwrite(arr, 1, 8, encInfo -> fptr_stego_image);
        } 
        else
        {
            return e_failure;
        }
    }
    return e_success;
}

/* Encode secret file size */
/*This function hides the total size of your secret file in bytes inside the BMP image.
Example:
If your secret file secret.txt is 150 bytes long,
this function stores 150 in binary form inside the LSBs of 32 image bytes.
Later, when decoding, this exact number tells the program:*/
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    //Declare the array with size 32
    char arr[32];

    //Read 32 byte of data from src file
    fread(arr, 1, 32, encInfo->fptr_src_image);

    if((encode_int_to_lsb(file_size, arr)) == e_success)
    {
        //write  the 32 byte data into dest file
        fwrite(arr, 1, 32, encInfo->fptr_stego_image);
        return e_success;
    }
    return e_failure;
}

/* Encode secret file data*/
/*This function hides the real data of your secret file into the BMP image.
It takes 1 character at a time from your secret file,
and hides that character into 8 bytes of the image using LSB encoding.*/
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char arr[8];
    char ch;

    //Rewind for fptr_secret
    rewind(encInfo -> fptr_secret);

    //Run the loop  encInfo -> size_secret_file times
    for(int i = 0; i < encInfo -> size_secret_file; i++)
    {
        //Read 1 byte from secret file
        fread(&ch, 1, 1, encInfo -> fptr_secret);
        
        //Read 8 bytes from source image
        fread(arr, 1, 8, encInfo -> fptr_src_image);

        /* Encode a byte into LSB of image data array */
        if((encode_byte_to_lsb(ch, arr)) == e_success)
        {
            //Write the 8 bytes to stego image
            fwrite(arr, 1, 8, encInfo -> fptr_stego_image);
        }
        else
        {
            return e_failure;
        }
    }
    return e_success;  
}

/* Copy remaining image bytes from src to stego image after encoding */
/*After encoding the secret message,
there’s still unused image data left the rest of the BMP image pixels
This function simply copies all the leftover bytes
from the source image to the stego image*/
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{ 
   char ch;

   //read and write remaining data byte by byte
   while(fread(&ch, 1, 1, fptr_src))
   {
        fwrite(&ch, 1, 1, fptr_dest);
   }

   return e_success;
}

/* Perform the complete encoding */
Status do_encoding(EncodeInfo *encInfo)
{
    /* Get File pointers for i/p and o/p files */
    if((open_files(encInfo)) == e_success)
    {
        printf("File Opened ready to encode...!\n");
        
        // Initialize file information
        encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
        strcpy(encInfo->extn_secret_file, ".txt"); // file extension
        
        printf("Size of secret file: %ld bytes\n", encInfo->size_secret_file);
       // printf("extension type: %s\n", encInfo->extn_secret_file);
        
        if((check_capacity(encInfo)) == e_success)
        {
            //printf("Checking the capacity of file done...\n");
            /* Copy bmp image header */
            if((copy_bmp_header(encInfo -> fptr_src_image, encInfo -> fptr_stego_image)) == e_success)
            {
               // printf("Copied header successfully...\n");
                /* Store Magic String */
                if((encode_magic_string(MAGIC_STRING, encInfo)) == e_success)
                {
                    printf("Magic string uploaded...\n");
                    /* Encode extenstion size */
                    if((encode_secret_extn_file_size(MAX_FILE_SUFFIX, encInfo)) == e_success)
                    {
                        //printf("Encoded secret File extention Size Successfully...\n");
                        /* Encode secret file extenstion */
                        if((encode_secret_file_extn(encInfo -> extn_secret_file, encInfo)) == e_success)
                        {
                           // printf("Encoded secret File extention Successfully...\n");
                            /* Encode secret file size */
                            if((encode_secret_file_size(encInfo -> size_secret_file, encInfo)) == e_success)
                            {
                               // printf("secret File Size encoded Successfully...\n");
                                /* Encode secret file data*/
                                if((encode_secret_file_data(encInfo)) == e_success)
                                {
                                    //printf("File data encoded Successfully...\n");
                                    if((copy_remaining_img_data(encInfo -> fptr_src_image, encInfo -> fptr_stego_image)) == e_success)
                                    {
                                        printf("Secret file data uploaded...!\n");                                       
                                        return e_success; 
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            printf("Capacity check failed\n");
        }
    }
    else
    {
        printf("Failed to open files\n");
    }
    return e_failure;
}