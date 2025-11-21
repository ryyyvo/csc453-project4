#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "libDisk.h"

int main(int argc, char *argv[]){
	char *fileName = "hello.bin";
	openDisk(fileName, 768);
	

	return 0;
}
