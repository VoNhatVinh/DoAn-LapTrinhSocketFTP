// ftp_clnt_csocket.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ftp_clnt_csocket.h"
#include <afxsock.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

//Hàm tách 1 chuỗi command thành hai chuỗi đầu là lệnh và sau là tham số
void TachChuoi(char a[], char dau[], char sau[]);

//Hàm tìm PORT của một Socket, trả về PORT
int GetPort(const CSocket &a);

//Check tồn tại của chuỗi a trong vector. Nếu có trả về vị trí của nó, không có trả về -1
int CheckTonTai(vector<string> a, char x[]);

int TachChuoi(vector<string> &temp, char str[]);

string Lowercase(string a);

//Trong cái gửi lệnh này có cả cái gửi PORT và Gửi lệnh luôn.
int GuiLenh(CSocket &SDManage, CSocket &ClientSocket, char temp_truoc[], char temp_sau[]);

void Put(char buf[], CSocket &SDTranfer, CSocket &ClientSocket, char put_file[]);

void Get(char get_file[], char buf[], CSocket &SDTranfer, CSocket & ClientSocket);



void replylogcode(int code)
{
	switch (code) {
	case 200:
		printf("Command okay");
		break;
	case 500:
		printf("Syntax error, command unrecognized.");
		printf("This may include errors such as command line too long.");
		break;
	case 501:
		printf("Syntax error in parameters or arguments.");
		break;
	case 202:
		printf("Command not implemented, superfluous at this site.");
		break;
	case 502:
		printf("Command not implemented.");
		break;
	case 503:
		printf("Bad sequence of commands.");
		break;
	case 530:
		printf("Not logged in.");

	case 550:
		printf("File not found.");
		break;
	case 425:
		printf("Can't open data connection for transfer");
		break;
	}
	printf("\n");
}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
			// TODO: code your application's behavior here.
			// Khoi tao thu vien Socket
			if (AfxSocketInit() == FALSE)
			{
				cout << "Khong the khoi tao Socket Libraray";
				return FALSE;
			}
			// Tao socket dau tien
			CSocket ClientSocket;
			ClientSocket.Create();

			// Ket noi den Server
			if (ClientSocket.Connect(_T("127.0.0.1"), 21) != 0)
			{
				cout << "Ket noi toi Server thanh cong !!!" << endl << endl;
			}
			else
				return FALSE;

			char buf[BUFSIZ + 1];
			int tmpres, size, status;
			/*
			Connection Establishment
			   120
				  220
			   220
			   421
			Login
			   USER
				  230
				  530
				  500, 501, 421
				  331, 332
			   PASS
				  230
				  202
				  530
				  500, 501, 503, 421
				  332
			*/
			char * str;
			int codeftp;
			printf("Connection established, waiting for welcome message...\n");
			//How to know the end of welcome message: http://stackoverflow.com/questions/13082538/how-to-know-the-end-of-ftp-welcome-message
			memset(buf, 0, sizeof buf);
			while ((tmpres = ClientSocket.Receive(buf, BUFSIZ, 0)) > 0) {
				sscanf(buf, "%d", &codeftp);
				printf("%s", buf);
				if (codeftp != 220) //120, 240, 421: something wrong
				{
					replylogcode(codeftp);
					exit(1);
				}

				str = strstr(buf, "220");//Why ???
				//Tìm coi trong buf có code 220 hay không.
				if (str != NULL) {
					break;
				}
				memset(buf, 0, tmpres);
			}
			//Send Username
			char info[50];
			printf("Name (%s): ", "127.0.0.1");
			memset(buf, 0, sizeof buf);
			scanf("%s", info);

			sprintf(buf, "USER %s\r\n", info);
			tmpres = ClientSocket.Send(buf, strlen(buf), 0);

			memset(buf, 0, sizeof buf);
			tmpres = ClientSocket.Receive(buf, BUFSIZ, 0);

			sscanf(buf, "%d", &codeftp);
			if (codeftp != 331)
			{
				replylogcode(codeftp);
				exit(1);
			}
			printf("%s", buf);

			//Send Password
			memset(info, 0, sizeof info);
			printf("Password: ");
			memset(buf, 0, sizeof buf);
			scanf("%s", info);

			sprintf(buf, "PASS %s\r\n", info);
			tmpres = ClientSocket.Send(buf, strlen(buf), 0);

			memset(buf, 0, sizeof buf);
			tmpres = ClientSocket.Receive(buf, BUFSIZ, 0);

			sscanf(buf, "%d", &codeftp);
			if (codeftp != 230)
			{
				replylogcode(codeftp);
				exit(1);
			}
			printf("%s", buf);

			CSocket SDManage, SDTranfer;
			//List kiểm soát lệnh
			vector<string>list = { "dir","ls","put","get","mput","mget","cd","lcd","del","mdel","mkdir","rmdir","pwd","pasv","quit" };
			//Chỉ số để chỉ ra lệnh đó là lệnh nào. Port là Port của Socket
			int chiso, port;
			//byte_length để tính độ dài byte
			int byte_length = 0;
			//size_temp là biến lặp cho mput và mget
			int temp_size;
			//duration là thời gian chuyển
			double duration;
			//Một char command để nhập lệnh vào, temp_truoc để lưu lệnh, temp_sau để lưu phần sau lại.
			char command[40], temp_truoc[5], temp_sau[35];
			//tên file put lên server
			char put_file[35], get_file[35];
			//mput and mget
			vector<string>argv, mfile;
			fgetc(stdin);
			while (true)
			{//Xử lý lệnh trong này
				temp_size = 0;
				printf("ftp> ");
				fgets(command, 40, stdin);
				TachChuoi(command, temp_truoc, temp_sau);
				if ((chiso = CheckTonTai(list, temp_truoc)) != -1)
				{
					if (chiso < 4)
					{
						if (chiso == 0)
							sprintf(temp_truoc, "LIST");
						if (chiso == 1)
							sprintf(temp_truoc, "NLST");
						if (chiso == 2)
						{
							sprintf(temp_truoc, "STOR");
							if (!strcmp(temp_sau, ""))
							{
								printf("Local File: ");
								fgets(put_file, 100, stdin);
								if (chiso == 2)
								{
									sscanf(put_file, "%s", put_file);
									printf("Remote file: ");
									fgets(temp_sau, 100, stdin);
									sscanf(temp_sau, "%s", temp_sau);
								}
							}
							else
							{
								TachChuoi(argv, temp_sau);
								strcpy(put_file, argv[0].c_str());
								//Nếu có hơn hai chuỗi ở sau thì 1 cái đầu ở sau sẽ là tên file up lên
								//Nếu không thì lấy chuỗi đầu là tên file up lên
								if (argv.size() >= 2)
									strcpy(temp_sau, argv[1].c_str());
								else
									strcpy(temp_sau, argv[0].c_str());
								argv.clear();
							}
							FILE *f = fopen(put_file, "rb");
							if (f == NULL)
							{
								cout << put_file << ": File not found" << endl;
								SDManage.Close();
								continue;
							}
						}
						if (chiso == 3)
						{
							sprintf(temp_truoc, "RETR");
							if (!strcmp(temp_sau, ""))
							{
								printf("Remote file: ");
								fgets(temp_sau, 100, stdin);
								sscanf(temp_sau, "%s", temp_sau);
								printf("Local File: ");
								fgets(get_file, 100, stdin);
								sscanf(get_file, "%s", get_file);
							}
							else {
								TachChuoi(argv, temp_sau);
								strcpy(temp_sau, argv[0].c_str());
								//Nếu có hơn hai chuỗi ở sau thì 1 cái đầu ở sau sẽ là tên file tải về
								//Nếu không thì lấy chuỗi đầu là tên file tải về
								if (argv.size() >= 2)
								{
									strcpy(get_file, argv[1].c_str());
								}
								else
								{
									strcpy(get_file, argv[0].c_str());
								}
								argv.clear();
							}
						}

						if (GuiLenh(SDManage, ClientSocket, temp_truoc, temp_sau) == -1)
						{
							SDManage.Close();
							continue;
						}

						SDManage.Listen();
						if (SDManage.Accept(SDTranfer))
						{
							//Lệnh dir hoặc ls
							if (chiso == 1 || chiso == 0)
							{
								double duration;
								//Cho nhận dữ liệu...
								clock_t start = clock();
								while (tmpres != 0)
								{
									memset(buf, 0, BUFSIZ);
									tmpres = SDTranfer.Receive(buf, BUFSIZ);
									printf("%s", buf);
									byte_length += tmpres;
								}
								clock_t finish = clock();
								duration = double(finish - start) / CLOCKS_PER_SEC;
								memset(buf, 0, BUFSIZ);
								tmpres = ClientSocket.Receive(buf, BUFSIZ);
								printf("%s", buf);
								printf("ftp: %d bytes received in %.3fSenconds %.3fKbytes/sec.\n", byte_length, duration, (byte_length) / (duration * 1024));
							}
							//Lệnh put
							else if (chiso == 2)
							{
								Put(buf, SDTranfer, ClientSocket, put_file);
							}
							//Lệnh get
							else if (chiso == 3)
							{
								Get(get_file, buf, SDTranfer, ClientSocket);
							}
						}
						SDTranfer.Close();
						SDManage.Close();
					}
					else if (chiso == 4 || chiso == 5)//Truyền nhận dữ liệu nhiều lần mget và mput
					{
						string choose;
						//Cắt command + xử lý nhiều file.
						chiso == 4 ? sprintf(temp_truoc, "STOR") : sprintf(temp_truoc, "RETR");
						if (!strcmp(temp_sau, ""))
						{
							chiso == 4 ? printf("Local Files: ") : printf("Remote Files: ");
							fgets(temp_sau, 35, stdin);
						}
						TachChuoi(mfile, temp_sau);
						for (int i = 0; i < mfile.size(); i++)
						{
							if (chiso == 4)
							{
								FILE *f = fopen(mfile[i].c_str(), "rb");
								if (f == NULL)
								{
									cout << mfile[i] << ": File no found." << endl;
									continue;
								}
								fclose(f);
								printf("put %s? ", mfile[i].c_str());
							}
							else
								printf("get %s? ", mfile[i].c_str());
							getline(cin, choose);
							//choose = Lowercase(choose);
							if (choose != "y" && choose != "yes")
								continue;

							if (GuiLenh(SDManage, ClientSocket, temp_truoc, strdup(mfile[i].c_str())) == -1)
							{
								SDManage.Close();
								continue;
							}
							SDManage.Listen();
							if (SDManage.Accept(SDTranfer))
							{
								if (chiso == 4)
									Put(buf, SDTranfer, ClientSocket, strdup(mfile[i].c_str()));
								else Get(strdup(mfile[i].c_str()), buf, SDTranfer, ClientSocket);
							}
							SDTranfer.Close();
							SDManage.Close();
						}
						mfile.clear();
					}
					else
					{

					}
				}
				else
				{
					cout << "Invalid Command!\n";
				}
			}

			ClientSocket.Close();
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}

