#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "timer.h"

#pragma pack(1)

// UFSCar Sorocaba
// Parallel Computing
// Final Project
// Rodrigo Barbieri, Rafael Machado and Guilherme Baldo
// MPI version

typedef struct ProgramInfo { //Structure responsible for maintaining program information and state
	unsigned int height; //image height
	unsigned int width; //image width
	int p; //number of threads selected by the user
	unsigned int header_size; //header size
	char decode; //whether the user chose to print the sorted array
	int padding;
	int repeat;
	unsigned int originalSize;
	unsigned int originalPlusPadding;
	char* encodedImageHEAD;
	char* imageInBytesHEAD;
	unsigned int* encodedSize;
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

void writeToFile(char* message, unsigned int* size,char* filename){

	FILE* output = NULL;

	while(output == NULL){
		output = fopen(filename, "ab");
	}
	
	if(NULL != output){
		fwrite(message, sizeof(char), *size, output);
		fflush(output);
		fclose(output);
	} else {
		printf("Could not write to file for some reason\n");
	}
	
}





FILE* validation(int* argc, char* argv[]){ //validates several conditions before effectively starting the program

	FILE* f = NULL;
	
	if (*argc != 5)
	{ //validates number of arguments passed to executable, currently number of threads and file name
		printf("Usage: %s <file name> <decode? Y/N> <Times to repeat> <number of threads>\n",argv[0]);
		fflush(stdout);
	} 
	else 
	{
		p_info->repeat = strtol(argv[3],NULL,10);
		if (p_info-> repeat < 1)
			exit(0);

		p_info->p = strtol(argv[4],NULL,10);
		if (p_info->p < 1)
			exit(0);

		p_info->decode = *argv[2];

		f = fopen(argv[1],"rb");

		if (f == NULL)
		{ //check if the file inputted exists
			printf("File not found!");
			fflush(stdout);
		}
	}
	
	return f;
}

void decode (char* message, unsigned int encodedWidth, unsigned int width, char* output){
	unsigned int i = 0,j = 0;
	unsigned int count = 0;
	char* color = NULL;
	int outputIndex = 0;
		
	while (i < encodedWidth){
		count = (unsigned int) message[i] & 0x000000FF;

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

void encode (char* message, unsigned int width, char* output, unsigned int* encodedSize){
	unsigned int i = 0;
	unsigned int count = 1;
	unsigned int outputIndex = 0;
	
	while (i < width){

		if (message[i] == message[i+3] && 
			message[i+1] == message[i+4] && 
			message[i+2] == message[i+5] && 
			count < 255 && (i + 3 < width)) {
			count++;
		}
		else {
			output[outputIndex] = (char) count & 0x000000FF;
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

void calculateLocalArray(unsigned int* local_n,unsigned int* my_first_i, int* rank){ //calculates local number of elements and starting index for a specific rank based on total number of elements
	unsigned int div = p_info->height / p_info->p;
	int r = p_info->height % p_info->p; //divides evenly between all threads, firstmost threads get more elements if remainder is more than zero
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

void manageProcessesReadingFile(char* bytes,char* filename, unsigned int* local_n, unsigned int size, unsigned int* my_first_i, int* rank)
{
	int i;
	FILE* input = NULL;
	int read;

	input = fopen(filename,"rb");

	if (input != NULL){

		fseek(input,p_info->header_size + (*my_first_i * size),SEEK_SET);
		for (i = 0; i < *local_n ; i++){	
			read = fread(bytes,sizeof(char), size, input);
//			printf("read: %d, rank: %d, i: %d, size: %u, my_first_i: %u\n",read,*rank,i,size,*my_first_i);
			writeToFile(bytes,&size,"teste-input.bmp");
			bytes += size;
		}
		fclose(input);
	} else
		printf("Rank: %d, Could not open file for reading\n",*rank);

}

void manageProcessesWritingToFile(char* bytes,char* filename, unsigned int* local_n,unsigned int block_size, unsigned int* size_list, int* rank){

	int i;
	FILE* output = NULL;

	output = fopen(filename, "ab");

	if (output != NULL){
		for (i = 0; i < *local_n; i++){
			fwrite(bytes, sizeof(char), size_list[i], output);
			fflush(output);
			bytes += block_size;
		}
		fclose(output);
	} else 
		printf("Rank: %d, Could not write to file\n",*rank);

}

void* threadFunction(void* rank){

	unsigned int local_n,my_first_i,i;
	int my_rank = (int) rank;
	char* encodedImage = NULL;
	char* imageInBytes = NULL;

	calculateLocalArray(&local_n,&my_first_i,&my_rank);

	encodedImage = p_info->encodedImageHEAD + (my_first_i * (p_info->originalSize * 2)) ;
	imageInBytes = p_info->imageInBytesHEAD + (my_first_i * p_info->originalPlusPadding);
//	printf("my_first_i: %d, local_n: %d, my_rank: %d\n",my_first_i,local_n,my_rank);
	for (i = my_first_i; i < local_n + my_first_i; i++){
		encode(imageInBytes,p_info->originalSize,encodedImage,&p_info->encodedSize[i]);
		imageInBytes += p_info->originalPlusPadding;
		encodedImage += p_info->originalSize * 2;
	}
	return NULL;

}

int main(int argc, char *argv[]){

	FILE *f = NULL;
	int rank;
	unsigned int local_n,my_first_i,t;
	double start = 0;
	double end = 0;
	double min = 999;
	unsigned int i;
	unsigned int* encodedSize = NULL;
	unsigned int dimensions[4];
	char* imageInBytesHEAD = NULL;
	char* encodedImageHEAD = NULL;
	char* encodedImage = NULL;
	char* imageInBytes = NULL;
	unsigned int originalSize;
	unsigned int originalPlusPadding;
	BMP_HEADER header;
	pthread_t* thread_handles;


	p_info = (ProgramInfo*) malloc(sizeof(ProgramInfo)); //allocates ProgramInfo structure

	rank = 0;
	my_first_i = 0;


	if (rank == 0){

		initialize_header(&header);
		f = validation(&argc,argv);

		if (f != NULL){
			fread(&header,sizeof(BMP_HEADER),1,f);
//			print_header(&header);
			fclose(f);
			
		}
		dimensions[0] = (unsigned int) header.width;
		dimensions[1] = (unsigned int) header.height;
		dimensions[2] = (unsigned int) header.offset_start;
		dimensions[3] = (unsigned int) p_info->repeat;
	}

	thread_handles = (pthread_t*) malloc (sizeof(pthread_t) * p_info->p);

	if (header.reserved1 != 0 || header.reserved2 != 0)
		printf("Your image is either malformed or your compiler is not reading pragma pack!\n");

	if (dimensions[0] != 0 && dimensions[1] != 0 && dimensions[2] != 0 && dimensions[3] != 0){

		p_info->width = dimensions[0];
		p_info->height = dimensions[1];
		p_info->header_size = dimensions[2];
		p_info->repeat = dimensions[3];
		p_info->padding = (p_info->width * 3) % 4 == 0 ? 0 : (4 - ((p_info->width * 3) % 4));
		originalSize = p_info->width * 3;
		originalPlusPadding = originalSize + p_info->padding;
		local_n = p_info->height;

//		printf("rank: %d, dimensions0: %d, dimensions1: %d, dimensions2: %d\n",rank,dimensions[0],dimensions[1],dimensions[2]);	

		if (rank == 0){
			remove("compressed.grg");
			remove("teste-input.bmp");
			writeToFile((char*) &header,&p_info->header_size,"compressed.grg");
			writeToFile((char*) &header,&p_info->header_size,"teste-input.bmp");
		}

		encodedSize = (unsigned int*) malloc (sizeof(unsigned int) * p_info->height);
		for (i = 0; i < p_info->height; i++)
			encodedSize[i] = 0;

		imageInBytes = (char *) malloc(originalPlusPadding * local_n);
		encodedImage = (char *) malloc(originalSize * 2 * local_n);

		memset(imageInBytes,'\0',originalPlusPadding * local_n);
		memset(encodedImage,'\0',originalSize * 2 * local_n);

		imageInBytesHEAD = imageInBytes;
		encodedImageHEAD = encodedImage;

		manageProcessesReadingFile(imageInBytes,argv[1],&local_n,originalPlusPadding, &my_first_i, &rank);


		p_info->imageInBytesHEAD = imageInBytesHEAD;
		p_info->encodedImageHEAD = encodedImageHEAD;
		p_info->originalSize = originalSize;
		p_info->originalPlusPadding = originalPlusPadding;
		p_info->encodedSize = encodedSize;


		for (t = 0 ; t < p_info->repeat; t++){
	

			GET_TIME(start);

			for (i = 0; i < p_info->p; i++)
				pthread_create(&thread_handles[i],NULL,threadFunction,(void*)i);

			for (i = 0; i < p_info->p; i++)
				pthread_join(thread_handles[i],NULL);


			GET_TIME(end);

//			printf("rank: %d, time: %lf\n",rank, end - start);

			if (end - start < min)
				min = end - start;
		}		

		printf("rank: %d, min: %lf\n",rank, min);

		encodedImage = encodedImageHEAD;
		imageInBytes = imageInBytesHEAD;
		
		manageProcessesWritingToFile(encodedImage,"compressed.grg",&local_n,originalSize * 2,encodedSize,&rank);

		encodedImage = encodedImageHEAD;
		imageInBytes = imageInBytesHEAD;
		

		
		if (encodedImage != NULL)
			free(encodedImage);
		if (imageInBytes != NULL)
			free(imageInBytes);
		if (encodedSize != NULL)
			free(encodedSize);
	}

	if (p_info != NULL)
		free(p_info);



	return 0;
}
