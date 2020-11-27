#include "../include/header.h"
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;

// #include <netinet/in.h>

// struct sockaddr_in {
//     short            sin_family;   // e.g. AF_INET
//     unsigned short   sin_port;     // e.g. htons(3490)
//     struct in_addr   sin_addr;     // see struct in_addr, below
//     char             sin_zero[8];  // zero this if you want to
// };

// struct in_addr {
//     unsigned long s_addr;  // load with inet_aton()
// };

int valread[5], new_socket[5];
int server_fd;
int opt = 1;
struct sockaddr_in address; // <netinet/in.h>
int addrlen = sizeof(address);
char recvFromClientbuffer[1024] = {0};
// char sendToClientbuffer[1024] = {0};
unordered_map<string, string> idPass;
char sendtoPeerBuf[1024]; //to send info to client, which client uses in readfromtracker func
pthread_t threadc[5];

unordered_map<int, string> sidUser;
unordered_map<string, vector<string>> uidgid;
unordered_map<string, string> grpOwner;
unordered_map<string, vector<string>> grpmembers;
unordered_map<string, vector<string>> requests;
unordered_map<string, string> uidPort;
unordered_map<string, string> filename_Chunks;
vector <string> downloadedFiles;

//****client user mapping*** socket id, uid (dont save alag ho skti)
//group owner mapping
//group user(member) mapping

string findName(string path)
{
    vector<string> t;
    stringstream check1(path);
    string intermediate;
    while (getline(check1, intermediate, '/'))
    {
        t.push_back(intermediate);
    }
    return t[t.size() - 1];
}

// vector <struct group> groups;

struct fileInfoPerGroup
{
    string gid;
    unordered_map<string, vector<string>> activePeers; //<filename,Uid>
};
vector<struct fileInfoPerGroup> files;

string accept_req(string gid, string uid)
{
    grpmembers[gid].push_back(uid);
}

