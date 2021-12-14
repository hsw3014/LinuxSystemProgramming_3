#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/stat.h>
#include"backup.h"

#define BUFFER_SIZE 1024
#define MAX 255

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct NODE{
	struct NODE* next;
	char filename[MAX];
	char input_name[BUFFER_SIZE];
	char path_file[BUFFER_SIZE];
	char myfile[BUFFER_SIZE];
	pthread_t tid;

	time_t mtime;
	int period;
	int add_mapping_list[ADD_LENGTH];
	char options[MAX];

	int NUMBER;
	int TIME;
};

int fd;
struct NODE* Header;
struct NODE* cur;
struct NODE* before;
int list_length = 0;
char backup_path[BUFFER_SIZE] = {0};
char start_path[BUFFER_SIZE] = {0};

int main(int argc, char* argv[])
{
	if((fd = open("backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644)) < 0){
		fprintf(stderr, "open error for backup_log.txt\n");
		exit(1);
	}

	Header = malloc(sizeof(struct NODE));
	cur = malloc(sizeof(struct NODE));
	before = malloc(sizeof(struct NODE));

	Header->next = NULL;
	cur->next = Header->next;	//커서. 위치이동용 리스트
	before->next = Header;	//이전노드 기억용

	getcwd(start_path, BUFFER_SIZE);

	if(argc == 2)	//인자로서 백업디렉토리를 생성할 경로를 입력받았으면 시행
		check_directory(argv[1]);
	else if(argc == 1){
		//현재 디렉토리에 생성
		if(access("backup", F_OK) < 0){	//해당 위치에 backup 디렉토리가 없으면 생성. 있으면 오류
			system("mkdir backup");
			chdir("backup");	//백업할 디렉토리로 이동
		}
		else{
			chdir("backup");
		}

		getcwd(backup_path, BUFFER_SIZE);	//백업파일 경로를 기억함
	
		if(chdir(start_path) == -1){
			fprintf(stderr, "chdir error : %s\n", start_path);
			exit(1);
		}
	}
	else{	//인자 개수가 안맞으면 에러
		fprintf(stderr, "Need 1 or 2 Parameter. : ./ssu_backup [DIRECTORY]\n");
		exit(1);
	}

	//프롬프트 진입
	system("clear");
	while(1){
		int count = 0;
		char buf[BUFFER_SIZE];	//명령어 받은 한줄
		char temp[BUFFER_SIZE] = {0};	//공백을 기점으로 나눔
		char* ptr;

		char e_argv[100][100] = {0};
		char c;

		printf("20132478>");
		fgets(buf, BUFFER_SIZE, stdin);
		ptr = buf;
		if(strlen(buf) == 1){	//아무것도 입력안했을경우
			continue;
		}
		
		//매개변수 관리
		//한줄 입력받고 별개로 담아냄
		for(; *ptr != '\n'; ptr+=1){
			c = *ptr;
			if(c == ' '){
				strcpy(e_argv[count], temp);
				count++;
				memset(temp, 0, sizeof(temp));
				continue;
			}
			strncat(temp, &c, 1);
		}
		strcpy(e_argv[count], temp);	//마지막 부분도 처리함
		count++;
		memset(temp, 0, sizeof(temp));

		strcpy(e_argv[count], "");	//가장 끝은 빔

		backup_start(e_argv, count);
	}
}

void check_directory(char* dir)
{
	struct stat statbuf;
	DIR* dirp;

	//해당 디렉토리(혹은 파일) 없으면 에러출력
	if(access(dir, F_OK) < 0){
		fprintf(stderr, "There is no File :: Usage : ssu_backup <directory>\n");
		exit(1);
	}

	if(lstat(dir, &statbuf) < 0){
		fprintf(stderr, "lstat Error :: Usage : ssu_backup <directory>\n");
		exit(1);
	}

	//디렉토리가 아님
	if(!S_ISDIR(statbuf.st_mode)){
		fprintf(stderr, "No Directory :: Usage : ssu_backup <directory>\n");
		exit(1);
	}

	//읽어들일 수 있는지 확인
	if(access(dir, R_OK) < 0){
		fprintf(stderr, "Can't read dir :: Usage : ssu_backup <directory>\n");
		exit(1);
	}

	//디렉토리 오픈 및 백업을 진행할 디렉토리로 이동
	if((dirp = opendir(dir)) == NULL || chdir(dir) == -1){
		fprintf(stderr, "There is No Directory : Usage : ssu_backup <directory>\n");
		exit(1);
	}

	if(access("backup", F_OK) < 0){	//해당 위치에 backup 디렉토리가 없으면 생성. 있으면 오류
		system("mkdir backup");
		chdir("backup");	//백업할 디렉토리로 이동
	}
	else{
		chdir("backup");	//백업할 디렉토리로 이동
	}

	getcwd(backup_path, BUFFER_SIZE);	//백업파일 경로를 기억함

	if(chdir(start_path) == -1){
		fprintf(stderr, "chdir error : %s\n", start_path);
		exit(1);
	}
}
