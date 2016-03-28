#pragma once
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

struct FromTo
{
	char From[256];
	char To[256];
};

struct Container
{
	struct FromTo* Arr;
	int count;
};

void processChild(int* current);

extern struct Container Work;