void *readFromClient(void *params)
{
    while (1)
    {
        int *p = (int *)params;
        int i = *p;
        printf("i is current peer number %d\n", i);
        read(new_socket[i], recvFromClientbuffer, 1024); //blocking call
        printf("data from client is: %s\n", recvFromClientbuffer);
        vector<string> token;
        stringstream check1(recvFromClientbuffer);
        string intermediate;
        while (getline(check1, intermediate, ' '))
        {
            token.push_back(intermediate);
        }
        if (token[0] == "create_user") //user ko persistan rakhana hai
        {
            cout << "user created \n";
            //add user to map
            idPass[token[1]] = token[2];
            for (auto m = idPass.begin(); m != idPass.end(); m++)
            {
                cout << m->first << " " << m->second << endl;
            }
            strcpy(sendtoPeerBuf, USER_CREATED);
            write(new_socket[i], sendtoPeerBuf, 1024);
            // user << token[1] << " " << token[2];
        }
        else if (token[0] == "login")
        {
            cout << "in login :" << endl;
            string id = token[1];
            string pass = token[2];
            string c_port = token[3];

            cout << "id is " << id << endl;
            cout << "pass is " << pass << endl;
            cout << "port is " << c_port << endl;

            if (idPass.find(id) == idPass.end())
            {
                cout << "no such user exists\n";
            }
            else
            {
                cout << "pass saved is " << idPass[id];
                if (idPass[id].compare(pass) == 0)
                {
                    cout << "login successfull for user: " << id << endl;
                    sidUser[new_socket[i]] = id;
                    uidPort[id] = c_port;

                    strcpy(sendtoPeerBuf, LOGIN_SUCCESS);
                    cout << sendtoPeerBuf << "\n";
                    write(new_socket[i], sendtoPeerBuf, 1024);
                    // uidPort << id << " " << c_port;
                }
                else
                {
                    cout << "sorry! The password is wrong\n";
                    strcpy(sendtoPeerBuf, LOGIN_INVALID);
                    cout << sendtoPeerBuf << "\n";
                    write(new_socket[i], sendtoPeerBuf, 1024);
                }
            }
        }
        else if (token[0] == "create_group")
        {
            cout << "creating group...on the way..\n";
            string gid = token[1];
            string uid = sidUser[new_socket[i]];
            grpOwner[gid] = uid; //uid is owner of group with id as gid
            cout << "voila!! group created!\n";
            strcpy(sendtoPeerBuf, GROUP_CREATED);
            write(new_socket[i], sendtoPeerBuf, 1024);
        }
        else if (token[0] == "join_group")
        {
            string gid = token[1];
            string uid = sidUser[new_socket[i]];
            requests[gid].push_back(uid);
            strcpy(sendtoPeerBuf, GROUP_JOINED);
            write(new_socket[i], sendtoPeerBuf, 1024);
        }

        else if (token[0] == "leave_group")
        {
            //first check if member of grp

            string grp_id = token[1];
            string u_id = sidUser[new_socket[i]];
            //if uid is owner, delete grp
            //else
            auto it = find(grpmembers[grp_id].begin(), grpmembers[grp_id].end(), u_id);
            grpmembers[grp_id].erase(it);
            strcpy(sendtoPeerBuf, GROUP_LEFT);
            write(new_socket[i], sendtoPeerBuf, 1024);
        }

        else if (token[0] == "requests")
        {
            string gid = token[2];
            vector<string> reqList;
            reqList = requests[gid];
            string req;
            cout << "Pending requests for group " << gid << " are:" << endl;
            for (auto i = reqList.begin(); i != reqList.end(); i++)
            {
                cout << *i << endl;
                req = req + *i;
            }
            strcpy(sendtoPeerBuf, "P ");
            strcpy(sendtoPeerBuf, req.c_str());
            // sendtoPeerBuf = sendtoPeerBuf + " "+ req;
            cout << "send buff" << sendtoPeerBuf << endl;
            write(new_socket[i], sendtoPeerBuf, 1024);
            
        }
        else if (token[0] == "accept_request")
        {
            //pop from reqyests vali list and push to members list
            string grp_id = token[1];
            string u_id = token[2];
            //find the uid in requets list
            auto it = find(requests[grp_id].begin(), requests[grp_id].end(), u_id);
            requests[grp_id].erase(it);
            grpmembers[grp_id].push_back(u_id);
            strcpy(sendtoPeerBuf, REQ_ACCEPT);
            write(new_socket[i], sendtoPeerBuf, 1024);
        }
        else if (token[0] == "list_groups")
        {
            string grp = "g ";
            for (auto i = grpOwner.begin(); i != grpOwner.end(); i++)
            {
                cout << i->first << endl;
                grp = grp + " " + i->first;
            }
            cout<<grp<<endl;
            strcpy(sendtoPeerBuf, grp.c_str());
            write(new_socket[i], sendtoPeerBuf, 1024);
        }
        else if (token[0] == "list_files")
        {
            string file = "f ";
            string grp_id = token[1];
            for (auto i = files.begin(); i != files.end(); i++)
            {
                if (i->gid == grp_id)
                {
                    for (auto j = (i->activePeers).begin(); j != (i->activePeers).end(); j++)
                    {
                        cout << j->first << endl;
                        file = file + " " + j->first;
                    }
                }
            }
            cout<<file<<endl;
            strcpy(sendtoPeerBuf, file.c_str());
            write(new_socket[i], sendtoPeerBuf, 1024);
        }
        else if (token[0] == "upload_file")
        {
            // <path> <gid>
            //path se filename nikalo
            //push in fileinfo
            string filename = findName(token[1]);
            cout << "filename is " << filename << endl;
            struct fileInfoPerGroup f;
            f.gid = token[2];
            string uid = sidUser[new_socket[i]];
            f.activePeers[filename].push_back(uid);
            files.push_back(f);

            filename_Chunks[filename] = token[token.size() - 1];
            // fileinfo << filename << " " << uid;
            strcpy(sendtoPeerBuf, FILE_UPLOAD_SUCCESSFUL);
            write(new_socket[i], sendtoPeerBuf, 1024);
            cout << "upload success" << endl;
        }
        else if (token[0] == "download_file")
        {
            string grpid = token[1];
            string fName = token[2]; //corresponding to this filename, i must get the number of chunks
            //check if gid mentioned is his own grp
            //assuming he does:
            //insert file info in the vector
            // struct fileInfoPerGroup f;
            string uid = sidUser[new_socket[i]];
            downloadedFiles.push_back(fName);

            // f.activePeers[fName].push_back(uid);
            // files.push_back(f);
            // fileinfo << fName << " " << uid;
            // string u_id = sidUser[new_socket[i]];
            // string clientPort = uidPort[uid];
            vector<string> peerUIDs;
            string portList = "D";

            for (auto i = files.begin(); i != files.end(); i++)
            {
                if (i->gid == grpid)
                {
                    for (auto j = (i->activePeers).begin(); j != (i->activePeers).end(); j++)
                    {
                        // cout << j->first << endl;
                        if (j->first == fName)
                        {
                            peerUIDs = j->second;
                        }
                    }
                }
            }
            for (auto i = peerUIDs.begin(); i != peerUIDs.end(); i++)
            {
                portList = portList + " " + uidPort[*i];
            }
            cout << "port list is " << portList << "\n";
            portList = portList + " " + fName;
            portList = portList + " " + filename_Chunks[fName];
            cout << portList << endl;

            //SEND LIST OF C_PORTS WHICH HAVE THE FILE
            strcpy(sendtoPeerBuf, portList.c_str());
            cout << sendtoPeerBuf << "\n";
            write(new_socket[i], sendtoPeerBuf, 1024);
        }
        else if (token[0] == "logout")
        {
            string uid = token[1];
            idPass.erase(uid);
        }
        else if (token[0] == "show_downloads")
        {
            for(auto i=downloadedFiles.begin(); i!=downloadedFiles.end(); i++)
            {
                cout<<*i<<" ";
            }
        }
        else if (token[0] == "stop_share")
        {
            // cout<<""
        }

        // // Printing the token vector
        // for(int i = 0; i < token.size(); i++)
        //     cout << token[i] << '\n';
        // strcpy(sendBuff, "Wrong Command\n");
        // write(connfd[i], sendBuff, SIZE);

        // memset(sendtoPeerBuf, '\0', SIZE);
        // memset(readBuff, '\0', SIZE);
        memset(sendtoPeerBuf, '\0', 1024);
        memset(recvFromClientbuffer, '\0', 1024);
    }
}

