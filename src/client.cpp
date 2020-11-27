// Client side C/C++ program to demonstrate Socket programming
#include "../include/header.h"
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/sha.h>

#define TEMP_PART_FOLDER "/home/foo/Downloads/AOS/assignment2/minitorrent/temp/"
#define DEBUG 1
#define IFD if (DEBUG)
using namespace std;
#define BUFFER_SIZE 512 * 1024
int server_c_fd, peer_fd[10];
pthread_t thread_c[10];
int opt = 1;
int sock = 0, valread;
struct sockaddr_in serv_addr;
string hello = "create_group g1";
char buffer[1024] = {0};
char sendtoTrackerBuff[1024] = {0};
char recvFromTrackerBuff[1024] = {0};
pthread_t thread1, thread2, pt, thread_main;
int c_port;
void *writeToTracker(void *);
void *senderToReciever(void *);
void *recieverToSender(void *);
void *p2p(void *);

vector<string> getSHA(string fpath)
{
	cout << "fpath is " << fpath << endl;
	vector<string> result;
	vector<string> piecewiseSHA;
	FILE *fp = fopen(fpath.c_str(), "r+");
	if (fp == NULL)
	{
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	rewind(fp);

	unsigned char buffer[BUFFER_SIZE];
	unsigned char hash1[20];
	char partial[40];
	string total_chunk_string = "";
	int n;

	int numOfPieces = 0;
	while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0)
	{ //for every 512K piece
		// file_size = file_size-n;
		SHA1(buffer, n, hash1);
		bzero(buffer, BUFFER_SIZE); //reset buffer
		for (int i = 0; i < 20; i++)
		{ //convert to hex rep
			sprintf(partial + 2 * i, "%02x", hash1[i]);
		}
		piecewiseSHA.push_back(partial);
		cout << partial << "\n";
		numOfPieces++;
		// total_chunk_string += partial; //append hashes of all pieces into this
	}
	// result.push_back(total_chunk_string);

	// return result;
	return piecewiseSHA;
}

string findName(string path)
{
	vector<string> t;
	stringstream check1(path);
	string intermediate;
	while (getline(check1, intermediate, '/'))
	{
		t.push_back(intermediate);
	}
	for (auto i = t.begin(); i != t.end(); i++)
	{
		cout << *i << " ";
	}
	cout << "t[t.size() - 1]" << t[t.size() - 1] << endl;
	return t[t.size() - 1];
}
struct shaInfo
{
	string fileName;
	unordered_map<int, string> pieceWiseSHA;
};
vector<struct shaInfo> shaInfoPerFile;
unordered_map<string,string> filehash;

