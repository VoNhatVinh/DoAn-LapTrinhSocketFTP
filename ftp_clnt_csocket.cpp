﻿// ftp_clnt_csocket.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ftp_clnt_csocket.h"
#include <wchar.h>
#include <afxsock.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <fcntl.h> //_O_U16TEXT
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

//Hàm tách 1 chuỗi command thành hai chuỗi đầu là lệnh và sau là tham số
void TachChuoi(WCHAR a[], WCHAR dau[], WCHAR sau[]);
void TachChuoi(char a[], char dau[], char sau[]);

//Hàm tìm PORT của một Socket, trả về PORT
int GetPort(CSocket &a);

//Check tồn tại của chuỗi a trong vector. Nếu có trả về vị trí của nó, không có trả về -1
int CheckTonTai(vector<string> a, char x[]);

int TachChuoi(vector<string> &temp, char str[]);

string Lowercase(string a);

//Trong cái gửi lệnh này có cả cái gửi PORT và Gửi lệnh luôn.
int GuiLenh(CSocket &SDManage, CSocket &ClientSocket, char temp_truoc[], const char temp_sau[], int choose = 1);

int GuiLenhPasv(CSocket &SDTranfer, CSocket &ClientSocket, char temp_truoc[], const char temp_sau[], int choose = 1);

string Ls_Dir(CSocket & SDTranfer, CSocket &ClientSocket, int res_pasv, int choose = 1);

void Put(char buf[], CSocket &SDTranfer, CSocket &ClientSocket, const char put_file[]);

void Get(const char get_file[], char buf[], CSocket &SDTranfer, CSocket & ClientSocket, string path = "", int mode = 'a');