void TachChuoi(char a[], char dau[], char sau[])
{
	sscanf(a, "%s", dau);
	int n = strlen(dau);
	for (int i = n + 1; i <= strlen(a); i++)
	{
		sau[i - n - 1] = a[i];
	}
	sau[strlen(a) - n - 2] = '\0';
}

int GetPort(const CSocket &a)
{//Nguồn: https://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
	struct sockaddr_in sa;
	int sa_len;
	sa_len = sizeof(sa);
	if (getsockname(a, (struct sockaddr *)&sa, &sa_len) == -1) {
		perror("getsockname() failed");
		return -1;
	}
	return (int)ntohs(sa.sin_port);
}

int CheckTonTai(vector<string> a, char x[])
{
	int n = a.size();
	for (int i = 0; i < n; i++)
	{
		if (a[i] == x)
		{
			return i;
		}
	}
	return -1;
}

int TachChuoi(vector<string> &temp, char str[])
{
	int count = 0;
	char* p = strtok(str, " \n");
	while (p != NULL)
	{
		temp.push_back(p);
		p = strtok(NULL, " \n"); //cat chuoi tu vi tri dung lai truoc do
		count++;
	}
	return count;
}

int GuiLenh(CSocket &SDManage, CSocket &ClientSocket, char temp_truoc[], char temp_sau[])
{
	int port, codeftp;
	char buf[BUFSIZ + 1];
	//Create Socket
	SDManage.Create();
	port = GetPort(SDManage);
	//Gửi lệnh PORT lên Server
	sprintf(buf, "PORT 127,0,0,1,%d,%d\n\r", port / 256, port % 256);
	ClientSocket.Send(buf, strlen(buf));
	memset(buf, 0, BUFSIZ);
	ClientSocket.Receive(buf, BUFSIZ);
	printf("%s", buf);
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 200)
	{
		replylogcode(codeftp);
		exit(1);
	}
	//Sau đó gửi lệnh lên
	sprintf(buf, "%s %s\r", temp_truoc, temp_sau);
	ClientSocket.Send(buf, strlen(buf));
	memset(buf, 0, BUFSIZ);
	ClientSocket.Receive(buf, BUFSIZ);
	printf("%s", buf);
	if (strstr(buf, "425") != NULL)
	{
		replylogcode(425);
		return -1;
	}
	sscanf(buf, "%d", &codeftp);
	if (codeftp != 150)
	{
		//replylogcode(codeftp);
		return -1;
	}
}