void *writeToTracker(void *p)
{
	char *ptr;
	while (1)
	{
		cin.getline(sendtoTrackerBuff, 1024, '\n');
		//if download: define the destn path of file
		//if upload
		//
		cout << "client entered: " << endl;
		cout << sendtoTrackerBuff << endl;
		vector<string> token;
		stringstream check1(sendtoTrackerBuff);
		string intermediate;
		while (getline(check1, intermediate, ' '))
		{
			token.push_back(intermediate);
		}
		if (token[0] == "login")
		{
			string cp = to_string(c_port);
			cp = " " + cp;
			char c[10];
			strcpy(c, cp.c_str());
			strcat(sendtoTrackerBuff, c);
			cout << "send buffer is " << sendtoTrackerBuff << "\n";
		}
		if (token[0] == "upload_file")
		{
			cout << "token [0] is " << token[0] << endl;
			cout << "token [1] is " << token[1] << endl;
			cout << "token [2] is " << token[2] << endl;
			string filename = findName(token[1]);
			// string filename = "b.txt";
			cout << "filename is " << filename << endl;
			vector<string> vect;
			vect = getSHA(token[1]);
			int n = vect.size();
			FILE *fp = fopen(filename.c_str(), "r");
			if (fp == NULL)
			{
				printf("File Not Found!\n");
				// return -1;
			}
			fseek(fp, 0L, SEEK_END);
			long long fileSize = ftell(fp);
			// closing the file
			fclose(fp);

			long long chunkSize = 512 * 1024;
			long long numOfChunks = ceil(fileSize / (chunkSize * 1.0)); //1 chunk is 512KB
			cout << "file size is " << fileSize << endl;
			cout << "number of chunks: " << numOfChunks << endl;
			struct shaInfo f;
			f.fileName = filename;
			string totalFileSHA;
			for (int i = 0; i < n; i++)
			{
				f.pieceWiseSHA[i] = vect[i];
				cout << i << " " << f.pieceWiseSHA[i] << endl;
				totalFileSHA = totalFileSHA + f.pieceWiseSHA[i];
			}
			filehash[filename] = totalFileSHA;
			cout<<"filehash[filename] "<<filehash[filename]<<endl;
			shaInfoPerFile.push_back(f);
			cout << "toatla file sha is " << totalFileSHA << endl;
			string chunks = to_string(numOfChunks);
			chunks = " " + chunks;
			char c[10];
			strcpy(c, chunks.c_str());
			// char chunks[10] =  (to_string(numOfChunks)).c_str();
			// sendtoTrackerBuff = sendtoTrackerBuff +" " + (to_string(numOfChunks));
			strcat(sendtoTrackerBuff, c);
			cout << "concatnated buffer with numer of chunks are: " << sendtoTrackerBuff;
		}

		if (token[0] == "download_file")
		{
			// download_file​ <group_id> <file_name> <destination_path>
			string fName = token[2];
			string destinationPath = token[3];
			destinationPath = "./" + destinationPath + "/" + fName;
		}
		cout << "writing to socket" << endl;
		cout << sendtoTrackerBuff;
		write(sock, sendtoTrackerBuff, 1024);
	}

	//  send(sock,sendtoTrackerBuff, strlen(sendtoTrackerBuff), 0);
}

