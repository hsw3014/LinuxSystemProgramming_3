#include<stdio.h>
#include<ctype.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<sys/stat.h>
#include<time.h>
#include<dirent.h>
#include"backup_start.h"

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
extern int fd;
extern pthread_mutex_t mutex;
extern struct NODE* Header;
extern struct NODE* cur;
extern struct NODE* before;
extern int list_length;
extern char backup_path[BUFFER_SIZE];
extern char start_path[BUFFER_SIZE];

time_t t;
struct tm *tm;

void *backup_thread(void *arg);
void search_dir(char* dir, int* add_mapping_list, int period, int NUMBER, int TIME, time_t mt);

void backup_start(char argv[100][100], int argc)
{
	t = time(NULL);
	tm = localtime(&t);
	int i,j;
	int where;

	cur->next = Header->next;
	before->next = Header;	//매번 초기세팅으로

	//char path[BUFFER_SIZE] = {0};
	char path_file[BUFFER_SIZE] = {0};
	//getcwd(path, BUFFER_SIZE);

	char filename[MAX] = {0};
	char input_name[BUFFER_SIZE] = {0};
	char r_path[BUFFER_SIZE] = {0};
	int period;
	char mapping_list[C_LENGTH] = {0};
	int add_mapping_list[ADD_LENGTH] = {0};
	int NUMBER;
	int TIME;

	struct stat statbuf;
	struct NODE choice;

	for(i=0; i<C_LENGTH; i++){	//명령어 스캔
		if(strcmp(argv[0], command_list[i]) == 0){
			mapping_list[i] = 1;
			break;
		}
	}

	if(mapping_list[0] == 1){		//add
		//<FILENAME> <PERIOD> <OPTION>
		if(argc >= 3){	//option은 없어도 기능해야함
			//argv[0] 은 기능
			//argv[1] 은 FILENAME
			//argv[2] 은 PERIOD
			//argv[3] 부터는 옵션
			
			strcpy(input_name, argv[1]);	//입력받은 상대경로 저장
			char* ptr = strtok(argv[1], "/");	//파일명만 저장
			char* save;
			while(ptr != NULL){	//상대경로 끝단의 파일만 사용
				save = ptr;
				ptr = strtok(NULL, "/");
			}
			
			strcpy(filename, save);
			//절대경로 - 백업생성될 폴더의 절대경로임
			sprintf(path_file,"%s/%s", backup_path, filename);	//경로포함한 파일명
			if(strlen(path_file) > 255){	//길이제한
				fprintf(stderr, "Path is too long : %s\n", path_file);
				return;
			}

			//정수체크
			int len = strlen(argv[2]);
			for(i=0; i<len; i++){
				if(!isdigit(argv[2][i])){
					fprintf(stderr, "No integer!\n");
					return;
				}
			}

			period = atoi(argv[2]);	//5~10으로
			if(period < 5 || period > 10){	//5~10아니면 에러출력후 프롬프트로 복귀
				fprintf(stderr, "period must be 5 ~ 10\n");
				return;
			}

			//해당 이름에 대한 statbuf 구함.
			if(lstat(input_name, &statbuf) < 0){
				fprintf(stderr, "lstat error : %s. Please input correct file\n", input_name);
				return;
			}
			time_t mt = statbuf.st_mtime;

			//옵션존재
			if(argc >= 4){
				for(i=0; i<ADD_LENGTH; i++){
					for(j=3; j<argc; j++){
						if(strcmp(add_option[i], argv[j]) == 0){
							add_mapping_list[i] = j;	//해당 옵션의 사용된 위치 저장
							//0 == m, 1 == d, 2 == n, 3 == t
							//-m, -d는 단독사용됨
							//-n, -t는 뒤의 인자 추가로 받음
						}
					}
				}
			}

			//n 옵션
			if((where = add_mapping_list[2]) > 0){
				if(where+1 > argc-1){	//옵션뒤에 인자가 아예 없으면 에러
					fprintf(stderr, "-n option needs parameter\n");
					return ;
				}

				//전부 int형이어야만 작동함
				int length = strlen(argv[where+1]);
				for(i=0; i<length; i++){
					if(!isdigit(argv[where+1][i])){
						fprintf(stderr, "n- No integer!\n");
						return;
					}
				}

				NUMBER = atoi(argv[where+1]);	
				if(NUMBER < 1 || NUMBER > 100){	//1~100이 아니면 오류 출력 후 프롬프트로 복귀
					fprintf(stderr, "NUMBER must be 1 ~ 100\n");
					return;
				}
			}

			//t 옵션
			if((where = add_mapping_list[3]) > 0){
				if(where+1 > argc-1){	//옵션뒤에 인자가 아예 없으면 에러
					fprintf(stderr, "-t option needs parameter\n");
					return ;
				}

				//전부 int형이어야만 작동함
				int length = strlen(argv[where+1]);
				for(i=0; i<length; i++){
					if(!isdigit(argv[where+1][i])){
						fprintf(stderr, "t- No integer!\n");
						return;
					}
				}

				TIME = atoi(argv[where+1]);
				if(TIME < 60 || TIME > 1200){	//60~1200이 아니면 오류출력후 프롬프트로 복귀
					fprintf(stderr, "TIME must be 60 ~ 1200\n");
					return;
				}
			}

			//d 옵션
			if(add_mapping_list[1] > 0){
				//d옵션일 경우, 매개변수는 디렉토리.
				//디렉토리 내부의 모든파일에 대하여 동일옵션 적용해야함
				DIR* dirp;
				struct dirent *dentry;
				//디렉토리면 진행, 아니면 오류
				if(S_ISDIR(statbuf.st_mode)){
					search_dir(input_name, add_mapping_list, period, NUMBER, TIME, mt);
					chdir(start_path);	//이후 원래자리로 돌아옴
					cur->next = Header->next;
				}
				else{
					fprintf(stderr, "Not Directory : %s\n", input_name);
					return;
				}

			}
			else{	//d옵션일때와 다름
				//옵션까지 전부 체크했으므로 add기능 수행
	
				//reg파일이면 중복확인 후 리스트에 추가함
				//cur->next는 시작시 List를 가리킴
				if(S_ISREG(statbuf.st_mode)){
					realpath(input_name, r_path);	//입력한 파일에 대한 절대경로끼리 비교해야함
					while(cur->next != NULL){
						if(strcmp(cur->next->myfile, r_path) == 0){
							fprintf(stderr, "Same name file : %s\n", r_path);
							cur->next = Header->next;
							return;
						}
						cur->next = cur->next->next;	//커서는 커서의 다음것을 가리킴
					}
						
					//중복없으면 리스트에 추가
					//새로운노드 생성하여 추가	
					struct NODE* newNode = malloc(sizeof(struct NODE));
					newNode->next = Header->next;
					Header->next = newNode;
					strcpy(newNode->filename, filename);	//해당하는 파일명(파일 명)
					strcpy(newNode->input_name, input_name);//상대경로 파일명(사용자 입력이름)
					strcpy(newNode->path_file, path_file);	//절대경로 파일명(백업경로+이름)
					
					char abspath[BUFFER_SIZE] = {0};
					realpath(input_name, abspath);
					strcpy(newNode->myfile, abspath);	//사용자가 입력한 파일의 절대경로
					newNode->period = period;
					
					strcpy(newNode->options, "");	//옵션초기화

					//옵션에맞는 변수추가
					if(add_mapping_list[0] > 0){
						newNode->add_mapping_list[0] = 1;
						newNode->mtime = mt;	//mtime 기록
						strncat(newNode->options, "-m ", 3);
					}
					if(add_mapping_list[1] > 0){
						newNode->add_mapping_list[1] = 1;
						strncat(newNode->options, "-d ", 3);
					}
					if(add_mapping_list[2] > 0){
						newNode->add_mapping_list[2] = 1;
						strncat(newNode->options, "-n ", 3);
						newNode->NUMBER = NUMBER;
					}
					if(add_mapping_list[3] > 0){
						newNode->add_mapping_list[3] = 1;
						strncat(newNode->options, "-t ", 3);
						newNode->TIME = TIME;
					}
					Header->next = newNode;
					cur->next = Header->next;	//헤더 처음으로 되돌림
					list_length++;
					choice = *newNode;		//백업할 파일(노드)구조체
					
					//백업시작
					if(pthread_create(&tid[t_count++], NULL, backup_thread, (void*)&choice) != 0){
						fprintf(stderr, "pthread_craete error\n");
						exit(1);
					}
					newNode->tid = tid[t_count-1];	//본인의 tid 기억(삭제를위함)

					//로그파일 기록
					t = time(NULL);
					tm = localtime(&t);

					char buffer[BUFFER_SIZE] = {0};
					sprintf(buffer, "[%.02d%.02d%.02d %.02d%.02d%.02d] %s added\n",
							tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
							tm->tm_hour, tm->tm_min, tm->tm_sec,
							abspath);
					write(fd, buffer, strlen(buffer));//로그에 기록

				}
				else{
					fprintf(stderr, "No reg file\n");
					return;
				}
			}

		}
		else{
			fprintf(stderr, "At least need 3 parameter : add <FILENAME> [PERIOD] [OPTION]\n");
			return;
		}
	}
	else if(mapping_list[1] == 1){	//remove
		//remove <FILENAME> [OPTION]
		if(argc == 2){
			if(list_length == 0){
				fprintf(stderr, "There is no List\n");
				return;
			}

			//argv[0] == remove, argv[1] == filename or -a
			if(strcmp(argv[1], "-a") == 0){
				//모든 리스트제거
				while(Header->next != NULL){
					pthread_cancel(Header->next->tid);

					//로그파일에 기록
					t = time(NULL);
					tm = localtime(&t);

					char buffer[BUFFER_SIZE+BUFFER_SIZE] = {0};
					sprintf(buffer, "[%.02d%.02d%.02d %.02d%.02d%.02d] %s deleted\n",
							tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
							tm->tm_hour, tm->tm_min, tm->tm_sec,
							Header->next->myfile);
					write(fd, buffer, strlen(buffer));//로그에 기록

					struct NODE* del_node = malloc(sizeof(struct NODE));
					del_node = Header->next;
					Header->next = Header->next->next;
					free(del_node);
					list_length--;
				}
			}
			else{
				//선택받은 리스트만 제거
				char deleted_file_path[BUFFER_SIZE] = {0};
				struct stat statbuf;
				int del_ok = -1;

				if(lstat(argv[1], &statbuf) < 0){	//파일 정보 가짐.
					fprintf(stderr, "lstat error for %s. Please input correct file\n", argv[1]);
					return;
				}

				if(S_ISREG(statbuf.st_mode)){	//해당파일이 일반파일이면 진행
					realpath(argv[1], deleted_file_path);	//절대경로를 확보

					while(cur->next != NULL){
						if(strcmp(cur->next->myfile, deleted_file_path) == 0){
							//삭제하고싶은 파일이 리스트에 존재함
							del_ok = 1;
							break;
						}
						//비교해서 존재하지않으면 다음 노드로
						before->next = cur->next;	//이전노드 기억시킴
						cur->next = cur->next->next;
					}

					if(del_ok == 1){	//리스트에 존재했으면 실행
						struct NODE* del_node = malloc(sizeof(struct NODE));
						del_node = cur->next;

						before->next->next = cur->next->next;	//바로 다음노드 가리키게함

						list_length--;	//리스트 길이 감소
						pthread_cancel(cur->next->tid);	//해당 쓰레드작업 제거

						//로그파일에 기록
						t = time(NULL);
						tm = localtime(&t);
	
						char buffer[BUFFER_SIZE+BUFFER_SIZE] = {0};
						sprintf(buffer, "[%.02d%.02d%.02d %.02d%.02d%.02d] %s deleted\n",
								tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
								tm->tm_hour, tm->tm_min, tm->tm_sec,
								cur->next->myfile);
						write(fd, buffer, strlen(buffer));//로그에 기록

						cur->next = Header->next;	//각 커서들 리셋
						before->next = Header;

						free(del_node);//메모리해제
					}
					else{	//리스트에 존재하지않았으므로 에러출력
						fprintf(stderr, "There is no file in list : %s\n", argv[1]);
						return;
					}
				}
			}
		}
		else{
			fprintf(stderr, "Need 2 parameter : remove <FILENAME> / remove [OPTION]\n");
			return;
		}
	}
	else if(mapping_list[2] == 1){	//compare
		//compare <FILENAME1> <FILENAME2>
		if(argc == 3){
			struct stat statbuf_1;
			struct stat statbuf_2;

			//올바른파일이 아니면 에러출력 후 복귀
			if(lstat(argv[1], &statbuf_1) < 0){
				fprintf(stderr, "lstat error for %s. Please input correct file\n", argv[1]);
				return;
			}

			if(lstat(argv[2], &statbuf_2) < 0){
				fprintf(stderr, "lstat error for %s. Please input correct file\n", argv[2]);
				return;
			}

			//둘중 하나라도 일반파일이 아니면 에러출력 후 복귀
			if(S_ISREG(statbuf_1.st_mode) && S_ISREG(statbuf_2.st_mode)){
				if((statbuf_1.st_mtime == statbuf_2.st_mtime) && (statbuf_1.st_size == statbuf_2.st_size)){
					//mtime과 사이즈가 모두 동일하면 같은파일
					printf("%s and %s are same file\n", argv[1], argv[2]);
				}
				else{	//다르면 각각 출력
					printf("%s's mtime : %ld, size : %ld\n", argv[1], statbuf_1.st_mtime, statbuf_1.st_size);
					printf("%s's mtime : %ld, size : %ld\n", argv[2], statbuf_2.st_mtime, statbuf_2.st_size);
				}
			}
			else{
				fprintf(stderr, "File is not REG file\n");
				return;
			}
		}
		else{	//입력인자로서 2개를 못받으면 에러출력 후 복귀
			fprintf(stderr, "Need 3 parameter : compare <FILENAME1> <FILENAME2>\n");
			return;
		}
	}
	else if(mapping_list[3] == 1){	//recover
		//recover <FILENAME> [OPTION] (NEWFILE)
		if(argc >= 2){	//옵션이 있을수도, 없을수도있는 최소수치 2
			if(argc == 3 || argc > 4){	//옵션이 3개만 입력되면 옵션인자가 빠진것이므로 에러출력
				fprintf(stderr, "Option need Parameter (NEWFILE) or too many arg\n");
				return;
			}
			else{
				if(argc == 4 && strcmp(argv[2], "-n") == 0){
					if(access(argv[3], F_OK) == 0){	//복사하려는 파일이 이미 존재하면아웃
						fprintf(stderr, "Already %s is exist\n", argv[3]);
						return;
					}
				}

				char file_name_end[BUFFER_SIZE] = {0};
				char file_realpath[BUFFER_SIZE] = {0};
				struct stat r_statbuf;
				int rec_ok = 0;
				
				if(lstat(argv[1], &r_statbuf) < 0){	//해당 파일에 대한 정보를 가짐
					fprintf(stderr, "lstat error - Wrong File : %s\n", argv[1]);
					return;
				}

				realpath(argv[1], file_realpath);	//찾으려는 파일의 절대경로 확보

				while(cur->next != NULL){
					if(strcmp(cur->next->myfile, file_realpath) == 0){
						//해당 파일이 리스트내에 존재하면 멈춤
						rec_ok = 1;
						break;
					}
					before->next = cur->next;	//이전노드 가리킴
					cur->next = cur->next->next;
				}

				//cur->next는 현재 삭제 및 복구될 노드
				if(rec_ok == 1){
					//시간순서대로 리스트를 보여주며, 0번 입력시 프롬프트복귀
					//선택되고 파일이 존재했을경우 그 파일로 덮어쓰며, 완료메시지출력
					//파일이 존재하지 않으면 에러처리후 복귀

					pthread_cancel(cur->next->tid);	//해당 쓰레드 취소

					strcpy(file_name_end, cur->next->filename);
					file_name_end[strlen(file_name_end)] = '_';	//끝에 _ 붙임

					struct recover{
						char filepath[BUFFER_SIZE];
						long long int date;
						off_t size;
					};

					struct recover recover_struct[105];
					struct recover reco_temp;
					DIR* dirp;
					struct dirent *dentry;
					int reco_count = 0;
					int key;

					//해당 디렉토리 오픈 후 이동
					if((dirp = opendir(backup_path)) == NULL || chdir(backup_path) == -1){
						fprintf(stderr, "opendir or chdir error %s\n", backup_path);
						return;
					}

					//문서 탐색시작
					while((dentry = readdir(dirp)) != NULL){
						struct stat s_statbuf;
						char dname[BUFFER_SIZE] = {0};
						strcpy(dname, dentry->d_name);	//원본소실 방지

						if(lstat(dname, &s_statbuf) < 0){
							continue;	//해당파일없으면 스킵
						}

						char* ptr = strstr(dname, file_name_end);
						if(ptr != NULL){	//발견되면 리스트에 담음
							char* pptr = strtok(dname, file_name_end);
							recover_struct[reco_count].date = atoll(pptr);
							recover_struct[reco_count].size = s_statbuf.st_size;
							realpath(dname, recover_struct[reco_count].filepath);
							reco_count++;
						}
					}

					//소팅
					for(i=0; i<reco_count; i++){
						for(j=0; j<reco_count-(i+1); j++){
							if(recover_struct[j].date > recover_struct[j+1].date){
								reco_temp = recover_struct[j+1];
								recover_struct[j+1] = recover_struct[j];
								recover_struct[j] = reco_temp;
							}
						}
					}

					//리스트출력
					for(i=-1; i<reco_count; i++){
						if(i == -1)
							printf("0. exit\n");
						else{
							printf("%d. %lld	%ldbytes\n",
								   	i+1, recover_struct[i].date, recover_struct[i].size);
						}
					}
					char cc;
					printf("Choose file to recover : ");
					scanf("%d", &key);
					scanf("%c", &cc);	//개행무시
					
					if(key == 0)
						return;

					printf("recover file : %s\n", recover_struct[key-1].filepath);

					if(chdir(start_path) == -1){//디렉토리 복귀
						fprintf(stderr, "chdir error for %s\n", start_path);
						return;
					}	

					//해당 파일을 리스트에서 제거하며 쓰레드도 제거, 그리고 복사한다.
					//옵션분기
					char command[BUFFER_SIZE+BUFFER_SIZE+300] = {0};
					if(argc == 4 && strcmp(argv[2], "-n") == 0){	//옵션이 걸렸고, argc[3]에 파일인자가 존재하면
						if(access(recover_struct[key-1].filepath, F_OK) == 0){
							sprintf(command, "cp %s %s", recover_struct[key-1].filepath, argv[3]);
						}
						else{
							fprintf(stderr, "Can't find file %s\n", recover_struct[key-1].filepath);
							return;
						}
					}
					else{	//옵션없음
						if(access(recover_struct[key-1].filepath, F_OK) == 0)
							sprintf(command, "\\cp -f %s %s", recover_struct[key-1].filepath, cur->next->myfile);
						else{
							fprintf(stderr, "Can't find file %s\n", recover_struct[key-1].filepath);
							return;
						}
					}
					system(command);	//덮어씀


					//리스트 제거
					struct NODE* del_node = malloc(sizeof(struct NODE));
					del_node = cur->next;

					before->next->next = cur->next->next;	//바로 다음노드 가리키게함

					free(del_node);
					list_length--;	//리스트 길이 감소


					//모든 일 마치면 출력
					printf("Recovery success\n");
					cur->next = Header->next;
					before->next = Header;
				}
				else{	//해당 파일이 없었음
					cur->next = Header->next;	//노드 복구
					before->next = Header;
					fprintf(stderr, "There is no file in List : %s\n", argv[1]);
					return;
				}
			}
		}
		else{		//파라미터가 없으면 에러출력후 돌아감
			fprintf(stderr, "At least need 2 Parameter : recover <FILENAME> [OPTION] (NEWFILE)\n");
			return;
		}
	}
	else if(mapping_list[4] == 1){	//list
		while(cur->next != NULL){
			printf("%s	%d	%s\n", cur->next->myfile, cur->next->period, cur->next->options);
			cur->next = cur->next->next;
		}
		cur->next = Header->next;
	}
	else if(mapping_list[5] == 1){	//ls
		if(argc < 2){	//ls만 입력받았을 경우
			system("ls");
		}
		else{
			char command[BUFFER_SIZE] = {"ls "};
			for(i=1; i<argc; i++){
				strcat(command, argv[i]);
				strncat(command, " ", 1);
			}
			system(command);
		}
	}
	else if(mapping_list[6] == 1){	//vi
		if(argc < 2){	//vi만 입력받았을 경우
			system("vi");
		}
		else{
			char command[BUFFER_SIZE] = {"vi "};
			for(i=1; i<argc; i++){
				strcat(command, argv[i]);
				strncat(command, " ", 1);
			}
			system(command);
		}
	}
	else if(mapping_list[7] == 1){	//vim
		if(argc < 2){	//vim만 입력받았을 경우
			system("vim");
		}
		else{
			char command[BUFFER_SIZE] = {"vim "};
			for(i=1; i<argc; i++){
				strcat(command, argv[i]);
				strncat(command, " ", 1);
			}
			system(command);
		}
	}
	else if(mapping_list[8] == 1){	//exit
		exit(0);
	}
}