void Del(const char temp_sau[], CSocket& ClientSocket);

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
			vector<string>list = { "dir","ls","put","get","mput","mget","cd","lcd","delete","mdelete","mkdir","rmdir","pwd","pasv","quit" };
			//Chỉ số để chỉ ra lệnh đó là lệnh nào. Port là Port của Socket
			int chiso, port;
			//byte_length để tính độ dài byte
			int byte_length;
			//size_temp là biến lặp cho mput và mget
			int temp_size;
			//duration là thời gian chuyển
			double duration;
			//Một char command để nhập lệnh vào, temp_truoc để lưu lệnh, temp_sau để lưu phần sau lại.
			char command[40], temp_truoc[10], temp_sau[30];
			//tên file put lên server
			char put_file[35], get_file[35];
			//mput and mget
			vector<string>argv, mfile;
			string choose;//yes or y
			int ret_guilenh_pasv;
			//Check Mode
			string mode = "active";
			fgetc(stdin);
			while (true)
			{//Xử lý lệnh trong này
				byte_length = 0;
				temp_size = 0;
				printf("ftp> ");

				fgets(command, 40, stdin);
				TachChuoi(command, temp_truoc, temp_sau);
				chiso = CheckTonTai(list, temp_truoc);
				if (chiso != -1)
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
						if (mode == "active")
						{
							if (GuiLenh(SDManage, ClientSocket, temp_truoc, temp_sau) == -1)
							{
								SDManage.Close();
								continue;
							}
						}
						else
						{
							ret_guilenh_pasv = GuiLenhPasv(SDTranfer, ClientSocket, temp_truoc, temp_sau);
							if (ret_guilenh_pasv == -1)
							{
								SDTranfer.Close();
								continue;
							}
						}
						if (mode == "active")
							SDManage.Listen();
						if ((mode == "active" && SDManage.Accept(SDTranfer)) || mode == "passive")//Lưu ý mode == "active' phải check trước...
						{
							//Lệnh dir hoặc ls
							if (chiso == 1 || chiso == 0)
							{
								//Ls_Dir(SDTranfer, ClientSocket);
								tmpres = 1;
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
								if (ret_guilenh_pasv == 1 || mode == "active")
								{
									ClientSocket.Receive(buf, BUFSIZ);
									printf("%s", buf);
								}
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
					else if (chiso == 4 || chiso == 5)
					{
						sprintf(buf, "TYPE A\n\r");
						ClientSocket.Send(buf, strlen(buf));
						memset(buf, 0, BUFSIZ);
						ClientSocket.Receive(buf, BUFSIZ);
						printf("%s", buf);
						chiso == 4 ? sprintf(temp_truoc, "STOR") : sprintf(temp_truoc, "RETR");
						if (!strcmp(temp_sau, ""))
						{
							chiso == 4 ? printf("Local Files: ") : printf("Remote Files: ");
							fgets(temp_sau, 35, stdin);
						}
						TachChuoi(mfile, temp_sau);
						//Tại đây xử lý đối với trường hợp mget
						if (chiso == 4)
						{
							for (int i = 0; i < mfile.size(); i++)
							{
								FILE *f = fopen(mfile[i].c_str(), "rb");
								if (f == NULL)
								{
									cout << mfile[i] << ": File no found." << endl;
									continue;
								}
								fclose(f);
								printf("put %s? ", mfile[i].c_str());
								getline(cin, choose);
								if (choose == "y" && choose == "yes")
								{
									if (mode == "active")
									{
										if (GuiLenh(SDManage, ClientSocket, temp_truoc, strdup(mfile[i].c_str())) == -1)
										{
											SDManage.Close();
											continue;
										}
									}
									else {
										ret_guilenh_pasv = GuiLenhPasv(SDTranfer, ClientSocket, temp_truoc, temp_sau);
										if (ret_guilenh_pasv == -1)
										{
											SDTranfer.Close();
											continue;
										}
									}
									if (mode == "active")
										SDManage.Listen();
									if ((mode == "active" && SDManage.Accept(SDTranfer)) || mode == "passive")
									{
										Put(buf, SDTranfer, ClientSocket, strdup(mfile[i].c_str()));
									}
									SDTranfer.Close();
									SDManage.Close();
								}
							}

						}
						if (chiso == 5)
						{
							for (int i = 0; i < mfile.size(); i++)
							{
								if (mode == "active")
								{
									if (GuiLenh(SDManage, ClientSocket, "NLST", strdup(mfile[i].c_str()), 0) == -1)
									{
										SDManage.Close();
										continue;
									}
								}
								else {
									ret_guilenh_pasv = GuiLenhPasv(SDTranfer, ClientSocket, "NLST", temp_sau, 0);
									if (ret_guilenh_pasv == -1)
									{
										SDTranfer.Close();
										continue;
									}
								}

								if (mode == "active")
									SDManage.Listen();
								if ((mode == "active" && SDManage.Accept(SDTranfer)) || mode == "passive")
								{
									string temp_str;
									string temp = Ls_Dir(SDTranfer, ClientSocket, 1, 0);
									TachChuoi(files, strdup(temp.c_str()));
									for (int j = 0; j < files.size(); j++)
									{
										temp_str = Lowercase(files[0]) == mfile[i] ? "" : mfile[i] + "/";
										for (int k = 0; k < mfile[i].length(); k++)
										{
											if (mfile[i][k] == '*')
												temp_str = "";
										}
										files[j] = files[j].substr(0, files[j].length() - 1);
										SDManage.Close(); SDTranfer.Close();
										cout << "get " << files[j] << "? ";
										getline(cin, choose);
										if (choose == "y" || choose == "yes")
										{
											if (mode == "active")
											{
												if (GuiLenh(SDManage, ClientSocket, temp_truoc, files[j].c_str()) == -1)
												{
													SDManage.Close();
													continue;
												}
											}
											else {
												ret_guilenh_pasv = GuiLenhPasv(SDTranfer, ClientSocket, temp_truoc, files[j].c_str());
												if (ret_guilenh_pasv == -1)
												{
													SDTranfer.Close();
													continue;
												}
											}
											if (mode == "active")
												SDManage.Listen();
											if ((mode == "active" && SDManage.Accept(SDTranfer)) || mode == "passive")
												Get(files[j].c_str(), buf, SDTranfer, ClientSocket, temp_str, (mode == "active") ? 'a' : 'p');
										}
									}
									files.clear();
								}

								SDManage.Close();
							}
							mfile.clear();
						}
					}
					else //chiso > 5
					{
						if (chiso == 6) { //Thay đổi đường dẫn trên Server(cd)
							sprintf(temp_truoc, "CWD");

							if (!strcmp(temp_sau, "")) {//neu chi nhap cd -> enter
								cout << "Remote directory ";
								fgets(temp_sau, 100, stdin);
								sscanf(temp_sau, "%s", temp_sau);
							}

							//tao ket noi
							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s %s\r\n", temp_truoc, temp_sau);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);

							printf("%s", buf);
						
						}

						else if (chiso == 7)//thay doi duong dan duoi client
						{
							WCHAR infoBuf[BUFSIZ], infoBuf1[BUFSIZ], infoBuf2[BUFSIZ];
							memset(infoBuf, 0, BUFSIZ);
							fgetws(infoBuf, 100, stdin);
							TachChuoi(infoBuf, infoBuf1, infoBuf2);//infoBuf1=lcd
							
							WCHAR infoBuf4[BUFSIZ];
							//xem them tai http://www.tenouk.com/cpluscodesnippet/getsetcurrentdirectorywindowsystemdir.html
					
							//Chi nhap lcd
							if (infoBuf2 == L"")
							{
								if (!GetCurrentDirectory(BUFSIZ, infoBuf4))
									printf("File not found!\n");
								else
									wprintf(L"Local directory now: %ls\n", infoBuf4);
							}

							////nhap lcd <Duong dan>
							if (SetCurrentDirectory(infoBuf2)!=0)
								printf("File not found!\n");
							else
							{
								if (GetCurrentDirectory(BUFSIZ, infoBuf2)==0)
									printf("File not found!\n");
								else wprintf(L"Local directory now: %ls\n", infoBuf1);
							}
						}

						else if (chiso == 8)//xoa mot file tren Server 
						{
							sprintf(temp_truoc, "DELE");

							if (!strcmp(temp_sau, "")) {
								printf("Remote file ");
								fgets(temp_sau, 100, stdin);
								sscanf(temp_sau, "%s", temp_sau);//chi lay ten file dau tien
							}

							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s %s\r\n", temp_truoc, temp_sau);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);					
							printf("%s", buf);
						}

						else if (chiso == 9)//xoa nhieu file tren Server
						{
							sprintf(temp_truoc, "DELE");
							if (!strcmp(temp_sau, "")) {
								printf("Remote file ");
								fgets(temp_sau, 100, stdin);
							}
							TachChuoi(mfile, temp_sau);
							for (int i = 0; i < mfile.size(); i++) {

								cout << "delete " << mfile[i].c_str() << "?";
								getline(cin, choose);
								choose = Lowercase(choose);
								if (choose == "y" || choose == "yes") {
									memset(buf, 0, BUFSIZ);
									sprintf(buf, "%s %s\r\n", temp_truoc, mfile[i].c_str());
									ClientSocket.Send(buf, BUFSIZ);
									memset(buf, 0, BUFSIZ);
									ClientSocket.Receive(buf, BUFSIZ);
									printf("%s", buf);
								}
							}
							mfile.clear();
						}

						else if (chiso == 10)//tao thu muc tren Server
						{
							sprintf(temp_truoc, "MKD");

							if (!strcmp(temp_sau, "")) {
								cout << "Directory name ";
								fgets(temp_sau, 100, stdin);
								sscanf(temp_sau, "%s", temp_sau);
							}

							//tao ket noi
							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s %s\r\n", temp_truoc, temp_sau);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);
							printf("%s", buf);
							memset(buf, 0, BUFSIZ);
						}

						else if (chiso == 11)//xoa thu muc rong tren Server
						{
							sprintf(temp_truoc, "RMD");

							if (!strcmp(temp_sau, "")) {
								printf("Remote file ");
								fgets(temp_sau, 100, stdin);
								sscanf(temp_sau, "%s", temp_sau);//chi lay ten folder dau tien
							}

							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s %s\r\n", temp_truoc, temp_sau);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);
							printf("%s", buf);
						}

						else if (chiso == 12)//Hien thi duong dan hien tai tren server
						{
							sprintf(temp_truoc, "PWD");

							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s \r\n", temp_truoc);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);
							printf("%s", buf);
						}

						else if (chiso == 13)//chuyen sang che do passive
						{
							mode = "passive";
							cout << "Enter Passvie Mode" << endl;
						}

						if (chiso == 14)//lenh quit
						{
							memset(buf, 0, BUFSIZ);
							sprintf(buf, "%s", temp_truoc);
							ClientSocket.Send(buf, BUFSIZ);
							memset(buf, 0, BUFSIZ);
							ClientSocket.Receive(buf, BUFSIZ);
							printf("%s", buf);
							exit(1);
						}
					}
				
				}
				else //Chỉ số = -1
				{						
						if (!strcmp(temp_truoc, "acti"))
						{
							cout << "Enter Active Mode" << endl;
							mode = "active";
							continue;
						}
						cout << "Invalid Command!\n";
				}
			}

			ClientSocket.Close();
		}
	}
	else
	{
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}

