#include<iostream>
#include<stdio.h>
#include<string.h>

#include<sys/types.h>
#include<sys/stat.h>
//网络通信需要包含的头文件和需要加载的库文件
#include<winsock2.h>
#pragma comment(lib, "ws2_32.lib")

//#define PRINTF(str) printf("[%s - %d]"#str"%s\n", __func__, __line__, str);
#define PRINTF(str) std::cout << "[" << __func__ << " - " << __LINE__ << "]" << str;
using namespace std;

void error_die(const char* str)
{
	perror(str);
	exit(1);
}

//实现网络初始化
//返回值：套接字（服务器端套接字）
//端口
//参数：port表示端口
//		如果*port是0，那么自动分配一个可用的端口
//
int startup(unsigned short* port)
{
	//网络环境初始化
	WSADATA data;
	int ret = WSAStartup(  //wsastartup函数用于初始化网络环境，初始化为0
		MAKEWORD(1, 1),   //1.1协议版本
		&data);
	if (ret) {  //ret不为0，初始化失败
		error_die("wsastartup");
	}

	//创建套接字server_socket
	int server_socket = socket(PF_INET,  //套接字类型：网络套接字，地址类型ipv4
		SOCK_STREAM,  //流格式套接字
		IPPROTO_TCP);  //tcp协议
	if (server_socket == -1) {
		// 打印错误提示，结束程序
		error_die("套接字");
	}

	//设置端口可复用，防止端口假死
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");  //检测端口可复用是否设置成功
	}

	//配置服务器端的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);  //设置端口号，htons进行数据转换（本机转网络）
	server_addr.sin_addr.s_addr = INADDR_ANY;  //设置ip地址为任意访问，值为0

	//绑定套接字
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0) {
		error_die("bind");
	}

	//动态分配一个端口
	int namelen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &namelen) < 0) {
			error_die("getsockname");
		}
		*port = server_addr.sin_port;
	}

	//创建监听队列
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}

	return server_socket;
}

//从指定的客户端套接字sock，读取一行数据，保存到buff中
//返回实际读取到的字节数
int get_line(int sock, char *buff, int size)
{
	char c = 0;  // '\0'
	int i = 0;

	//通过while循环，用recv函数每次接收一个字符，存进buff中，直到遇到换行符
	while (i < size - 1 && c != '\n') {
		//recv函数：第一个参数sock是一个有效的套接字描述符，第二个参数&c是一个指向接收数据的
		//缓冲区的指针，第三个参数1表示要接收的字节数，第四个参数0表示不使用任何特殊的接收选项。
		int n = recv(sock, &c, 1, 0);  //函数返回值n表示实际接收的字节数，可以用来检测是否成功接收数据。
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK);  //用于接收数据时进行查看而不是实际取走数据
				if (n > 0 && c == '\n') {
					recv(sock, &c, 1, 0);
				}
				else {
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else {
			c = '\n';
		}
	}
	buff[i] = 0;
	return i;
}

void unimplement(int client) {
	//向指定套接字，发送一个提示还没有实现的错误页面
}

void not_found(int client) {
	//发送404响应
	char buff[1024];

	strcpy(buff, "http/1.0 404 not found\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "server: rockhttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "content-type:text/html\n");  //文本格式
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	//发送404网页内容
	sprintf(buff,
		"<html>                                       \
			<title>not found</title>                  \
			<body>                                    \
			<h2> the resource is unavaliable.</h2>    \
			<img src = \"404.png\">                   \
			</body>                                   \
		</html>");
	send(client, buff, strlen(buff), 0);
}

void headers(int client) {
	//发送响应包头文件信息
	char buff[1024];

	strcpy(buff, "http/1.0 200 ok\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "server: rockhttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "content-type:text/html\n");  //文本格式
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
}

void cat(int client, FILE* resource) {
	char buff[4096];
	int count = 0;

	while (1) {
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0) {
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	cout << "一共发送" << count << "字节给浏览器\n";
}

//发送资源给客户端
void server_file(int client, const char* filename) {
	//to do. 
	int numchars = 1;
	char buff[1024];

	//把请求数据包的剩余数据行读完
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	//file* resource = fopen(filename, "r");
	FILE* resource = NULL;
	if (strcmp(filename, "htdocs/index.html") == 0) {  //发送文件资源给客户端
		resource = fopen(filename, "r");
	}
	else {
		resource = fopen(filename, "rb");
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		//正式发送资源给浏览器
		headers(client);

		//发送请求的资源信息
		cat(client, resource);

		PRINTF("资源发送完毕!\n");
	}

	fclose(resource);
}

//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg)
{
	char buff[1024];  //1k
	int client = (SOCKET)arg;  //客户端套接字

	//读取一行数据
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);

	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);

	//检查请求的方法，本服务器是否支持
	if (stricmp(method, "get") && stricmp(method, "post")) {
		//像浏览器返回一个错误提示页面
		unimplement(client);
		return 0;
	}

	//解析资源文件的路径
	//"get / http/1.1\n"
	char url[255];  //存放请求的资源的完整路径
	i = 0;
	//跳过资源路径前面的空格
	while (isspace(buff[j]) && j < sizeof(buff)) j++;

	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);

	//
	char path[512] = "";
	sprintf(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}
	PRINTF(path);

	struct stat status;
	if (stat(path, &status) == -1) {
		//请求包的剩余数据读取完毕
		while (numchars > 0 && strcmp(buff, "\n")) {
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);  //向浏览器发送一个404页面
	}
	else {
		if ((status.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		server_file(client, path);
	}

	closesocket(client);
	return 0;
}

int main()
{
	unsigned short port = 80;  //配置端口号
	int server_sock = startup(&port);
	cout << "http服务器已启动，正在监听" << port << "端口\n" << endl;

	struct sockaddr_in client_addr;  //表示ipv4网络地址的数据结构
	int client_addr_len = sizeof(client_addr);  //提供地址长度

	//to do
	while (1)
	{
		/*
		阻塞式等待用户通过浏览器发起访问用户套接字，与用户进行交互等待客户端
		或浏览器发起访问很多时候接口接收地址时，还需要一个地址的长度
		client_sock返回一个新的套接字，与服务连接，与server_sock套接字进行一对一服务
		*/

		int client_sock = accept(server_sock,  //接收套接字，等待用户发起访问
			(struct sockaddr*)&client_addr,  //记录用户从哪进行访问的
			&client_addr_len);  //地址长度
		if (client_sock == -1) {
			error_die("accept");
		}

		//创建新线程
		DWORD threadid = 0;
		CreateThread(0, 0,
			accept_request,  //处理用户请求的线程函数
			(void*)client_sock,  //创建的套接字对用户进行服务，该参数传递给线程函数
			0, &threadid);

	}

	closesocket(server_sock);  //关闭套接字
	return 0;
}