void *backup_thread(void *arg)
{
	int i,j;
	struct NODE node = *(struct NODE*)arg;	//매개변수로 구조체넘겨받음
	char end_name[BUFFER_SIZE] = {0};
	strcpy(end_name, node.filename);
	char input_name[BUFFER_SIZE] = {0};			//상대경로 이름
	strcpy(input_name, node.input_name);

	char path_file[BUFFER_SIZE] = {0};
	strcpy(path_file, node.path_file);	//파일이름을 포함한 절대경로

	time_t mt = node.mtime;
	int period = node.period;	//이 시간마다 백업을 진행함
	int maxNUM = node.NUMBER;	//해당 개수 넘어가면 가장 나중의것삭제됨
	int TIME = node.TIME;	//해당 시간이 넘어가면 백업파일 제거함
	char backupTIME[BUFFER_SIZE];	//경로 뒤에붙을 백업시간
	int file_count = 0;	//루프마다 1씩증가하며 이 값은 maxNUM과 관련있음
	
	char myfile[BUFFER_SIZE] = {0};
	strcpy(myfile, node.myfile);	//백업할 원본파일의 절대경로

	char del_array[MAX][BUFFER_SIZE] = {0};
	char t_array[MAX][BUFFER_SIZE] = {0};

	while(1){
		pthread_mutex_lock(&mutex);	//락 걸어 안겹치게진행

		t = time(NULL);
		tm = localtime(&t);
	
		char filename[BUFFER_SIZE] = {0};
		char fullname[BUFFER_SIZE] = {0};
		//period마다 반복하며 백업함
		//생성시간 매번 확인하여 t옵션만큼의 시간이 지났으면 해당파일 삭제
		//파일개수가 maxNUM인 상태에서 루프를 하면 파일 전부삭제 후 백업함

		//-n 옵션이 걸려있고, 개수가 넘어가면 시행
		if(node.add_mapping_list[2] > 0){
			//리스트내의 파일중 가장 늦은것만 날림
			DIR* dirp;
			int fcount = 0;
			struct dirent *dentry;
			time_t time_array[105] = {0};
			time_t tmin;
			int first_t = 1;
			struct stat t_statbuf;

			dirp = opendir(backup_path);		//백업파일 디렉토리를 오픈
			while((dentry = readdir(dirp)) != NULL){
				if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
					continue;	//.과 ..은무시함
				if(strstr(dentry->d_name, end_name) != NULL){	//해당파일이름에 파일명존재시
					char real_dname[BUFFER_SIZE] = {0};
					sprintf(real_dname, "%s/%s", backup_path, dentry->d_name);	//절대경로로 저장
					strcpy(del_array[fcount], real_dname);

					lstat(del_array[fcount], &t_statbuf);	//해당파일 mtime 저장해야함
					if(first_t == 1){
						first_t = 0;
						tmin = t_statbuf.st_mtime;	//가장 초기값이 최소값으로 설정
					}

					if(tmin > t_statbuf.st_mtime){	//최소값을 찾음
						tmin = t_statbuf.st_mtime;
					}
					time_array[fcount] = t_statbuf.st_mtime;
					fcount++;
				}
			}
			
			if(fcount >= maxNUM){	//fcount가 넘어가면 전부 삭제함(절대경로)
				char command[BUFFER_SIZE] = {0};
				for(i=0; i<fcount; i++){
					if(time_array[i] != tmin){
						continue;	//최소값과 맞지 않는 부분은 다 넘김
					}
					if(access(del_array[i], F_OK) == 0){
						sprintf(command, "rm %s", del_array[i]);
						system(command);
						strcpy(del_array[i], "");
					}
				}
			}
		}
		
		//-t 옵션이 걸려있으면, 리스트 스캔하며 생성시간 비교 후 삭제함
		if(node.add_mapping_list[3] > 0){
			DIR* dirp;
			struct dirent *dentry;

			dirp = opendir(backup_path);		//백업디렉토리 오픈
			while((dentry = readdir(dirp)) != NULL){	//해당 디렉토리내 파일들 read
				if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0)
					continue;	//. 과 ..는 무시한다
				struct stat t_statbuf;
				char real_dname[BUFFER_SIZE] = {0};
				sprintf(real_dname, "%s/%s", backup_path, dentry->d_name);	//절대경로로저장
				if(lstat(real_dname, &t_statbuf) < 0){	//해당 파일의 정보를 읽어들임
					continue;
				}
				time_t current_time;		//현재시간
				time(&current_time);

				//해당 파일의 mtime과 현재시간의 차가 지정한 타임보다 크거나 같으면 삭제진행
				if((current_time - t_statbuf.st_mtime) >= TIME){
					char command[BUFFER_SIZE] = {0};
					if(access(real_dname, F_OK) == 0){
						sprintf(command, "rm %s", real_dname);
						system(command);
					}
				}
			}
		}

		//-m 옵션이 걸려있고, mtime이 서로 다르면 원본 백업을 진행한다.
		if(node.add_mapping_list[0] > 0){
			struct stat statbuf;
			char real_name[BUFFER_SIZE] = {0};
			realpath(input_name, real_name);

			//사용자가 입력했던 파일의 mtime을 확인
			if(lstat(real_name, &statbuf) < 0){
				continue;
			}

			//원본의 mtime이 바뀌면 백업한다.
			if(statbuf.st_mtime != mt){
				mt = statbuf.st_mtime;	//mt값 갱신
				
				sprintf(backupTIME, "%.02d%.02d%.02d%.02d%.02d%.02d",
						tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec);
				sprintf(fullname, "%s_%s", path_file, backupTIME);	//절대경로 포함한이름

				char command[BUFFER_SIZE] = {0};
				
				sprintf(command, "cp %s %s", input_name, fullname);
				char buffer[BUFFER_SIZE] = {0};
				sprintf(buffer, "[%.02d%.02d%.02d %.02d%.02d%.02d] %s generated\n",
						tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
						tm->tm_hour, tm->tm_min, tm->tm_sec,
						fullname);
				write(fd, buffer, strlen(buffer));//로그에 기록

				system(command);

				pthread_mutex_unlock(&mutex);
				sleep(period);
				file_count++;
			}
			else{	//변함이없으면 Period만큼 sleep시키고 다음으로 진행시킴
				pthread_mutex_unlock(&mutex);
				sleep(period);
				continue;
			}
		}
		else{	//mitme없으면 기존의 백업을 진행
			sprintf(backupTIME, "%.02d%.02d%.02d%.02d%.02d%.02d",
					tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec);
			sprintf(fullname, "%s_%s", path_file, backupTIME);	//절대경로 포함한이름

			//filename에 시간을 붙인 파일명으로 백업파일이 복사되어 생성된다.
			char command[BUFFER_SIZE] = {0};
			sprintf(command, "cp %s %s", myfile, fullname);
			system(command);

			char buffer[BUFFER_SIZE] = {0};
			sprintf(buffer, "[%.02d%.02d%.02d %.02d%.02d%.02d] %s generated\n",
					tm->tm_year-100, tm->tm_mon+1, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec,
					fullname);
			write(fd, buffer, strlen(buffer));//로그에 기록

			pthread_mutex_unlock(&mutex);	//모든과정이끝나면 락 해제 
			//잠들어있는동안 락을 해제한다.
			sleep(period);
			file_count++;
		}
	}
}

