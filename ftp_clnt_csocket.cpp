// ftp_clnt_csocket.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ftp_clnt_csocket.h"
#include <afxsock.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <string.h>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

void XulyChuoi(char a[], char dau[], char sau[]);

void replylogcode(int code)
{
	switch (code) {
		//Log In do thầy Bắp Bắp viết
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
		break;
		//My code go here
		//Dir vs Ls
	case 550:
		printf("Directory not found.");
	}
	printf("\n");
}


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);//Tạo Handle cho file .exe

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
				cout << "Khong the khoi tao Socket Library";
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
			int tmpres, size, status, port;
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
				//strstr là cắt chuỗi từ khi gặp cái "220" đầu tiên. Nếu ko có trả NULL
				//Quét chuỗi 220 trong buf 220 return code là "Service ready for new user." thì break nó ra vì server đã sẵn sàng để kết nối.
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
			//Run command tại đây
			char command[30], temp_dau[5], temp_sau[25];
			double duration;
			int byte_length = 0;
			CSocket SocketData; //Sóc két này để kiểm soát các socket con khác của nó để giao tiếp với Server.
			CSocket CData;//Sóc két này là để SocketData kiểm soát
			fgetc(stdin);

			while (true)
			{
				sprintf(temp_dau, "");
				sprintf(temp_sau, temp_dau);
				printf("ftp> ");
				fgets(command, 30, stdin);
				XulyChuoi(command, temp_dau, temp_sau);
				SocketData.Create();//Tạo socket ở 1 port ngẫu nhiên
				//----------------------------------------------------------------------
				//Nguồn: https://stackoverflow.com/questions/4046616/sockets-how-to-find-out-what-port-and-address-im-assigned
				struct sockaddr_in sa;
				int sa_len;
				sa_len = sizeof(sa);

				if (getsockname(SocketData, (struct sockaddr *)&sa, &sa_len) == -1) {
					perror("getsockname() failed");
					return -1;
				}
				port = (int)ntohs(sa.sin_port);
				//
				if (!strcmp(temp_dau, "dir") || !strcmp(temp_dau, "ls"))
				{
					if (!strcmp(temp_dau, "dir"))
						sprintf(temp_dau, "LIST");
					else sprintf(temp_dau, "NLST");

					clock_t start = clock();// ham bất đầu đếm thời gian thực hiện chương trình

					//Báo cho sever PORT để nó kết nối
					memset(buf, 0, strlen(buf));
					//PORT có 6 giá trị tham số
					sprintf(command, "127,0,0,1,%d,%d", port / 256, port - (int)(port / 256) * 256);
					sprintf(buf, "PORT %s\n\r", command);
					ClientSocket.Send(buf, strlen(buf));
					//Nhận lại thông báo
					memset(buf, 0, strlen(buf));
					ClientSocket.Receive(buf, BUFSIZ);
					sscanf(buf, "%d", &codeftp);
					if (codeftp != 200)
					{
						replylogcode(codeftp);
						exit(1);
					}
					printf("%s", buf);
					//Gửi lệnh lên sever
					memset(buf, 0, strlen(buf));
					sprintf(buf, "%s%s\r", temp_dau, temp_sau);
					ClientSocket.Send(buf, strlen(buf));
					//Nhận lại lệnh trả về kêu mở kết nối
					memset(buf, 0, strlen(buf));
					ClientSocket.Receive(buf, BUFSIZ);
					printf("%s", buf);
					if (strstr(buf, "425") != NULL)
					{
						SocketData.Close();
						continue;
					}
					SocketData.Listen();
					//Xong Cho socket PORT 1742 listen()
					if (SocketData.Accept(CData))
					{
						byte_length = 0;
						//Nghe được thì nhận string từ sever trả về
						memset(buf, 0, strlen(buf));
						ClientSocket.Receive(buf, BUFSIZ);
						printf("%s", buf);

						//Kết nối rồi thì lấy data thui. Data là lấy từ thằng CData ấy
						while (true)
						{
							memset(buf, 0, sizeof(buf));
							CData.Receive(buf, BUFSIZ);
							byte_length += strlen(buf);
							printf("%s", buf);
							if (buf[0] == 0)
								break;
						}
						//Nguồn: https://daynhauhoc.com/t/dem-thoi-gian-chay-chuong-trinh-trong-c-ch-hi-n-0-00/13022
						clock_t finish = clock();// ham đếm thời gian kết thúc
						duration = (double)(finish - start) / CLOCKS_PER_SEC;
					}
					printf("ftp: %d bytes received in %.2fSenconds %.2fBytes/sec.\n", byte_length, duration, (byte_length) / duration);
					CData.Close();
				}
				else if (!strcmp(temp_dau, "put"))
				{
					byte_length = 0; duration = 0;
					char in_name[100], out_name[100];
					printf("Local File: ");
					fgets(in_name, 100, stdin);
					in_name[strlen(in_name) - 1] = '\0';
					printf("Remote file: ");
					fgets(out_name, 100, stdin);
					out_name[strlen(out_name) - 1] = '\0';
					//Check FILE
					FILE *f = fopen(in_name, "rb");
					if (f == NULL)
					{
						cout << in_name << ": File not found" << endl;
						SocketData.Close();
						continue;
					}
					//PORT có 6 giá trị tham số
					memset(buf, 0, strlen(buf));
					sprintf(command, "127,0,0,1,%d,%d", port / 256, port - (int)(port / 256) * 256);
					sprintf(buf, "PORT %s\n\r", command);
					ClientSocket.Send(buf, strlen(buf));
					memset(buf, 0, strlen(buf));
					ClientSocket.Receive(buf, BUFSIZ);
					printf("%s", buf);
					memset(buf, 0, strlen(buf));
					sprintf(buf, "STOR %s\n\r", out_name);
					ClientSocket.Send(buf, strlen(buf));
					memset(buf, 0, strlen(buf));
					ClientSocket.Receive(buf, BUFSIZ);
					printf("%s", buf);
					if (strstr(buf, "425") != NULL)
					{
						SocketData.Close();
						continue;
					}
					SocketData.Listen();
					if (SocketData.Accept(CData))
					{
						memset(buf, 0, strlen(buf));
						clock_t start = clock();// ham bất đầu đếm thời gian thực hiện chương trình
						while (fread(buf, 1, 1, f) != NULL)
						{
							CData.Send(buf, 1);
							byte_length += 1;
						}
						fclose(f);
						CData.Close();
						clock_t finish = clock();// ham đếm thời gian kết thúc
						duration = (double)(finish - start) / CLOCKS_PER_SEC;
						memset(buf, 0, strlen(buf));
						ClientSocket.Receive(buf, BUFSIZ);
						printf("%s", buf);
						printf("%d bytes sent in %.2fSeconds %fBytes/sec.\n", byte_length, duration, byte_length / duration);
					}
				}
				else
				{
					cout << "Invalid Command" << endl;
				}
				SocketData.Close();
			}
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


void XulyChuoi(char a[], char dau[], char sau[])
{
	sscanf(a, "%s", dau);
	int n = strlen(dau);
	for (int i = n; i <= strlen(a); i++)
	{
		sau[i - n] = a[i];
	}
}