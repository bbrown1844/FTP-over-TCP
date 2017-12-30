
//This function sends over the size of the filename.
void sendFileSize(int sockfd, char * buff, int name_size);

//This function sends over the filename to the server.
void sendFile(int sockfd, char * buff, char * temp);

//This function sends over the contents of the file to the server.
void sendContent(int sockfd, char * buff, FILE * in_file);
