/* Copyright (c) zhuhaichao
 *
 * defines.h
 *
 * function:declare global variable
 * server reply message
 */


#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <glib.h>
#include <grp.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


#ifndef __DEFINES_H
#define __DEFINES_H

#ifdef __cplusplus
extern "C"
{
#endif
	#define MAXPATHLEN 256
	#define MAXUSERLEN 50
	#define SENDBUFSIZE 1024 
	#define READBUFSIZE 1024 

	/*network time out set*/
	#define READ_TIME_OUT   10    
	#define DELIM " "

	/*set 10 secs connect time out*/
	#define TIME_OUT_SEC 50
	char current_dir[MAXPATHLEN];

	/*the FTP command*/
	#define CMD_USER "USER %s\r\n"
	#define CMD_PASS "PASS %s\r\n"
	#define CMD_SYST "SYST\r\n"
	#define CMD_PWD  "PWD\r\n"
	#define CMD_LIST "LIST\r\n"
	#define CMD_PORT "PORT %s,%s,%s\r\n"
	#define CMD_STOR "STOR %s\r\n"

	enum
	{
		E_LIST,
		E_STOR
	};

	/*the local file list info*/
	enum 
	{
		COLUMN_FILENAME,
		COLUMN_FILESIZE,
		COLUMN_OWNNAME,
		COLUMN_GROUPNAME,
		COLUMN_DATE,
		COLUMN_FILESTATE,
		NUM_COLUMNS
	};

	typedef struct
	{
		 char *file_name;
      	 	 guint file_size;
		 gchar *own_name;
		 gchar *group_name;
		 gchar *date;
		 gchar *file_state;
	}File;

	typedef struct file_buf file_buf;
	struct file_buf
	{
		char file_data[READBUFSIZE];
		file_buf *next;
	};

	typedef struct file_info file_info;
	struct file_info
	{
		char file_data[READBUFSIZE];
		struct file_info *next;
	};
	/*local list's files total number*/
	extern  int sum_files;
	/*List command use global variables*/

	/*head and tail  of link*/

	extern file_buf *filebuf_head;
	extern file_buf *filebuf_tail;
	extern  gint remote_filenum;

	extern file_info *fileinfo_head;
	extern file_info *fileinfo_tail;

	/*port command thread data struct*/
	typedef struct
	{
		char *ip;
		int *data_sock;
	}port_data;

	/* the client  listen socket*/
	int port_listen_sock;



#ifdef __cplusplus
}
#endif

#endif