void search_dir(char* dir, int* add_mapping_list, int period, int NUMBER, int TIME, time_t mt)
{
	//dir은 디렉토리임을 보장받음
	//d옵션일 경우, 매개변수는 디렉토리.
	//디렉토리 내부의 모든파일에 대하여 동일옵션 적용해야함
	DIR* dirp;
	struct dirent *dentry;
	struct stat statbuf;
	struct NODE choice[MAX] = {0};
	int choice_count = 0;
	char fname[BUFFER_SIZE] = {0};
	char thispath[BUFFER_SIZE] = {0};

	//디렉토리 만나면 재귀, 아니면 노드에 추가 후 리턴

	if((dirp = opendir(dir)) == NULL || chdir(dir) == -1){
		//해당 디렉토리로 이동
		fprintf(stderr, "opendir, chdir error : %s\n", dir);
		return;
	}
	getcwd(thispath, BUFFER_SIZE);	//현재위치 저장

	while((dentry = readdir(dirp)) != NULL){
		if(dentry->d_ino == 0)
			continue;

		memset(fname, 0, strlen(fname));	//이름초기화
		strcpy(fname, dentry->d_name);	//이름 카피

		char abspath[BUFFER_SIZE] = {0};
		realpath(fname, abspath);

		//. ..은 무시함
		if(strcmp(fname, ".") == 0 || strcmp(fname, "..") == 0){
			continue;
		}

		if(lstat(fname, &statbuf) < 0){
			//정보를 구해올수 없는 파일에 대해선 스킵함
			continue;
		}

		if(S_ISDIR(statbuf.st_mode)){	//디렉토리를 만날경우 재귀
			search_dir(fname, add_mapping_list, period, NUMBER, TIME, mt);
			chdir(thispath);	//다시 원래자리로 돌아옴
		}
		else if(S_ISREG(statbuf.st_mode)){	//일반파일이면 노드에 추가해야함. (옵션도 추가)
			int same_check = 0;
			while(cur->next != NULL){
				if(strcmp(cur->next->myfile, abspath) == 0){
					cur->next = Header->next;
					same_check = 1;		//같은이름나오면 이 루프를 깨고 변수를 1로만듬
					break;
				}
				cur->next = cur->next->next;	//리스트 끝까지 탐색
			}

			if(same_check == 1){	//같은이름이 발견되면 다음과정으로 스킵함
				fprintf(stderr, "Same name file : %s\n", abspath);
				continue;
			}

			struct NODE* newNode = malloc(sizeof(struct NODE));
			newNode->next = Header->next;
			Header->next = newNode;
			strcpy(newNode->filename, fname); //파일이름

			char input_path[BUFFER_SIZE] = {0};
			sprintf(input_path, "%s/%s", thispath ,fname);
			strcpy(newNode->input_name, input_path);	//경로를 포함한 파일이름(원본)

			char path_name[BUFFER_SIZE] = {0};
			sprintf(path_name, "%s/%s", backup_path, fname);
			strcpy(newNode->path_file, path_name);	//경로포함한 절대경로로서의 이름(복제될곳)

			strcpy(newNode->myfile, abspath);
			newNode->period = period;

			if(add_mapping_list[0] > 0){
				newNode->add_mapping_list[0] = 1;
				newNode->mtime = mt;
				strncat(newNode->options, "-m ", 3);
			}
			
			if(add_mapping_list[1] > 0){
				newNode->add_mapping_list[1] = 1;
				strncat(newNode->options, "-d ", 3);
			}

			if(add_mapping_list[2] > 0){
				newNode->add_mapping_list[2] = 1;
				strncat(newNode->options, "-n ", 3);
				newNode->NUMBER = NUMBER;
			}

			if(add_mapping_list[3] > 0){
				newNode->add_mapping_list[3] = 1;
				strncat(newNode->options, "-t ", 3);
				newNode->TIME = TIME;
			}

			Header->next = newNode;
			cur->next = Header->next;	//cur 리셋
			list_length++;
			choice[choice_count] = *newNode;
			
			//반복문임에 따라, choice가 배열이 아니면 원본이 소실되는 문제점이 발생하므로
			//배열로 만들어서, 원본이 유지되게 넘겨준다.
			//쓰레드의 매개변수는 포인터임
			if(pthread_create(&tid[t_count++], NULL, backup_thread, (void*)&choice[choice_count]) != 0){
				fprintf(stderr, "pthread_create error\n");
				exit(1);
			}
			newNode->tid = tid[t_count-1];	//본인의 tid 기억
			choice_count++;
		}
	}

	return;
}
