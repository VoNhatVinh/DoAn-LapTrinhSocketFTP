#pragma once
#include <afxsock.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <Windows.h>
#include "resource.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

//Hàm tách 1 chuỗi command thành hai chuỗi đầu là lệnh và sau là tham số
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

void Put(char buf[], CSocket &SDTranfer, CSocket &ClientSocket, const char put_file[], int mode = 'a');

void Get(const char get_file[], char buf[], CSocket &SDTranfer, CSocket & ClientSocket, string path = "", int mode = 'a');

void Del(const char temp_sau[], CSocket& ClientSocket);

void replylogcode(int code);