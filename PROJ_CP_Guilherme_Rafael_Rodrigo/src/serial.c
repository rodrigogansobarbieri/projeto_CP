#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

// UFSCar Sorocaba
// Parallel Computing
// Final Project
// Rodrigo Barbieri, Rafael Machado and Guilherme Baldo


typedef struct ProgramInfo { //Structure responsible for maintaining program information and state
	long height; //image height
	long width; //image width
	int p; //number of threads selected by the user
	char print; //whether the user chose to print the sorted array
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

void read_header(BMP_HEADER *header,FILE *f){
	fread(&(header->signature),2,1,f);
	fread(&(header->size),4,1,f);
	fread(&(header->reserved1),2,1,f);
	fread(&(header->reserved2),2,1,f);
	fread(&(header->offset_start),4,1,f);
	fread(&(header->header_size),4,1,f);
	fread(&(header->width),4,1,f);
	fread(&(header->height),4,1,f);
	fread(&(header->planes),2,1,f);
	fread(&(header->bits),2,1,f);
	fread(&(header->compression),4,1,f);
	fread(&(header->size_data),4,1,f);
	fread(&(header->hppm),4,1,f);
	fread(&(header->vppm),4,1,f);
	fread(&(header->colors),4,1,f);
	fread(&(header->important_colors),4,1,f);

}

void myMemCpy(int* dest,int* src,int size){ //copy an amount of data from one array to another
	int i;
	for (i = 0; i < size; i++)
		dest[i] = src[i];
}

void writeToFile(char* encoded, long* size){

	FILE* compressedFile = NULL;
	char FileName[30]={'\0'};

	sprintf(FileName, "compressed.grg");

	compressedFile = fopen(FileName, "ab+");
	
	if(NULL != compressedFile){
		fseek(compressedFile, 0, SEEK_END);
		fwrite(encoded, sizeof(char), *size, compressedFile);
	}

	fclose(compressedFile);
}

void manageProcessesWritingToFile(char* encoded,long* size){

	writeToFile(encoded, size);
}



void print_local_array(int *local_n, int local_array[]){ //prints an array

	int i;
	for (i = 0; i < *local_n; i++)
		printf("%d ",local_array[i]);
	printf("\n");
}



FILE* validation(int* argc, char* argv[]){ //validates several conditions before effectively starting the program

	char filename[50];
	FILE* f = NULL;
	
	if (*argc != 2)
	{ //validates number of arguments passed to executable, currently number of threads and file name
		printf("Usage: %s <file name>\n",argv[0]);
		fflush(stdout);
	} 
	else 
	{
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




int encode (char *message, int width, int height, char *output){
	int i = 0,j = 0;
	int count = 1;
	int outputIndex = 0;
	
	int countBytesPerLine = 0;
	int bytesPerLine = width*3;
	int bytesPadding = width%4;
		
	while (j < height) {
		while (countBytesPerLine < bytesPerLine){

			if (message[i] == message[i+3] && message[i+1] == message[i+4] && message[i+2] == message[i+5] && count < 255) {
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
			countBytesPerLine += 3;
			i += 3;
		}
		countBytesPerLine = 0;
		j++;
		i += bytesPadding;
	}
	
	return outputIndex;
	
}

void calculateLocalArray(long* local_n,long* my_first_i,int* rank){ //calculates local number of elements and starting index for a specific rank based on total number of elements
	long div = p_info->height / p_info->p;
	long r = p_info->height % p_info->p; //divides evenly between all threads, firstmost threads get more elements if remainder is more than zero
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

char* readAndEncode(FILE *input,long* encodedSize){
	long bufferSize; 
	char *buffer = NULL; 
	char *encoded = NULL;

	if (input != NULL){
		bufferSize = sizeof(char) * 3 * (p_info->width);
		buffer = (char *) malloc(bufferSize);
		encoded = (char *) malloc(bufferSize * 2);
		memset(buffer,'0',bufferSize);
		memset(encoded,'0',bufferSize * 2);
		fread(buffer,1,bufferSize,input);		
		
		*encodedSize = encode(buffer,bufferSize,p_info->width,encoded);
		encoded = (char*) realloc(encoded,sizeof(char) * *encodedSize);
	} 
	else {
		printf("Could not reopen file for some reason\n");
	}
	return encoded;

}

int main(int argc, char *argv[])
{

	FILE *f = NULL;
	FILE *output = NULL;
	char filename[50];
	char print = 'Y';
	int rank,p,t;
	long local_n,my_first_i,n;
	int *array = NULL;
	int *aux;
	int *result;
	int *local_array;
	double start = 0;
	double end = 0;
	double total = 0;
	double max = 0;
	int i;
	long encodedSize = 0;
	long dimensions[2];
	char* encoded = NULL;
	BMP_HEADER header;

	p_info = (ProgramInfo*) malloc(sizeof(ProgramInfo));//allocates ProgramInfo structure

	p_info->p = 1;
	rank = 0;

	initialize_header(&header);

	
	f = validation(&argc,argv);
	if (f != NULL)
	{
		read_header(&header,f);
		print_header(&header);
		fclose(f);
		dimensions[0] = header.width;
		dimensions[1] = header.height;
	} 
	else 
	{
		dimensions[0] = 0;
		dimensions[1] = 0;
	}
	
	if (dimensions[0] != 0 && dimensions[1] != 0)
	{
		p_info->width = dimensions[0];
		p_info->height = dimensions[1];

		

		calculateLocalArray(&local_n,&my_first_i,&rank);

		f = fopen(argv[1],"r");
		fseek(f,54 + my_first_i,SEEK_SET);

		for (i = 0; i < local_n; i++)
		{	

			encoded = readAndEncode(f,&encodedSize);

			if (encoded != NULL)
			{
				manageProcessesWritingToFile(encoded,&encodedSize);
			} 
			else 
			{
				printf("Could not encode for some reason\n");
			}
		}
		fclose(f);
	}
	
	return 0;
}
