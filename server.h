//This function reads input from the client until it reaches a terminating state,
//that being buff[0] != 0 indicating a change of phase. After this function is
//called, input can again be read from the client.
void start(int connfd, char * buff);

//This function returns the size of the name of the output file, so that the
//char array "name" can be correctly allocated. It records the number of -1s
//written by the client and increments int "size" for every -1.
int readFileSize(int connfd, char * buff);

//This function reads in the filename of the file that the server needs to open.
//It reads from the client into char array "buff" 5 bytes at a time. It then
//checks to see if the first element is 0 indicating that the server should stop
//reading and initiate the next phase.
void readFile(int connfd, char * buff, char * temp);

//This function uses the same implementation as readFile, except that it also
//writes to the data it receives in "buff" to the output file.
void readContent(int connfd, char * buff, FILE * out_file);
