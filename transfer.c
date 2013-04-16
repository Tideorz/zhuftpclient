/*Copyright (c) zhuhaichao
 *
 * file:transfer.c
 *
 * function:command and data interract
 *
 */

#include "transfer.h"

file_buf *filebuf_head = NULL;
file_buf *filebuf_tail  = NULL;
gint remote_filenum = 0;

/*make connection with server return -1 error, socket number if Ok*
 * set nsec to deal with connect timeout
 */
int make_server_connection(const gchar *host, const gchar *service,int nsec)
{
	struct hostent *phe;
	struct servent *pse;
	struct sockaddr_in sin;
	int s;

	memset(&sin,0,sizeof(sin));
	sin.sin_family = PF_INET;

	if(pse = getservbyname(service,"tcp"))
	{
		sin.sin_port = pse->s_port;
	}
	else if((sin.sin_port = htons((unsigned short)atoi(service))) == 0)
	{
		return -1;
	}

	if(phe = gethostbyname(host))
	{
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	{
		return -1;
	}

	s = socket(PF_INET,SOCK_STREAM,0);
	if(s<0)
	{
		return -1;
	}
	/*be used in select*/
	struct timeval tm;
	fd_set set;
	unsigned long ul = 1;
	/*set nonblock*/
	ioctl(s,FIONBIO,&ul);
	/*false means there is something wrong with connect*/
	gboolean ret = FALSE;
	int error = -1;
	int len = sizeof(int);

	if(connect(s,(struct sockaddr*)&sin,sizeof(sin)) == -1)
	{
		tm.tv_sec = TIME_OUT_SEC;
		tm.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(s,&set);
		int result;
		result =select(s+1,NULL,&set,NULL,(struct timeval*)&tm);
		if(result > 0)
		{
			getsockopt(s,SOL_SOCKET,SO_ERROR,&error,(socklen_t*)&len);
			if(error == 0)
			{
				printf("connect success\n");
				ret = TRUE;
			}
			else ret = FALSE;
		}
		else ret = FALSE;
	}
	else ret = TRUE;

	ul = 0;
	ioctl(s,FIONBIO,&ul);

	if(ret == FALSE)
	{
		close(s);
		return -1;
	}
	return s;
}

/*send the command to server*/
gboolean send_command_to_server(int sock, const char *command)
{
	int len;
	if(sock < 0)
	{
		printf("command sock < 0\n");
		return FALSE;
	}
	if((len = send(sock,command,strlen(command),0)) < 0)
	{
		printf("cmd: %s, sock: %d\n", command, sock);
		perror("send cmd error:\n");
		return FALSE;
	}
	return TRUE;
}

/*get reply from server*/
gboolean get_reply_from_server(int sock, char *result)
{
	memset(result,0,sizeof(result));
	char buf[READBUFSIZE];
	memset(buf, 0, READBUFSIZE);
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	int len;
	if((len = recv(sock, buf, READBUFSIZE, 0))<0)
	{
		return FALSE;
	}
	set_data(buf,result);
	return TRUE;
}

/*judge the return str, Ok return True, Wrong return False*/
gboolean command_is_ok(const char * ok_str, const char *result)
{
	if(strncmp(ok_str,result,sizeof(ok_str)-1) == 0)
	{
		return TRUE;
	}
	else return FALSE;
}

/* send command to server, write and read reply: -1 send error
 * -2 read error,-3 command content error*/
gint control_connection_operation(int sock, char *command, char *reply, const char *ok_str)
{
	gboolean flag;
	memset(reply,0,strlen(reply));
	flag = send_command_to_server(sock, command);
	if(!flag)
	{
		return -1;
	}

	flag = get_reply_from_server(sock, reply);
	printf("reply: %s\n", reply);
	if(!flag)
	{
		return -2;	
	}

	flag = command_is_ok(ok_str, reply);
	if(!flag)
	{
		return -3;
	}
	memset(command,0,sizeof(command));
	return 0;
}

/* set the value of data_buf*/
void set_data(char *src, char *dest)
{
	int i =0;
	int tag =0;
	gboolean hasbegun = FALSE;
	for(;;i++)
	{
		if(hasbegun == TRUE)
		{
			dest[tag] = src[i];
			if( src[i] == '\0' ||src[i] == '\r' || src[i] == '\n' )
			{
					dest[tag] = '\0';
					break;
			}
			tag++;
		}
		else
		{
			if(src[i] == ' ' || src[i] =='	')
				continue;
			else
			{
				dest[0] = src[i];
				hasbegun = TRUE;
				tag = 1;

			}
		}
	}
	dest[tag] = '\0';
}


/* the port method to create data connection*/
gint port_transfer_bind(struct sockaddr_in *port_server_sin,const char *ip)
{
	/*the cleint addr */
	int clieaddr_len;
	int servaddr_len;
	struct hostent *phe;
	socklen_t server_len;
	int len = 0;

	port_server_sin->sin_family = PF_INET;
	/*the ip addr of server*/
	//if(phe= gethostbyname(ip))
	//{
	//	memcpy(&port_server_sin->sin_addr.s_addr,phe->h_addr,phe->h_length);
	//}
	if((port_server_sin->sin_addr.s_addr = inet_addr(ip)) == INADDR_NONE)
	{
		return -1;
	}

	/*port of server*/
	port_server_sin->sin_port = htons(0);
	servaddr_len = sizeof(port_server_sin);
	port_listen_sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(port_listen_sock < 0)
	{
		return -2;
	}

	int flag = 1;
	setsockopt(port_listen_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&flag,sizeof(int));
	setsockopt(port_listen_sock,IPPROTO_TCP,1,(char*)&flag,sizeof(int));
	
	server_len = sizeof(*port_server_sin);
	/*bind function*/
	if(bind(port_listen_sock,(struct sockaddr*)port_server_sin,server_len)<0)
	{
		perror("bind error:\n");
		return;
	}
	/*listen function*/
	if(listen(port_listen_sock,1)<0)
	{
		printf("listen error\n");
		return -3;
	}
	return 0;
}

/*get the remote file list content*/
gint get_remote_filelist(int data_fd)
{
	 gint len;
	 file_buf *filebuf_new;
	 char buf[READBUFSIZE];
	
	 if(filebuf_head != NULL)
	 {
		free_remote_filelist(&filebuf_head);
	 }

	 filebuf_head = malloc(sizeof(file_buf));
	 if(filebuf_head == NULL)
	 {
		 return -1;
	 }
	 filebuf_tail = filebuf_head;
	 
	 /*set the read time out option*/
	 struct timeval tv;
	 tv.tv_sec = READ_TIME_OUT;
	 tv.tv_usec = 0;
	 setsockopt(data_fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
	 while(1)
	 {
		memset(buf,0,READBUFSIZE);
	 	len = recv(data_fd,buf,READBUFSIZE,0);
		if(len < 0)
		{
			perror("recv:");
			/*network error*/
			return -2;
		}

		if(len == 0)
		{
			break;
		}

		if(len > 0)
		{
			filebuf_new = malloc(sizeof(file_buf));
			if(filebuf_new == NULL)
			{
				return -1;	
			}
			memset(filebuf_new->file_data,0,READBUFSIZE);
			strcpy(filebuf_new->file_data,buf);
			filebuf_new->next = NULL;
			if(filebuf_head == filebuf_tail)
			{
				filebuf_tail = filebuf_new;
				filebuf_head->next = filebuf_new;
			}
			else
			{
				filebuf_tail->next = filebuf_new;
				filebuf_tail = filebuf_new;
			}
		}
	 }
	 return 0;
}


/*free the remote filelist*/
void free_remote_filelist(file_buf **head)
{
	file_buf *temp;
	temp = (*head)->next;
	while(temp != NULL)
	{
		free(*head);
		*head = temp;
		temp = temp->next;
	}
	free(*head);
	/*set NULL*/
	*head = NULL;
}

/*stor command send file to server*/
gboolean send_file_to_server(int fd, int data_sock)
{
	char send_buf[SENDBUFSIZE];
	if(fd < 0 || data_sock <0)
	{
		return FALSE;
	}
	while(1)
	{
		memset(send_buf,0,SENDBUFSIZE);
		int len = read(fd, send_buf, SENDBUFSIZE);
		printf("%s,   len = %d\n",send_buf, strlen(send_buf));
		if(len > 0)
		{
			if(data_sock > 0)
			{
				len = send(data_sock, send_buf, strlen(send_buf), 0);
				printf("send buf size : %d", len);
				if(len < 0)
				{
					char *msg = "Send file error";
					printf("%s\n", msg);
					return FALSE;
				}
			}
			else return FALSE;
		}
		else break;
	}
	close(fd);
	return TRUE;
}
