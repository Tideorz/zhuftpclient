/* Copyright (c) zhuhaichao
 *
 * file: transfer.h
 *
 * function:some functions about data or control
 *
 */

#include "defines.h"
#ifndef __TRANSFER_H
#define __TRANSFER_H

#ifdef __cplusplus
extern "C"
{
#endif

	int make_server_connection(const gchar *host, const gchar *service,int nsec);
	gboolean send_command_to_server(int sock, const char *command);
	gboolean get_reply_from_server(int sock, char *result);
	gboolean command_is_ok(const char * ok_str, const char *result);
	gint control_connection_operation(int sock, char *command, char *reply, const char *ok_str);
	void set_data(char *src, char *dest);
	gint port_transfer_bind(struct sockaddr_in *port_server_sin,const char *ip);
	gint get_remote_filelist(int data_fd);
	void free_remote_filelist(file_buf **head);
	gboolean send_file_to_server(int fd, int data_sock);


#ifdef __cplusplus
}
#endif
#endif
