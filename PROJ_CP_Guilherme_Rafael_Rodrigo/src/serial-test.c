#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#pragma pack(1)

// UFSCar Sorocaba
// Parallel Computing
// Final Project
// Rodrigo Barbieri, Rafael Machado and Guilherme Baldo


typedef struct ProgramInfo { //Structure responsible for maintaining program information and state
	unsigned int height; //image height
	unsigned int width; //image width
	int p; //number of threads selected by the user
	unsigned int header_size; //header size
	char decode; //whether the user chose to print the sorted array
	int padding;
} ProgramInfo;

ProgramInfo *p_info;

typedef struct BMP_HEADER {
	short signature;
	long size;
	short reserved1;
	short reserved2;
	long offset_start;
	long header_size;
	long width;
	long height;
	short planes;
	short bits;
	long compression;
	long size_data;
	long hppm;
	long vppm;
	long colors;
	long important_colors;
} BMP_HEADER;

void initialize_header(BMP_HEADER *header){
	header->signature = 0;
	header->size = 0;
	header->reserved1 = 0;
	header->reserved2 = 0;
	header->offset_start = 0;
	header->header_size = 0;
	header->width = 0;
	header->height = 0;
	header->planes = 0;
	header->bits = 0;
	header->compression = 0;
	header->size_data = 0;
	header->hppm = 0;
	header->vppm = 0;
	header->colors = 0;
	header->important_colors = 0;

}

void print_header(BMP_HEADER *header){
	printf("signature: %hd,\nsize: %ld,\nreserved1: %hd,\nreserved2: %hd,\noffset_start: %ld,\nheader_size: %ld,\nwidth: %ld,\nheight: %ld,\nplanes: %hd,\nbits: %hd,\ncompression: %ld,\nsize_data: %ld,\nhppm: %ld,\nvppm: %ld,\ncolors: %ld,\nimportant_colors: %ld\n",header->signature,header->size,header->reserved1,header->reserved2,header->offset_start,header->header_size,header->width,header->height,header->planes,header->bits,header->compression,header->size_data,header->hppm,header->vppm,header->colors,header->important_colors);
}

void myMemCpy(int* dest,int* src,int size){ //copy an amount of data from one array to another
	int i;
	for (i = 0; i < size; i++)
		dest[i] = src[i];
}

void writeToFile(char* message, unsigned int* size,char* filename){

	FILE* output = NULL;

	output = fopen(filename, "ab+");
	
	if(NULL != output){
		fseek(output, 0, SEEK_END);
		fwrite(message, sizeof(char), *size, output);
		fflush(output);
		fclose(output);
	} else {
		printf("Could not write to file for some reason\n");
	}
	
}

void manageProcessesWritingToFile(char* encoded,unsigned int* size,char* filename){

	writeToFile(encoded, size,filename);
}

FILE* validation(int* argc, char* argv[]){ //validates several conditions before effectively starting the program

	char filename[50];
	FILE* f = NULL;
	
	if (*argc != 3)
	{ //validates number of arguments passed to executable, currently number of threads and file name
		printf("Usage: %s <file name> <decode? Y/N>\n",argv[0]);
		fflush(stdout);
	} 
	else 
	{
		p_info->decode = *argv[2];

		strcpy(filename,argv[1]);
		f = fopen(filename,"r");

		if (f == NULL)
		{ //check if the file inputted exists
			printf("File not found!");
			fflush(stdout);
		}
	}
	
	return f;
}


void decode (int *message, unsigned encodedWidth, unsigned int width, int *output){
	int i = 0,j = 0;
	unsigned int count = 0;
	int* color = NULL;
	int outputIndex = 0;
		
	while (i < encodedWidth){
		count = (unsigned int) message[i] & 0xFF;

		if(count > 255){
			count = count >> 24;
		}
		
		color = &message[i+1];	

		while (j < count){
			output[outputIndex] = color[0];
			output[++outputIndex] = color[1];
			output[++outputIndex] = color[2];	
			j++;
			outputIndex++;
		}
		j = 0;
		i += 4;		
	}
}

void encode (int *message, unsigned int width, int *output, unsigned int* encodedSize){
	unsigned int i = 0;
	unsigned int count = 1;
	unsigned int outputIndex = 0;
	
	while (i < width - p_info->padding){

		if (message[i] == message[i+3] && 
			message[i+1] == message[i+4] && 
			message[i+2] == message[i+5] && 
			count < 255) {
			count++;
		}
		else {
			output[outputIndex] = (char) count & 0xFF;
			output[++outputIndex] = message[i];
			output[++outputIndex] = message[i+1];
			output[++outputIndex] = message[i+2];
			outputIndex++;
			count = 1;
		}
		i += 3;
	}
	
	*encodedSize = outputIndex;
	
}