int main(int argc, char const *argv[])
{
    
    idPass["u1"] = "p1";
    idPass["u2"] = "p2";
    grpOwner["g1"] = "u1";
    grpmembers["g1"].push_back("u1");
    grpmembers["g1"].push_back("u2");
    string hello = "hello from server";
    //create the socket fd
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {

        printf("socket failure\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    printf("port is %d\n", port);

    // attach socket to the given port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("error in attaching to port");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    //bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("binding error");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        printf("listening error");
        exit(EXIT_FAILURE);
    }
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int i = 0;

    while (i < 5)
    {
        int *parameter = (int *)malloc(sizeof(int)); //params is pointer to i
        parameter[0] = i;
        if ((new_socket[i] = accept(server_fd, (struct sockaddr *)&address,
                                    (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        pthread_create(&threadc[i], &attr, readFromClient, (void *)parameter);

        // printf("%s\n", buffer);
        // send(new_socket[i], hello.c_str(), strlen(hello.c_str()), 0);
        // printf("Hello message sent\n");
        i++;
    }
    i = 0;
    while (i < 5)
    {
        pthread_join(threadc[i], NULL);
        i++;
    }
    i = 0;
    while (i < 5)
    {
        pthread_join(threadc[i], NULL);
        i++;
    }
    return 0;
}