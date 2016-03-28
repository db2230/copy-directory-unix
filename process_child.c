#include "process_child.h"


struct Container Work;

int count = 0;

void copyFile(char* From, char* To)
{

	FILE* file = fopen(From, "rb");
	if(file == NULL)
		exit(5);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	char* buff = calloc(size, sizeof(char));
	fread(buff, size, sizeof(char), file);
	fclose(file);

	file = fopen(To, "wb");
	if(file == NULL)
		exit(6);
	fwrite(buff, size, sizeof(char), file);
	fflush(file);
	fclose(file);

	free(buff);

}


void createAllPath(char* path)
{
	char buff[256];
	DIR* dir = NULL;
	char* subchr = strchr(path+1, '/');
	while(subchr)
	{
		memset(buff, 0, sizeof(buff));
		memcpy(buff, path, subchr - path);
		dir = opendir(buff);
		if(dir)
			closedir(dir);
		else
			mkdir(buff, 0777);

		subchr = strchr(subchr+1, '/');
	}

}

void funcPrintf(int sgn)
{
	printf("Process #%d. SIGNAL: %d. Files: %d\n", getpid(), sgn, count);
}

void processChild(int* current)
{
	struct sigaction newSig;
	newSig.sa_handler = funcPrintf;
	sigemptyset(&newSig.sa_mask);
	newSig.sa_flags = 0;
	sigaction(SIGUSR1, &newSig, NULL);

	while(*current < Work.count)
	{
		int num = *current;
		++(*current);
		createAllPath(Work.Arr[num].To);
		copyFile(Work.Arr[num].From,Work.Arr[num].To);
		++count;
	}
	printf("Process #%d. Files: %d\n", getpid(), count);
	shmdt(current);
}

