#include <stdio.h>
#include "decode.h"
#include "types.h"
#include <string.h>
#include "common.h"

/* Function Definitions */
    char str[50];

/* Read and validate decode args from argv */
/*This function reads the commandline inputs given by the user while decoding.
It checks:
Whether the stego image (.bmp) is valid.
Whether the output file name is provided if not it sets it to “output” by default.
If everything is valid, decoding can continue.
Otherwise, it returns e_failure to stop execution.*/
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    // Check for stego image file
    if(argv[2][0] != '.')
    {
        if(strstr(argv[2], ".bmp") != NULL)
        {
            decInfo -> dest_image_fname = argv [2];
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

    //check for output file 
    if(argv[3] == NULL)
    {
        decInfo -> output_fname = "output";
    }
    else
    {
        decInfo -> output_fname = argv[3];
    }

    return e_success;//all arguments are valid
}

/* Get File pointers for i/p and o/p files */
/*This function opens the encoded image (stego image) that
 contains your hidden secret file.
It tries to open the file in read mode "r".
If it opens successfully, decoding can continue.
If not, it prints an error and returns failure, 
stopping the process before anything breaks.*/
Status open_files_for_decoding(DecodeInfo *decInfo)
{
    decInfo -> fptr_dest_image = fopen(decInfo -> dest_image_fname, "r");

    // Do Error handling
    if (decInfo -> fptr_dest_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo -> dest_image_fname);

    	return e_failure;
    }

    return e_success;
}

/* Skip bmp image header */
/*Every BMP image starts with a 54-byte header that stores only
 information about the image (like width, height, etc.).
Your secret data starts after this header, in the actual 
pixel data area.This function simply skips those 
first 54 bytes, so decoding starts from the correct spot*/
Status skip_bmp_header(FILE *fptr_dest_image)
{
    if(fseek(fptr_dest_image, 54, SEEK_SET) != 0)
    {
        printf("Error: Failed to skip BMP header\n");
        return e_failure;
    }
    return e_success;// Successfully skipped BMP header
}

/* Decode byte from LSB*/
/*This function reads 8 bytes from the image
takes the last bit (LSB) from each one
and combines them to rebuild the original secret byte
Basically, it does what encode_byte_to_lsb() did during encoding*/
Status decode_byte_from_lsb(char *data, char *image_buffer)  
{
    char decoded_byte = 0;
    char bit_data;

    for(int i = 7; i >= 0; i--)
    {
        bit_data = image_buffer[7 - i] & 1;  // Get the LSB
        decoded_byte = decoded_byte | (bit_data << i);
    }

    *data = decoded_byte;   
    return e_success; 
}

/* Decode int from LSB*/
/*Extracts a 32-bit integer (4 bytes) of hidden data
 from 32 bytes of image data using the Least 
 Significant Bit (LSB) method.*/
Status decode_int_from_lsb(int *size, char *image_buffer)  
{
    int decoded_size = 0;
    char bit_data;

    for(int i = 31; i >= 0; i--)
    {
        bit_data = image_buffer[31 - i] & 1;    // Get the LSB
        decoded_size = decoded_size | (bit_data << i);
    }

    *size = decoded_size;  
    return e_success; 
}

