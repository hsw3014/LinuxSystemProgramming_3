#ifndef BACKUPSTART
#define BACKUPSTART
#define BUFFER_SIZE 1024
#define MAX 255
#define C_LENGTH 9
#define ADD_LENGTH 4
#define REM_LENGTH 1
#define REC_LENGTH 1
char *command_list[C_LENGTH] = {
	"add", "remove", "compare", "recover", "list", "ls", "vi", "vim", "exit"};
//char mapping_list[C_LENGTH] = {0};

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

//Add
char add_option[ADD_LENGTH][ADD_LENGTH] = {
	"-m", "-d", "-n", "-t"};

//Remove
char rem_option[REM_LENGTH][REM_LENGTH+3] = {
	"-a"};

//Recover
char rec_option[REC_LENGTH][REC_LENGTH+3] = {
	"-n"};

pthread_t tid[BUFFER_SIZE] = {0};
int t_count = 0;
#endif
