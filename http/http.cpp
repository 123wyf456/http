#include<iostream>
#include<stdio.h>
#include<string.h>

#include<sys/types.h>
#include<sys/stat.h>
//����ͨ����Ҫ������ͷ�ļ�����Ҫ���صĿ��ļ�
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

//ʵ�������ʼ��
//����ֵ���׽��֣����������׽��֣�
//�˿�
//������port��ʾ�˿�
//		���*port��0����ô�Զ�����һ�����õĶ˿�
//
int startup(unsigned short* port)
{
	//���绷����ʼ��
	WSADATA data;
	int ret = WSAStartup(  //wsastartup�������ڳ�ʼ�����绷������ʼ��Ϊ0
		MAKEWORD(1, 1),   //1.1Э��汾
		&data);
	if (ret) {  //ret��Ϊ0����ʼ��ʧ��
		error_die("wsastartup");
	}

	//�����׽���server_socket
	int server_socket = socket(PF_INET,  //�׽������ͣ������׽��֣���ַ����ipv4
		SOCK_STREAM,  //����ʽ�׽���
		IPPROTO_TCP);  //tcpЭ��
	if (server_socket == -1) {
		// ��ӡ������ʾ����������
		error_die("�׽���");
	}

	//���ö˿ڿɸ��ã���ֹ�˿ڼ���
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
		(const char*)&opt, sizeof(opt));
	if (ret == -1) {
		error_die("setsockopt");  //���˿ڿɸ����Ƿ����óɹ�
	}

	//���÷������˵������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);  //���ö˿ںţ�htons��������ת��������ת���磩
	server_addr.sin_addr.s_addr = INADDR_ANY;  //����ip��ַΪ������ʣ�ֵΪ0

	//���׽���
	if (bind(server_socket, (struct sockaddr*)&server_addr,
		sizeof(server_addr)) < 0) {
		error_die("bind");
	}

	//��̬����һ���˿�
	int namelen = sizeof(server_addr);
	if (*port == 0) {
		if (getsockname(server_socket, (struct sockaddr*)&server_addr, &namelen) < 0) {
			error_die("getsockname");
		}
		*port = server_addr.sin_port;
	}

	//������������
	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}

	return server_socket;
}

//��ָ���Ŀͻ����׽���sock����ȡһ�����ݣ����浽buff��
//����ʵ�ʶ�ȡ�����ֽ���
int get_line(int sock, char *buff, int size)
{
	char c = 0;  // '\0'
	int i = 0;

	//ͨ��whileѭ������recv����ÿ�ν���һ���ַ������buff�У�ֱ���������з�
	while (i < size - 1 && c != '\n') {
		//recv��������һ������sock��һ����Ч���׽������������ڶ�������&c��һ��ָ��������ݵ�
		//��������ָ�룬����������1��ʾҪ���յ��ֽ��������ĸ�����0��ʾ��ʹ���κ�����Ľ���ѡ�
		int n = recv(sock, &c, 1, 0);  //��������ֵn��ʾʵ�ʽ��յ��ֽ�����������������Ƿ�ɹ��������ݡ�
		if (n > 0) {
			if (c == '\r') {
				n = recv(sock, &c, 1, MSG_PEEK);  //���ڽ�������ʱ���в鿴������ʵ��ȡ������
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
	//��ָ���׽��֣�����һ����ʾ��û��ʵ�ֵĴ���ҳ��
}

void not_found(int client) {
	//����404��Ӧ
	char buff[1024];

	strcpy(buff, "http/1.0 404 not found\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "server: rockhttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "content-type:text/html\n");  //�ı���ʽ
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	//����404��ҳ����
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
	//������Ӧ��ͷ�ļ���Ϣ
	char buff[1024];

	strcpy(buff, "http/1.0 200 ok\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "server: rockhttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);

	strcpy(buff, "content-type:text/html\n");  //�ı���ʽ
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
	cout << "һ������" << count << "�ֽڸ������\n";
}

//������Դ���ͻ���
void server_file(int client, const char* filename) {
	//to do. 
	int numchars = 1;
	char buff[1024];

	//���������ݰ���ʣ�������ж���
	while (numchars > 0 && strcmp(buff, "\n")) {
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	//file* resource = fopen(filename, "r");
	FILE* resource = NULL;
	if (strcmp(filename, "htdocs/index.html") == 0) {  //�����ļ���Դ���ͻ���
		resource = fopen(filename, "r");
	}
	else {
		resource = fopen(filename, "rb");
	}
	if (resource == NULL) {
		not_found(client);
	}
	else {
		//��ʽ������Դ�������
		headers(client);

		//�����������Դ��Ϣ
		cat(client, resource);

		PRINTF("��Դ�������!\n");
	}

	fclose(resource);
}

//�����û�������̺߳���
DWORD WINAPI accept_request(LPVOID arg)
{
	char buff[1024];  //1k
	int client = (SOCKET)arg;  //�ͻ����׽���

	//��ȡһ������
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);

	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;
	PRINTF(method);

	//�������ķ��������������Ƿ�֧��
	if (stricmp(method, "get") && stricmp(method, "post")) {
		//�����������һ��������ʾҳ��
		unimplement(client);
		return 0;
	}

	//������Դ�ļ���·��
	//"get / http/1.1\n"
	char url[255];  //����������Դ������·��
	i = 0;
	//������Դ·��ǰ��Ŀո�
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
		//�������ʣ�����ݶ�ȡ���
		while (numchars > 0 && strcmp(buff, "\n")) {
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);  //�����������һ��404ҳ��
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
	unsigned short port = 80;  //���ö˿ں�
	int server_sock = startup(&port);
	cout << "http�����������������ڼ���" << port << "�˿�\n" << endl;

	struct sockaddr_in client_addr;  //��ʾipv4�����ַ�����ݽṹ
	int client_addr_len = sizeof(client_addr);  //�ṩ��ַ����

	//to do
	while (1)
	{
		/*
		����ʽ�ȴ��û�ͨ���������������û��׽��֣����û����н����ȴ��ͻ���
		�������������ʺܶ�ʱ��ӿڽ��յ�ַʱ������Ҫһ����ַ�ĳ���
		client_sock����һ���µ��׽��֣���������ӣ���server_sock�׽��ֽ���һ��һ����
		*/

		int client_sock = accept(server_sock,  //�����׽��֣��ȴ��û��������
			(struct sockaddr*)&client_addr,  //��¼�û����Ľ��з��ʵ�
			&client_addr_len);  //��ַ����
		if (client_sock == -1) {
			error_die("accept");
		}

		//�������߳�
		DWORD threadid = 0;
		CreateThread(0, 0,
			accept_request,  //�����û�������̺߳���
			(void*)client_sock,  //�������׽��ֶ��û����з��񣬸ò������ݸ��̺߳���
			0, &threadid);

	}

	closesocket(server_sock);  //�ر��׽���
	return 0;
}