/* Store Magic String */
/*This function checks if the image is encoded or not
It reads back the hidden characters magic string that the encoder stored first
and compares them with the original magic string (#*)
If everything matches decoding continues
If it doesn’t match it stops meaning the image has no valid hidden data.*/
Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)
{
    char arr[8];
    char decoded_char;

    //Run the loop strlen(magic_string) times
    for(int i = 0; i < strlen(magic_string); i++)
    {
        //Read the 8byte of data from src file
        fread(arr, 1, 8, decInfo -> fptr_dest_image);

        /* Decode a byte from LSB of image data */
        if((decode_byte_from_lsb(&decoded_char, arr)) == e_success)  
        {
            //Verify the decoded character matches magic string
            if(decoded_char == magic_string[i])
            {
                continue;
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
    return e_success;
}

/* Decode secret file extension size */
/*Decodes the size number of characters of the secret
 file extension (ex "shreedhar.txt" = 4) from the stego image.*/
Status decode_secret_file_extn_size(int *size, DecodeInfo *decInfo)  
{
    char arr[32];

    fread(arr, 1, 32, decInfo->fptr_dest_image);

    if((decode_int_from_lsb(size, arr)) == e_success)  
    {
        return e_success;
    } 
    return e_failure;//`Error in decoding file extension size 
}

/* Decode secret file extension */
/*This function reads and rebuilds the hidden file extension (like .txt or .c)
and then adds it to the output file name so that the 
recovered file gets saved with the correct format.
For example:
Hidden extension = .txt
Output file base = output
 Final output = output.txt*/
Status decode_secret_file_extn(char *file_extn, DecodeInfo *decInfo)
{
    char arr[8];
    char decoded_char;

    for(int i = 0; i < MAX_FILE_SUFFIX_DECODE; i++)
    {
        fread(arr, 1, 8, decInfo->fptr_dest_image);

        if((decode_byte_from_lsb(&decoded_char, arr)) == e_success) 
        {
            file_extn[i] = decoded_char;//store decoded character in file_extn
            
            if(decoded_char == '\0')//null character
                break;
        }
        else
        {
            return e_failure;//Error in decoding file extension
        }
    }
    
    int i=0;
    while(decInfo -> output_fname[i])//loop until null character
    {
        if(decInfo -> output_fname[i] != '.')//until we reach the null character 
        {
            str[i] = decInfo -> output_fname[i];//copy output file name to str
        }
        else
        {
            break;
        }
        i++;
    }

    str[i] = '\0';//null terminate the string 

    strcat(str, file_extn);//concatenate the file extension 
    printf("--%s\n",str);//
    decInfo -> output_fname = str;//update output file name with extension
    printf("--%s\n",decInfo -> output_fname);
    return e_success;//`Successfully decoded file extension 
}

/* Decode secret file size */
/*This function extracts the hidden file size that the encoder stored inside the image
It tells how big the secret file is for example, 200 bytes
so that the decoder knows exactly how many bytes of secret data to extract next.*/
Status decode_secret_file_size(long *file_size, DecodeInfo *decInfo)
{
    char arr[32];
    int size;

    fread(arr, 1, 32, decInfo->fptr_dest_image);

    if((decode_int_from_lsb(&size, arr)) == e_success) 
    {
        *file_size = (long)size;
        return e_success;// Successfully decoded file size 
    }
    return e_failure;//`Error in decoding file size 
}

/* Decode secret file data*/
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char arr[8];
    char decoded_char;

    // Open output file for writing
    /*This function extracts the hidden file data from the image,
    one byte at a time, and writes it to your output file*/
    decInfo->fptr_output = fopen(decInfo->output_fname, "w");
    if(decInfo->fptr_output == NULL)
    {
        return e_failure;//Error in opening output file
    }

    for(int i = 0; i < decInfo->size_output_file; i++)
    {
        fread(arr, 1, 8, decInfo->fptr_dest_image);

        if((decode_byte_from_lsb(&decoded_char, arr)) == e_success)  
        {
            fwrite(&decoded_char, 1, 1, decInfo->fptr_output);
        }
        else
        {
            fclose(decInfo->fptr_output);
            return e_failure;//Error in decoding
        }
    }
    
    fclose(decInfo->fptr_output);
    return e_success; //All bytes decoded successfully
}

/* Perform the complete decoding process */
Status do_decoding(DecodeInfo *decInfo)
{
    int extn_size;  

    /* Get File pointers for i/p files */
    if((open_files_for_decoding(decInfo)) == e_success)
    {
        printf("Data image file opened successfully...\n");

        /* Skip bmp image header */
        if((skip_bmp_header(decInfo -> fptr_dest_image)) == e_success)
        {
            //printf("BMP header skipped\n");

            /* Decode Magic String */
            if((decode_magic_string(MAGIC_STRING, decInfo)) == e_success)
            {
                printf("Magic string recieved...\n");

                /* Decode secret file extension size */
                if((decode_secret_file_extn_size(&extn_size, decInfo)) == e_success)  
                {
                    printf("size of file extension decoded: %d\n", extn_size);

                    /* Decode secret file extension */
                    if((decode_secret_file_extn(decInfo->extn_output_file, decInfo)) == e_success)
                    {
                        //printf("Secret file extension decoded: %s\n", decInfo->extn_output_file);

                        /* Decode secret file size */
                        if((decode_secret_file_size(&decInfo->size_output_file, decInfo)) == e_success)
                        {
                            printf("File size decoded: %ld\n", decInfo->size_output_file);
                                
                            /* Decode secret file data */
                            if((decode_secret_file_data(decInfo)) == e_success)
                            {
                                printf("Secret file data decoded successfully...\n");

                                return e_success;//All steps successful
                            }
                        }
                    }
                }
            }
        }
    }

    return e_failure;//If any of the steps fail
}