void calculateLocalArray(unsigned int* local_n,unsigned int* my_first_i,int* rank){ //calculates local number of elements and starting index for a specific rank based on total number of elements
	unsigned int div = p_info->height / p_info->p;
	unsigned int r = p_info->height % p_info->p; //divides evenly between all threads, firstmost threads get more elements if remainder is more than zero
	if (*rank < r){
		*local_n = div + 1;
		if (my_first_i != NULL) //allows my_first_i parameter to be NULL instead of an address
			*my_first_i = *rank * *local_n;
	} else {
		*local_n = div;
		if (my_first_i != NULL) //allows my_first_i parameter to be NULL instead of an address
			*my_first_i = *rank * *local_n + r;
	}
}

void readAndDecode(FILE *input,int* buffer, unsigned int* encodedSize, unsigned int* decodedSize, int* decoded){

	if (input != NULL){

		fread(buffer,1,*encodedSize,input);

		decode(buffer,*encodedSize,*decodedSize,decoded);

	} else {

		printf("Could not read file for some reason\n");
	
	}

}

int main(int argc, char *argv[])
{

	FILE *f = NULL;
//	FILE *output = NULL;
	int rank;
	unsigned int local_n,my_first_i;
//	double start = 0;
//	double end = 0;
//	double total = 0;
//	double max = 0;
	int i;
	unsigned int* encodedSize = NULL;
	unsigned int dimensions[3];
	int* encoded = NULL;
//	int* decoded = NULL;
	int* buffer = NULL;
	BMP_HEADER header;

	p_info = (ProgramInfo*) malloc(sizeof(ProgramInfo)); //allocates ProgramInfo structure

	//Serial configuration
	p_info->p = 1;
	rank = 0;

	initialize_header(&header);

	f = validation(&argc,argv);

	if (f != NULL){
		fread(&header,sizeof(BMP_HEADER),1,f);
		print_header(&header);

	}

	dimensions[0] = (unsigned int) header.width;
	dimensions[1] = (unsigned int) header.height;
	dimensions[2] = (unsigned int) header.offset_start;

	if (dimensions[0] != 0 && dimensions[1] != 0 && dimensions[2] != 0)
	{

		p_info->width = dimensions[0];
		p_info->height = dimensions[1];
		p_info->header_size = dimensions[2];

		encodedSize = (unsigned int*) malloc (sizeof(unsigned int) * p_info->height);
		for (i = 0; i < p_info->height; i++){
			encodedSize[i] = 0;
		}

	

		calculateLocalArray(&local_n,&my_first_i,&rank);


//		f = fopen(argv[1],"r");
//		if (f != NULL){
		
			fseek(f,p_info->header_size + my_first_i,SEEK_SET);

			writeToFile((char*) &header,&p_info->header_size,"compressed.grg");

			p_info->padding = (4 - ((p_info->width * 3) % 4));

			buffer = (int *) malloc(p_info->width);
			encoded = (int *) malloc(p_info->width * 2);

			for (i = 0; i < local_n; i++)
			{	
				memset(buffer,0,p_info->width);
				memset(encoded,0,p_info->width * 2);

				fread(buffer,sizeof(int),p_info->width,f);		
		
				encode(buffer,p_info->width,encoded,&encodedSize[i]);

				if (encoded != NULL)
					manageProcessesWritingToFile((char*) encoded,&encodedSize[i],"compressed.grg");
				else 
					printf("Could not encode for some reason\n");

				memset(encoded,0,p_info->width * 2);

			}
			fclose(f);
			if (buffer != NULL)
				free(buffer);
			if (encoded != NULL)
				free(encoded);
		
			buffer = NULL;
			encoded = NULL;
/*

			if (p_info->decode == 'Y'){

				writeToFile((char*) &header,&p_info->header_size,"uncompressed.bmp");

				f = fopen("compressed.grg","r");
				fseek(f,54 + my_first_i,SEEK_SET);

				decoded = (char*) malloc(originalSize);
			
				for (i = 0; i < local_n; i++)
				{	
					buffer = (char*) realloc(buffer, encodedSize[i]);

					memset(buffer,'0',encodedSize[i]);
					memset(decoded,'0',originalSize);

					printf("%ld\n", i);
					readAndDecode(f,buffer,&encodedSize[i],&originalSize,decoded);
					printf("Step1\n");
					if (decoded != NULL)
					{
						printf("Step2\n");
						manageProcessesWritingToFile(decoded,&originalSize,"uncompressed.bmp");
						printf("Step3\n");
					} 
					else 
					{
						printf("Step4\n");
						printf("Could not encode for some reason\n");
					}
				}

				fclose(f);

				free(buffer);
				free(decoded);

		
			
*/
//		} else {
//			printf("Could not open original image\n");
//		}




	}
	
	return 0;
}
