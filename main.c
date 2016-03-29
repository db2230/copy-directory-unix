#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "process_child.h"

extern struct Container Work;

int NumOfProcess = 1;
char* FromCopy = NULL;
char* ToCopy = NULL;
pid_t* ArrChild = NULL;

//cpdir-db -m 5 "/from" "/to"
//argv: 0   1 2  3       4
void processingArg(int argc, char *argv[])
{
	if(argc != 5)
		exit(1);
	NumOfProcess = atoi(argv[2]);
	FromCopy = argv[3];
	ToCopy = argv[4];
}

/*
 * открываем директорию
 * перечисляем все файлы
 * для всех файлов проверяем их тип
 * если тип = файлу, увеличиваем счетчик
 * если тип = директории, вызываем функцию рекурсивно
 */
void getFilesCountsToCopy(char* path, int* result)
{
	struct dirent* ent;
	DIR* dir = opendir(path);
	if(dir == NULL)
		exit(2);

	ent = readdir(dir); // перечисляем все файлы в директории
	while(ent !=0)
	{
		if(!strncmp(ent->d_name, ".", 1) || !strncmp(ent->d_name, "..", 2))
		{
			ent = readdir(dir);
			continue;
		}
		if(ent->d_type == DT_DIR) // рекурсивный вызов этой же функции
		{
			// a/b
			char* newPath = calloc(256, sizeof(char));
			snprintf(newPath, 256, "%s/%s", path, ent->d_name);
			newPath[255] = 0;
			getFilesCountsToCopy(newPath, result);
			free(newPath);
		}
		else if(ent->d_type == DT_REG) // подсчитываем файлы
			++(*result);

		ent = readdir(dir);
	}

	closedir(dir);
}

// рекурсивная функция
void fillWorkMemory(char* baseFrom, char* baseTo, int* num)
{
	struct dirent* ent;
	DIR* dir = opendir(baseFrom);
	if(dir == NULL)
		exit(2);

	ent = readdir(dir);
	while(ent !=0)
	{
		if(!strncmp(ent->d_name, ".", 1) || !strncmp(ent->d_name, "..", 2))
		{
			ent = readdir(dir);
			continue;
		}
		if(ent->d_type == DT_DIR) // рекурсивно вызываем для подпапки
		{
			// a/b
			char* newFrom = calloc(256, sizeof(char));
			char* newTo = calloc(256, sizeof(char));

			snprintf(newFrom, 256, "%s/%s", baseFrom, ent->d_name);
			newFrom[255] = 0;
			snprintf(newTo, 256, "%s/%s", baseTo, ent->d_name);
			newTo[255] = 0;
			fillWorkMemory(newFrom, newTo, num);

			free(newFrom);
			free(newTo);
		}
		else if(ent->d_type == DT_REG) // это файл
		{
			//Work.Arr[num].From/.To
			snprintf(Work.Arr[*num].From, 256, "%s/%s", baseFrom, ent->d_name);
			Work.Arr[*num].From[255] = 0;
			snprintf(Work.Arr[*num].To, 256, "%s/%s", baseTo, ent->d_name);
			Work.Arr[*num].To[255] = 0;
			++(*num);
		}
		ent = readdir(dir);
	}
	closedir(dir);
}

void funcProcessSig(int sgn)
{
	for(int i = 0; i < NumOfProcess; ++i)
		kill(ArrChild[i], SIGUSR1); // посылаем сигнал подпроцессам
}

int main(int argc, char *argv[])
{
	int num = 0;
	processingArg(argc, argv); //обработка передаваемых программе аргументов
	getFilesCountsToCopy(FromCopy, &Work.count); //подсчитать кол-во файлов для копирования
	Work.Arr = calloc(Work.count, sizeof(struct FromTo)); //выделяем память
	fillWorkMemory(FromCopy, ToCopy, &num); // строим список файлов

	if(num != Work.count)
		exit(3);

	ArrChild = calloc(NumOfProcess, sizeof(pid_t)); // выделяем память для хранения потомков

	int shmID = shmget(ftok(argv[0], 'a'), sizeof(int), IPC_CREAT | 0666);
	int *shmPtr = shmat(shmID, NULL, 0);
	*shmPtr = 0;

	for(int i = 0; i < NumOfProcess; ++i)
	{
		ArrChild[i] = fork();
		if(ArrChild[i] == 0)
		{
			processChild(shmPtr);
			exit(0);
		}
	}

	struct sigaction newSig;
	newSig.sa_handler = funcProcessSig;
	sigemptyset(&newSig.sa_mask);
	newSig.sa_flags = 0;
	sigaction(SIGUSR1, &newSig, NULL);

	for(int i = 0; i < NumOfProcess; ++i)
	{
		wait(0);
	}

	shmdt(shmPtr);
	shmctl(shmID, IPC_RMID, NULL);
	free(Work.Arr);
	free(ArrChild);
	return 0;
}