void TachChuoi(WCHAR a[], WCHAR dau[], WCHAR sau[])
{
	swscanf(a, L"%ls", dau);
	int n = wcslen(dau);
	int len = wcslen(a);
	for (int i = n + 1; i <= len; i++)
	{
		sau[i - n - 1] = a[i];
	}
	sau[wcslen(a) - n - 2] = '\0';
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

int GetPort(CSocket &a)
{
	unsigned int port;
	CString address;
	a.GetSockName(address, port);
	return port;
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

int GuiLenh(CSocket &SDManage, CSocket &ClientSocket, char temp_truoc[], const char temp_sau[], int choose)
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
	if (choose == 1)
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
	if (choose == 1)
		printf("%s", buf);
	if (strstr(buf, "425") != NULL)
	{
		//replylogcode(425);
		return -1;
	}

	sscanf(buf, "%d", &codeftp);
	if (codeftp == 550 && choose != 1)
	{
		printf("%s", buf);
	}
	if (codeftp != 150)
	{
		//replylogcode(codeftp);
		return -1;
	}
}

int GuiLenhPasv(CSocket &SDTranfer, CSocket &ClientSocket, char temp_truoc[], const char temp_sau[], int choose)
{
	int x[6];
	unsigned int codeftp;
	char buf[BUFSIZ + 1];
	//Create Socket
	SDTranfer.Create();
	sprintf(buf, "PASV\r\n");
	ClientSocket.Send(buf, strlen(buf));
	memset(buf, 0, BUFSIZ);
	ClientSocket.Receive(buf, BUFSIZ);
	if (choose == 1)
		printf("%s", buf);
	sscanf(strstr(buf, "127"), "%d,%d,%d,%d,%d,%d", &x[0], &x[1], &x[2], &x[3], &x[4], &x[5]);

	//Sau đó gửi lệnh lên
	sprintf(buf, "%s %s\n\r", temp_truoc, temp_sau);
	ClientSocket.Send(buf, strlen(buf));
	//Cho Socket ket noi toi Server
	SDTranfer.Connect(_T("127.0.0.1"), x[4] * 256 + x[5]);
	memset(buf, 0, BUFSIZ);
	ClientSocket.Receive(buf, BUFSIZ, 0);
	if (choose == 1)
		printf("%s", buf);
	if (strstr(buf, "550") != NULL)
	{
		return -1;
	}
	if (strstr(buf, "226") == NULL)
		return 1;
	return 0;
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

string Ls_Dir(CSocket & SDTranfer, CSocket &ClientSocket, int res_pasv, int choose)
{
	double duration;
	string temp;
	char buf[BUFSIZ + 1];
	int tmpres = 1, byte_length = 0;
	//Cho nhận dữ liệu...
	clock_t start = clock();
	while (tmpres != 0)
	{
		memset(buf, 0, BUFSIZ);
		tmpres = SDTranfer.Receive(buf, BUFSIZ);
		buf[tmpres] = '\0';
		if (choose == 1)
			printf("%s", buf);
		temp += buf;
		byte_length += tmpres;
	}
	clock_t finish = clock();
	duration = double(finish - start) / CLOCKS_PER_SEC;
	memset(buf, 0, BUFSIZ);
	if (res_pasv != 0)
		tmpres = ClientSocket.Receive(buf, BUFSIZ);
	if (choose == 1)
	{
		printf("%s", buf);
		printf("ftp: %d bytes received in %.3fSenconds %.3fKbytes/sec.\n", byte_length, duration, (byte_length) / (duration * 1024));
	}
	return temp;
}

void Put(char buf[], CSocket &SDTranfer, CSocket &ClientSocket, const char put_file[])
{
	int check = 0;
	if (strstr(buf, "226") != NULL)
		check = 1;
	FILE *f = fopen(put_file, "rb");
	int byte_length = 0; double duration;
	clock_t start = clock();// ham bất đầu đếm thời gian thực hiện chương trình
	memset(buf, 0, BUFSIZ);
	while (fread(buf, 1, BUFSIZ, f) != NULL)
	{
		SDTranfer.Send(buf, BUFSIZ);
		byte_length += strlen(buf);
	}
	fclose(f);
	SDTranfer.Close();
	clock_t finish = clock();// ham đếm thời gian kết thúc
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	memset(buf, 0, BUFSIZ);
	if (check == 0)
	{
		ClientSocket.Receive(buf, BUFSIZ);
		printf("%s", buf);
	}
	//printf("%d bytes sent in %.2fSeconds %fKbytes/sec.\n", byte_length, duration, byte_length / (duration * 1024));
}

void Get(const char get_file[], char buf[], CSocket &SDTranfer, CSocket & ClientSocket, string path, int mode)
{
	int check = 0;
	if (strstr(buf, "226") != NULL)
		check = 1;
	int byte_length = 0, tmpres;
	char file[20];
	double duration;
	FILE *f;
	fopen_s(&f, get_file, "wb");
	if (f == NULL)
	{
		int temp = 0;
		for (int i = path.length(); i < strlen(get_file); i++)
		{
			file[temp] = get_file[i];
			temp++;
		}
		file[temp] = '\0';
	}
	fopen_s(&f, file, "wb");
	memset(buf, 0, strlen(buf));
	clock_t start = clock();
	while ((tmpres = SDTranfer.Receive(buf, BUFSIZ)) != 0)
	{
		fwrite(buf, 1, BUFSIZ, f);
		byte_length += tmpres;
	}
	fclose(f);
	SDTranfer.Close();
	clock_t end = clock();
	duration = (double)(end - start) / CLOCKS_PER_SEC;
	memset(buf, 0, BUFSIZ);
	if (check == 0 || mode == 'a')
	{
		ClientSocket.Receive(buf, BUFSIZ);
		printf("%s", buf);
	}
	//printf("%d bytes sent in %.2fSeconds %fKbytes/sec.\n", byte_length, duration, byte_length / (duration * 1024));
}

void Del(const char temp_sau[], CSocket& ClientSocket)
{
	char buf[BUFSIZ + 1], temp_truoc[5];
	memset(buf, 0, BUFSIZ);
	sprintf(temp_truoc, "DELE");

	sprintf(buf, "%s %s\r\n", temp_truoc, temp_sau);
	ClientSocket.Send(buf, BUFSIZ);
	memset(buf, 0, BUFSIZ);
	ClientSocket.Receive(buf, BUFSIZ);
	//buf[strlen(buf)] = '\0';
	printf("%s", buf);
}