void *readFromTracker(void *p)
{
	memset(recvFromTrackerBuff, '\0', 1024);
	while (1)
	{
		// cout << "tok[0] is " << tok[0] << endl;

		read(sock, recvFromTrackerBuff, 1024);
		// cout << "msg recieved from tracker is " << recvFromTrackerBuff;

		if (strcmp(recvFromTrackerBuff, "2000") == 0)
		{
			cout << "USER_CREATED" << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2001") == 0)
		{
			cout << "USER_CREATION_UNSUCCESSFUL" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2100") == 0)
		{
			cout << "LOGIN_SUCCESSFUL" << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2101") == 0)
		{
			cout << "LOGIN_UNSUCCESSFUL" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2200") == 0)
		{
			cout << "GROUP_CREATED" << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2201") == 0)
		{
			cout << "GROUP_NOT_CREATED" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2300") == 0)
		{
			cout << "GROUP_JOINED" << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2301") == 0)
		{
			cout << "GROUP_NOT_JOINED" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2400") == 0)
		{
			cout << "GROUP_LEFT" << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2401") == 0)
		{
			cout << "GROUP_NOT_LEFT" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2500") == 0)
		{
			cout << "FILE_UPLOAD_SUCCESSFUL" << endl;
		}
		else if (recvFromTrackerBuff[0] == 'u')
		{
			cout << recvFromTrackerBuff << endl;
		}
		else if (recvFromTrackerBuff[0] == 'g')
		{
			cout << recvFromTrackerBuff << endl;
		}
		else if (recvFromTrackerBuff[0] == 'f')
		{
			cout << recvFromTrackerBuff << endl;
		}
		else if (strcmp(recvFromTrackerBuff, "2700") == 0)
		{
			cout << "REQ_ACCEPT" << endl;
		}

		else if (strcmp(recvFromTrackerBuff, "2501") == 0)
		{
			cout << "FILE_UPLOAD_UNSUCCESSFUL" << endl;
		}
		else if (recvFromTrackerBuff[0] == 'D')
		{
			// cout << "tok[0] is " << tok[0] << endl;
			cout << recvFromTrackerBuff << endl;
			vector<string> tok;
			stringstream check(recvFromTrackerBuff);
			string interm;
			while (getline(check, interm, ' '))
			{
				tok.push_back(interm);
			}
			vector<string>::iterator it;
			cout << "in download " << endl;
			it = tok.begin();
			tok.erase(it);

			it = tok.end() - 1;
			string numOfchunks = *it;
			cout << "chunks are " << numOfchunks << endl;
			// tok.erase(it);

			it = tok.end() - 2;
			string fileName = *it;
			cout << "filename is " << fileName << endl;
			// tok.erase(it);

			//create a thread (pass port list)to do download and call the p2p function
			//join it

			string portList;
			for (auto i = tok.begin(); i != tok.end(); i++)
			{
				portList = portList + *i + " ";
			}
			cout << "port list is :" << portList << endl; //now port list has 7001 b.txt 1

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			string *parameter = &portList;
			pthread_create(&pt, &attr, p2p, (void *)parameter);
		}
		else
		{
			cout << "no matching data recieved " << endl;
		}
		pthread_join(pt, NULL); //outside loop
		memset(recvFromTrackerBuff, '\0', 1024);
	}
}

void *peerToTracker(void *params)
{
	printf("here\n");
	int *p = (int *)params;
	printf("port is %d\n", *p);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(*p);
	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		// return;
	}
	// The connect() system call connects the socket referred
	// to by the file descriptor
	// sockfd to the address specified by addr.
	//  Server’s address and port is specified in addr.
	printf("connect\n");
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("\nConnection Failed \n");
		// return;
	}
	printf("trying to send message to server\n");

	// send(sock, hello.c_str(), strlen(hello.c_str()), 0); //peertotracker
	// printf("Hello message sent\n");

	//first implementing client to server functionality

	pthread_attr_t attr1, attr2;
	pthread_attr_init(&attr1);
	pthread_attr_init(&attr2);

	pthread_create(&thread1, &attr1, writeToTracker, (void *)p);
	pthread_create(&thread2, &attr2, readFromTracker, (void *)p);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}

void *p2p(void *args) //args has <port_num_list><filename><num_of_chunks> ie 7000 8000 inp.txt 1
{
	string portList = *((string *)args);
	vector<string> port;
	stringstream check1(portList);
	string intermediate;
	while (getline(check1, intermediate, ' '))
	{
		port.push_back(intermediate);
	}
	auto it = port.end() - 1;
	string chunks = *it;
	port.erase(it);
	it = port.end() - 1;
	string fName = *it;
	port.erase(it);

	int numOfPorts = port.size();
	int i = 0;
	pthread_t th1[numOfPorts];
	while (i < numOfPorts)
	{
		string pList = portList;
		portList = portList + " " + to_string(i);
		// int *p = (int *)(stoi(port[i]));
		// cout << "p is " << *p;

		// string info = "0 7001 1 a.txt 1";
		string info = to_string(i) + " " + port[i] + " " + to_string(numOfPorts) + " " + fName + " " + chunks;
		char *infoStr = (char *)malloc(512 * sizeof(char));

		strcpy(infoStr, info.c_str());

		printf("connection established to peer \n");
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&th1[i], &attr, recieverToSender, (void *)infoStr);
		//recieverToSender((void *)infoStr);

		// portList = pList;
		i++;
	}

	for (int i = 0; i < numOfPorts; i++)
	{
		pthread_join(th1[i], NULL);
	}
}

char sbuff[1024] = {0};
char rbuff[1024] = {0};
void *recieverToSender(void *args)
{
	//i, totalports,filename,chunk
	// sending file_name to another peer
	cout << "recieverToSender called with args : " << (char *)args << endl;
	//  getchar();
	string list = string((char *)args);

	cout << "list is " << list << endl;
	vector<string> token;
	stringstream check1(list);
	string intermediate;
	while (getline(check1, intermediate, ' '))
	{
		token.push_back(intermediate);
	}
	int nChunks = stoi(token[4]);
	string filename = token[3];
	int numOfPotSenders = stoi(token[2]);
	int designatedSenderPort = stoi(token[1]);
	int designatedSenderIndex = stoi(token[0]);

	int nChunksPerSender = ceil(nChunks / (numOfPotSenders * 1.0));
	cout << "no of chunks are " << nChunksPerSender << endl;

	// designatedSenderIndex*nChunksPerSender to (designatedSenderIndex+1)*nChunksPerSender -1
	// 0 -> 0*5 to 1*5-1

	// sending connect to the designated sender
	int senderSocketFD;
	if ((senderSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(designatedSenderPort);
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0)
	{
		printf("\nInvalid address/ Address not supported \n");
		// return;
	}
	printf("connect\n");
	if (connect(senderSocketFD, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		printf("\nConnection Failed to port : %d\n", designatedSenderPort);
	}
	cout << "connection done " << endl;

	char readBuffer[BUFFER_SIZE];
	long totBytesRead, bytesRead;
	//Do for every available chunk
	// vector<string> vect;
	int k = 0;
	cout << "designatedSenderIndex is " << designatedSenderIndex << endl;
	for (int chunkNo = designatedSenderIndex * nChunksPerSender; chunkNo <= (designatedSenderIndex + 1) * nChunksPerSender - 1; chunkNo++)
	{
		string sendString = filename + " " + to_string(chunkNo);
		strcpy(sbuff, sendString.c_str());

		//send piece request
		cout << "sending request  for piece : " << chunkNo << endl;
		write(senderSocketFD, sbuff, 1024);

		//receive piece size from sender
		read(senderSocketFD, rbuff, 32);
		cout << "piece size is " << rbuff << endl;
		long pieceSize = stol(rbuff);

		size_t datasize;
		string destPartFilePath = TEMP_PART_FOLDER + filename + "_part" + to_string(chunkNo);
		FILE *fd = fopen(destPartFilePath.c_str(), "wb");

		totBytesRead = 0;
		while (totBytesRead < pieceSize)
		{
			bytesRead = recv(senderSocketFD, readBuffer, pieceSize - totBytesRead, 0);
			cout << "bytes read =" << bytesRead << endl;
			totBytesRead += bytesRead;
			fwrite(readBuffer, sizeof(char), bytesRead, fd);
			// cout<<readBuffer<<endl;
			bzero(readBuffer, sizeof(readBuffer));
		}
		fclose(fd);
	}

	//send bye to sender

	// system("echo -n '1. Current Directory is '; pwd");

	string s = "cat /home/foo/Downloads/AOS/assignment2/minitorrent/temp/" + filename + "_* > /home/foo/Downloads/AOS/assignment2/minitorrent/temp/inp.txt_joined ";
	const char *command = s.c_str();
	system(command);
	
	//merge all part files and take out there hash
	vector<string> vect;
	string path = "/home/foo/Downloads/AOS/assignment2/minitorrent/temp/inp.txt_joined";
	vect = getSHA(path);
	string recievedFileHash;
	for(auto i =vect.begin();i!=vect.end();i++)
	{
		recievedFileHash = recievedFileHash + *i;
	}
	cout<<"recieved hash "<<recievedFileHash<<endl;
	cout<<"filehash[filename] "<<filehash[filename]<<endl;
	if(filehash[filename] == recievedFileHash)
	cout<<"hash matched";
	cout<<"hash matches!!!";
	string sendString = "BYE";
		cout << "Sending bye to sender" << endl;
	write(senderSocketFD, sendString.c_str(), 1024);
	cout << "here";
	close(senderSocketFD);
	
}

void *senderToReciever(void *args)
{
	// this rbuff is for receving file name from client
	char sbuff[BUFFER_SIZE], rbuff[1024];
	// int n;
	int receiverIndex = *((int *)args);
	int receiverFD = peer_fd[receiverIndex];
	cout << "receiverIndex is " << receiverIndex << endl;
	cout << "receiverFD is " << receiverFD << endl;

	string filename;
	int pieceNo;
	cout << "request recieved form downloader " << endl;
	while (1)
	{
		read(receiverFD, rbuff, 1024);

		if (strcmp(rbuff, "BYE") == 0)
		{
			cout << "recieved bye " << endl;
			break;
		}

		char *token = strtok(rbuff, " ");

		filename = string(token);
		token = strtok(NULL, " ");
		pieceNo = stoi(token);
		cout << "request received for file: " << filename << " piece :" << pieceNo << endl;

		string pieceSHA = "";

		for (auto i = shaInfoPerFile.begin(); i != shaInfoPerFile.end(); i++)
		{
			if (i->fileName.compare(filename) == 0)
			{
				pieceSHA = i->pieceWiseSHA[pieceNo];
				break;
			}
		}

		if (pieceSHA.length() == 0) //if file piece was not found
		{
			perror("file piece was not found\n");
		}

		FILE *fp = fopen(filename.c_str(), "r+");

		if (fp == NULL)
		{
			perror("file does not exist");
			pthread_exit(NULL);
		}

		printf("File opened, now sending piece : %d\n", pieceNo);

		// moving pointer of file to calculate file size
		long piece_size = 0;
		fseek(fp, pieceNo * BUFFER_SIZE, SEEK_SET);
		long piece_begin = ftell(fp);
		fseek(fp, 0, SEEK_END);
		long file_end = ftell(fp);
		if (file_end > piece_begin + BUFFER_SIZE - 1)
		{
			piece_size = BUFFER_SIZE;
		}
		else
		{
			piece_size = file_end - piece_begin;
		}
		//sending piece size dummy
		write(receiverFD, to_string(piece_size).c_str(), 32);

		//reset fp to piece beginning
		fseek(fp, pieceNo * BUFFER_SIZE, SEEK_SET);

		// sending file to client by reading in chunks
		int bytesReadFromFile;

		bytesReadFromFile = fread(sbuff, sizeof(char), BUFFER_SIZE, fp);
		send(receiverFD, sbuff, bytesReadFromFile, 0);
		bzero(sbuff, sizeof(sbuff));

		printf("File : %s ,Piece = %d , %d Bytes sent!\n", filename.c_str(), pieceNo, bytesReadFromFile);
		fclose(fp);
	}
	close(receiverFD);
}

void clientAsServer()
{
	struct sockaddr_in c_addr;
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	if ((server_c_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		printf("socket failure\n");
		exit(EXIT_FAILURE);
	}
	printf("port is %d\n", c_port);

	// attach socket to the given port
	if (setsockopt(server_c_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		printf("error in attaching to port");
		exit(EXIT_FAILURE);
	}
	c_addr.sin_family = AF_INET;
	c_addr.sin_addr.s_addr = INADDR_ANY;
	c_addr.sin_port = htons(c_port);

	//bind socket to port
	if (bind(server_c_fd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0)
	{
		printf("binding error");
		exit(EXIT_FAILURE);
	}
	if (listen(server_c_fd, 3) < 0)
	{
		printf("listening error");
		exit(EXIT_FAILURE);
	}
	int i = 0;
	while (i < 10)
	{
		int *params = (int *)malloc(sizeof(int));
		*params = i;
		if ((peer_fd[i] = accept(server_c_fd, (struct sockaddr *)&c_addr,
								 (socklen_t *)&c_addr)) < 0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		cout << "peer_fd[i] " << peer_fd[i] << endl;
		//create a thread to serve the download request from a peer for a piece of a file
		pthread_create(&thread_c[i], &attr, senderToReciever, (void *)params); //pass connfd by copy as parameter to avoid race
		i++;
	}
	i = 0;
	while (i < 10)
	{
		pthread_join(thread_c[i], NULL);
		i++;
	}
	i = 0;
}

void *sendChunk(void *args)
{
	string f_name = "a.txt";
	string sendToPeer;
	for (auto i = shaInfoPerFile.begin(); i != shaInfoPerFile.end(); i++)
	{
		struct shaInfo f = *i;
		if (f.fileName == f_name)
		{
			for (auto j = f.pieceWiseSHA.begin(); j != f.pieceWiseSHA.end(); i++)
			{
				sendToPeer = to_string(j->first) + "$" + j->second;
			}
		}
	}
}

// ./peer ip port (same port as of tracker)

int main(int argc, char const *argv[])
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("\n Socket creation error \n");
		return -1;
	}
	int port = atoi(argv[2]);
	c_port = atoi(argv[3]);
	// int *parameter = (int *)malloc(sizeof(int)); //params is pointer to i
	int *parameter = &port;
	printf("pthread created\n");
	pthread_create(&thread_main, &attr, peerToTracker, (void *)parameter); //parameter contains the port of tracker

	//make client also listen to requests
	clientAsServer();
	printf("pthread join\n");
	pthread_join(thread_main, NULL);
	return 0;
}
