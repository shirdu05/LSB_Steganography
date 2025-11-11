#include <stdio.h>
#include "encode.h"
#include "decode.h"
#include "types.h"
#include <string.h>

/* Check operation type */
/*This function checks the command-line argument 
  to identify which operation the user wants 
  to perform encoding or decoding.argv[]=array of cmd lines*/
OperationType check_operation_type(char *argv[])
{
    if(strcmp(argv[1], "-e") == 0)
    {
        return e_encode; //encoding
    }
    else if(strcmp(argv[1], "-d") == 0)
    {
        return e_decode;//decoding
    }
    else
    {
        return e_unsupported;//anyother than -e or -d
    }
}
/*If argv[1] is "-e", it means user selected encoding
  If argv[1] is "-d", it means user selected decoding
  Otherwise,it returns unsupported operation type*/

int main(int argc, char *argv[])
{
    EncodeInfo encInfo;  //structure variable
    
    int ret = check_operation_type(argv); 

    if(ret == 0)
    {
        if(argc >= 4)
        {
            /* Read and validate Encode args from argv */
            Status ret1 = read_and_validate_encode_args(argv, &encInfo);

           if(ret1 == e_failure)
           {
                printf("Error: Invalid arguments for encoding\n");
                return 0;
           }
           else
           {
                 /* Perform the encoding */
                if(do_encoding(&encInfo) == e_success)
                {
                    printf("File Encoding completed successfully\n");
                }
                else
                {
                    printf("Encoding failed!\n");
                }
           }
        }
        else
        {
            printf("Error: Insufficient arguments for encoding\n");
        }
    }
    else if(ret == 1)
    {
        DecodeInfo decInfo; 

        if(argc >= 3)
        {
            /* Read and validate Decode args from argv */
            Status ret2 = read_and_validate_decode_args(argv, &decInfo);

            if(ret2 == e_failure)
            {
                printf("Error: Invalid arguments for decoding\n");
                return 0;
            }
            else
            {
                if(do_decoding(&decInfo) == e_success)
                {
                    printf("Data Decoding completed successfully\n");
                    return 0;
                }
                else
                {
                    printf("Decoding failed!\n");
                    return 1;
                }
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        //Error messages
        printf("Error: Unsupported operation\n");
        printf("Use -e for encoding or -d for decoding\n");
        return 0;
    }

    return 0;
}
/*his is the main function of the program.
  It decides whether to perform encoding or decoding
   based on user input from the command line*/