string Lowercase(string a)
{
	string b;
	for (int i = 0; i < a.length(); i++)
	{
		b.push_back(a[i]);
		if (b[i] >= 'A' && b[i] <= 'Z')
		{
			b[i] += 'a' - 'A';
		}
	}
	return b;
}

void Put(char buf[], CSocket &SDTranfer, CSocket &ClientSocket, char put_file[])
{
	FILE *f = fopen(put_file, "rb");
	int byte_length = 0; double duration;
	clock_t start = clock();// ham bất đầu đếm thời gian thực hiện chương trình
	memset(buf, 0, BUFSIZ);
	while (fread(buf, 1, BUFSIZ, f) != NULL)
	{
		SDTranfer.Send(buf, strlen(buf));
		byte_length += strlen(buf);
	}
	fclose(f);
	SDTranfer.Close();
	clock_t finish = clock();// ham đếm thời gian kết thúc
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	memset(buf, 0, strlen(buf));
	ClientSocket.Receive(buf, BUFSIZ);
	printf("%s", buf);
	printf("%d bytes sent in %.2fSeconds %fKbytes/sec.\n", byte_length, duration, byte_length / (duration * 1024));
}

void Get(char get_file[], char buf[], CSocket &SDTranfer, CSocket & ClientSocket)
{
	int byte_length = 0, tmpres;
	double duration;
	FILE *f = fopen(get_file, "wb");
	memset(buf, 0, strlen(buf));
	clock_t start = clock();
	while ((tmpres = SDTranfer.Receive(buf, BUFSIZ)) != 0)
	{
		fwrite(buf, 1, strlen(buf), f);
		byte_length += tmpres;
	}
	fclose(f);
	SDTranfer.Close();
	clock_t end = clock();
	duration = (double)(end - start) / CLOCKS_PER_SEC;
	memset(buf, 0, strlen(buf));
	ClientSocket.Receive(buf, BUFSIZ);
	printf("%s", buf);
	printf("%d bytes sent in %.2fSeconds %fKbytes/sec.\n", byte_length, duration, byte_length / (duration * 1024));
}

