/* Copyright (c)  2013 3.16*
 * zhuhaichao
 * 
 * file: ftpclient_UI.h
 *
 * description:declare some functions about the
 * gtk-UI 
 */
#include "defines.h"
#include "fileutils.h"
#include "transfer.h"

#ifndef __FTPCLIENT_UI_H
#define __FTPCLIENT_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

	
	static File *local_file = NULL;
	static File *remote_file = NULL;
	static GtkTreeModel *model = NULL;

	GtkWidget *create_window(void);

	/*associate entry with struct File*/
	gboolean response_entry_enter(GtkWidget *widget, GdkEvent *event, gpointer data);

	static void add_columns(GtkTreeView *treeview);
	void show_text_news(const char *msg);
	gboolean response_button_login_down(GtkWidget *widget, GdkEvent *event, gpointer data);
	void deal_with_error(gint flag);
	gboolean send_and_response(int sock, char *command, const char *ok_str, char *reply, int len);
	void response_radio_button(GtkWidget *widget, gpointer data);
	void * make_data_connection(void *arg);
	gboolean response_button_abort_down(GtkWidget *widget, gpointer data);
	void get_remote_list();
	void on_changed(GtkWidget *widget, gpointer file);
	void port_transfer_accept(int *data_sock);
	gboolean create_data_sock_with_command(const char *command, const char *ok_str, int len);
	void get_data_sock(int *data_sock);
	void get_226_reply(int command_sock);
	void *remote_list_thread(void *arg);
	void *send_file_thread(void *info);
	void *create_list_model_idle(void *new_model);
	void * upload_stor_thread(void *arg);

#ifdef __cplusplus
}
#endif


#endif
