 /* zhuhaichao
 * 
 * file: ftpclient_UI.c
 *
 * description:define functions about the
 * gtk-UI 
 */

#include "ftpclient_UI.h"

GtkWidget *sw_locallist;
GtkWidget *treeview_local;
static GtkWidget *entry_localwd;  	/*current absolute path*/

GtkWidget *sw_remotelist;
GtkWidget *treeview_remote;

GtkWidget *entry_IP;
GtkWidget *entry_PORT;
GtkWidget *entry_username;
GtkWidget *entry_PASSWD;

/*the server directory*/
GtkEntryBuffer *entry_buffer_serverwd;


/*client function return msgs show in it*/
GtkWidget *text_view_interract;
GtkTextMark *text_mark_log;

/*if login button be pressed set this value TRUE*/
gboolean is_abort_down = FALSE;

/*the local files num*/
int sum_files;
char command[SENDBUFSIZE];
char command_str[SENDBUFSIZE];
char reply[100];

/*the command connection's socket*/
int command_sock;
/*data connection fd*/
int data_sock = -1;
/*the data connection method , 1 port ,2 passive*/
gint data_connection_method = 1;


/*upload file info*/
File upload_file;
typedef struct{
	int fd;
	int data_sock;
}fd_sock;

fd_sock *fs_info;	
fd_sock fs;

/*Posix thread */
pthread_mutex_t gtk_mutex;
pthread_cond_t gtk_cond;
pthread_attr_t detach_attr;

pthread_t thread_upload_stor;
pthread_t thread_list;
pthread_t thread_stor;


/*close the window ,later I will create a dialoag for the user to choose whether
 * to close the ftp-client*/
gint delete_event_top_window(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	return FALSE;
}

/*destroy the widget ,kill the process*/
void destroy(GtkWidget *widget, gpointer data)
{
	gtk_main_quit();
}

/*you can add a dialog latter*/
void destroy_widget(GtkWidget *widget, gpointer data)
{
	GtkWidget *de_widget = (GtkWidget*) data;
	gtk_widget_destroy(de_widget);
}

/*create a list store model to show the local file or directory status*/
static GtkTreeModel *create_list_model(File *file_data,gint sum_files)
{
	gint  i;
	GtkTreeIter iter;
	GtkListStore *store;
	store = gtk_list_store_new(NUM_COLUMNS,
				   G_TYPE_STRING,
				   G_TYPE_UINT,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING,
				   G_TYPE_STRING
			);
	for(i=0; i<sum_files; i++)
	{
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,
				   COLUMN_FILENAME, ((file_data->file_name)),
				   COLUMN_FILESIZE, (file_data->file_size),
				   COLUMN_OWNNAME, ((file_data->own_name)),
				   COLUMN_GROUPNAME, ((file_data->group_name)),
				   COLUMN_DATE, ((file_data->date)),
				   COLUMN_FILESTATE, ((file_data->file_state)),
				   -1);
		file_data++;
	}
	return GTK_TREE_MODEL(store);

}

static void add_columns (GtkTreeView *treeview)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* column for  file name */
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size(renderer,100,-1);
  column = gtk_tree_view_column_new_with_attributes ("File Name",
                                                     renderer,
                                                     "text",
                                                     COLUMN_FILENAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_FILENAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for  file size*/
  column = gtk_tree_view_column_new_with_attributes (("File Size"),
                                                     renderer,
                                                     "text",
                                                     COLUMN_FILESIZE,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_FILESIZE);
  gtk_tree_view_append_column (treeview, column);

  /* column for  own name*/
  column = gtk_tree_view_column_new_with_attributes (("Own Name"),
                                                     renderer,
                                                     "text",
                                                     COLUMN_OWNNAME,
                                                     NULL);
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_OWNNAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for  group name*/
  column = gtk_tree_view_column_new_with_attributes (("Group Name"),
                                                     renderer,
						     "text",
                                                     COLUMN_GROUPNAME,
                                                     NULL);

  gtk_tree_view_column_set_sort_column_id (column, COLUMN_GROUPNAME);
  gtk_tree_view_append_column (treeview, column);

  /* column for date */
  column = gtk_tree_view_column_new_with_attributes (("Date"),
                                                     renderer,
						     "text",
                                                     COLUMN_DATE,
						     NULL);
   
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_DATE);
  gtk_tree_view_append_column (treeview, column);

    /* column for  file attribute*/
  column = gtk_tree_view_column_new_with_attributes (("Attribute"),
                                                     renderer,
						     "text",
                                                     COLUMN_FILESTATE,
						     NULL);
   
  gtk_tree_view_column_set_sort_column_id (column, COLUMN_FILESTATE);
  gtk_tree_view_append_column (treeview, column);

}

/* when you press the Enter key int entry_localwd call this function*/
gboolean response_entry_enter(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	const gchar* path; 
	char  cw[MAXPATHLEN];
	path = gtk_entry_get_text(GTK_ENTRY(widget));
	File * file = local_file;
	GtkTreeModel *new;
	
	if(write_locallist(path,&local_file) == FALSE)
	{
		show_text_news("Path  Error!!!");
		return FALSE;
	}
	getcwd(cw,MAXPATHLEN);
	gtk_entry_set_text(GTK_ENTRY(widget),cw);

	new = create_list_model(local_file,sum_files);
	gtk_tree_view_set_model((GtkTreeView*)treeview_local,new);
	g_object_unref(new);
	return TRUE;
}

/*input conten into remote filelist*/
gboolean set_remote_filelist_treeview_model()
{
	GtkTreeModel *new;
	if(write_remotelist(&remote_file) == FALSE)
	{
		char *msg = "Remote filelist create Error";
		g_idle_add((GSourceFunc)show_text_news,msg);
		return FALSE;
	}
	g_idle_add(create_list_model_idle,new);
	return TRUE;
}

void *create_list_model_idle(void *new_model)
{
	GtkTreeModel *new = (GtkTreeModel*)new_model;
	new = create_list_model(remote_file,remote_filenum);
	gtk_tree_view_set_model((GtkTreeView*)treeview_remote,new);
	show_text_news("create remote list modle");
	g_object_unref(new);
	return FALSE;
}

/*if there is something wrong with control connection*
 * use this function to show in text veiw*/
void deal_with_error(gint flag)
{
	switch(flag)
	{
		case -1:
			show_text_news("Try to reconnect the server"); 
			break;
		case -2:
			show_text_news("Havn't got the server reply");
			break;
		case -3:
			show_text_news("Command content error");
			break;
		default:
			break;
	}
}

/*the client and server interract show in text view*/
gboolean send_and_response(int sock, char *command, const char *ok_str, char *reply, int len)
{
	show_text_news("\n");
	show_text_news(command);
	command[len] = '\0';
	if(sock == -1)
	{
		show_text_news("Command connection has been close");
		return FALSE;
	}
	int flag = control_connection_operation(sock, command, reply, ok_str); 	
	show_text_news(reply);
	if(flag < 0 && flag != -3)
	{
		deal_with_error(flag);
		close(sock);
		return FALSE;
	}
	return TRUE;
}

/*get the server work directory*/
void get_work_directory(char * reply)
{
	char *p = strtok(reply,"\"");
	p = strtok(NULL,"\"");
	gtk_entry_buffer_set_text(entry_buffer_serverwd,p,strlen(p));
}

/*create the control connection with the server*/
gboolean response_button_login_down(GtkWidget *widget, GdkEvent *event, gpointer data)
{  
	int len;
	/*close the former connection first*/
	if(command_sock > 0)
	{
		/* end all the running thread*/
		pthread_cancel(thread_list);
		pthread_cancel(thread_stor);
		pthread_cancel(thread_upload_stor);
		
		if(data_sock > 0)
		{
			close(data_sock);
			data_sock = -1;
		}

		close(command_sock);
		command_sock = -1;
	}

	gint flag;
	gboolean result;

	const gchar *ip;
	const gchar *port;
	const gchar *user_name;
	const gchar *passwd;

	ip = gtk_entry_get_text(GTK_ENTRY(entry_IP));
	port = gtk_entry_get_text(GTK_ENTRY(entry_PORT));
	user_name = gtk_entry_get_text(GTK_ENTRY(entry_username));
	passwd = gtk_entry_get_text(GTK_ENTRY(entry_PASSWD));

	if(strcmp(ip,"") == 0 ||  strcmp(port,"") == 0   || strcmp(user_name,"") == 0)
	{
		show_text_news("Login Info  Not Complete!!!");
		return FALSE;
	}

	/*connect with server*/
	command_sock = make_server_connection(ip,port,0);
	if(command_sock == -1)
	{
		show_text_news("Can't create command connection,\n Server or Network is not doing!!!");
		return FALSE;
	}
	printf("command sock: %d\n", command_sock);
	show_text_news("Connection Finished\n");

	result = get_reply_from_server(command_sock, reply);
	if(!result)
	{
		deal_with_error(-2);
		close(command_sock);
		return FALSE;
	}
	show_text_news(reply);

	result = command_is_ok("220", reply);
	if(!result)
	{
		deal_with_error(-3);
		close(command_sock);
		return FALSE;
	}

	
	/*first step USER xxx*/
	len = sprintf(command, CMD_USER, user_name);
	result = send_and_response(command_sock, command, "331", reply, len);
	if(!result)
	{
		return FALSE;
	}

	/*second step PASS xxx*/
	len = sprintf(command, CMD_PASS, passwd);
	result = send_and_response(command_sock, command, "230", reply, len);
	if(!result)
	{
		return FALSE;
	}

	/* the type of the system*/
	len = sprintf(command, CMD_SYST, NULL);
	result = send_and_response(command_sock, command, "215", reply, len);
	if(!result)
	{
		return FALSE;
	}

	/*printf the server current dir*/
	len = sprintf(command, CMD_PWD, NULL);
	result = send_and_response(command_sock, command, "257", reply, len);
	if(!result)
	{
		return FALSE;
	}
	get_work_directory(reply);

	show_text_news("OK!!!!!!!!!!!");

	/* LIST command to show the server current dir*/
	len = sprintf(command, CMD_LIST, NULL);
	data_transfer_with_server(command, E_LIST, "150", len, NULL); 
	return TRUE;
}

void get_226_reply(int command_sock)
{
	char buf[READBUFSIZE]= { 0 };
	char *msg;
	if(command_sock < 0)
	{
		msg = "Command connection haven't been finished"; 
		show_text_news(msg);
//		g_idle_add((GSourceFunc)show_text_news,msg);
	}
	get_reply_from_server(command_sock, buf);
	int len = strlen(buf);
	printf("226 len:%d\n", len);
	if(len > 0)
	{
		printf("%s\n", buf);
		show_text_news(buf);
//		g_idle_add((GSourceFunc)show_text_news,buf);
	}
	else
	{
		msg = "Haven't get 226 reply";
		show_text_news(msg);
	//	g_idle_add((GSourceFunc)show_text_news,msg);
	}
	/*close the data connection*/
	close(data_sock);
	data_sock = -1;
}

/*the command respond with data tranfer*/
gboolean create_data_sock_with_command(const char *command, const char *ok_str, int len)
{
	gboolean  result;
	result = send_and_response(command_sock, command, ok_str, reply, len);
	if(!result)
	{
		return FALSE;
	}
	get_data_sock(&data_sock);
	return TRUE;
}

void get_data_sock(int *data_sock)
{
	if(data_connection_method == 1)
	{
		 port_transfer_accept(data_sock);
	}

	if(data_connection_method == 2)
	{
		show_text_news("passive method haven't been achieved");
	}
}

/*port method use the accept to get data_sock*/
void port_transfer_accept(int *data_sock)
{
	/*accept function*/
	struct sockaddr_in client_sin;
	while(1)
	{
		unsigned int client_len = sizeof(client_sin);
		*data_sock = accept(port_listen_sock,(struct sockaddr*)&client_sin,&client_len);
		if(data_sock < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				perror("accept error:\n");
				exit(0);
			}
		}
		else break;
	}
}

/*the thread function to deal with data interract : port or passive*/
void * make_data_connection(void *arg)
{

	show_text_news("Input into make_data_connection");
	char reply[100];
	int len;
	gint result;

	/*port method*/
	if(data_connection_method == 1)
	{
		/*set the port method's ip and port*/
		char *ip = "127.0.0.1";
		struct sockaddr_in port_server_sin;

		/*bind the listen socket*/
		result = port_transfer_bind(&port_server_sin,ip);
		switch(result)
		{
			case -1:
				show_text_news("the ip error!!!");
				close(port_listen_sock);
				return;
			case -2:
				show_text_news("port method bind error");
				close(port_listen_sock);
				return;
			case -3:
				show_text_news("listen error");
				close(port_listen_sock);
				return;
			default:
				break;
		}
		
		/*get the port info from getsockname*/
		socklen_t s_len = sizeof(port_server_sin);
		if(getsockname(port_listen_sock,(struct sockaddr*)&port_server_sin,&s_len) != 0)
		{
			show_text_news("Can't get port info");
		}
		int port = ntohs(port_server_sin.sin_port);
		char former[5];
		sprintf(former,"%d",port/256);
		char latter[5];
		sprintf(latter,"%d",port%256);

		/*the client is listening, send PORT xxx*/
		int len = sprintf(command, CMD_PORT, "127,0,0,1", former, latter);	
		int result = send_and_response(command_sock, command, "200", reply, len);
		if(result == FALSE)
		{
			return;
		}
	}

	/*passive method*/	
	if(data_connection_method == 2)
	{
		show_text_news("passive method");
		return;
	}
}

/*get the remote file list info*/
void get_remote_list()
{
	if(data_sock < 0)
	{
		return;
	}
	/*get remote file directory*/		
	int result = get_remote_filelist(data_sock);
	get_226_reply(command_sock);
	char *msg;
	switch(result)
	{
		case -1:
			msg = "Remote filelist link malloc Error!!!";
			g_idle_add((GSourceFunc)show_text_news,msg);
			return;
		case -2:
			msg = "Recv Error!!!(data connection)";
			g_idle_add((GSourceFunc)show_text_news,msg);
			return ;
		case 0:
			
			break;
	}
	/*write into remote_file*/
	set_remote_filelist_treeview_model();
}


/*abort button operations*/
gboolean response_button_abort_down(GtkWidget *widget, gpointer data)
{
	char command[20];
	char reply[100];
	gboolean result;
	if(data_sock != -1)
	{
		int len = sprintf(command,"ABOR\r",NULL);
		result = send_and_response(command_sock,command,"226",reply,len);
		if(!result)
		{
			return FALSE;
		}
		/*set the data socket -1*/
		close(data_sock);
		data_sock = -1;

		/*end  the data connection thread*/
		pthread_cancel(thread_list);
		pthread_cancel(thread_stor);
		pthread_cancel(thread_upload_stor);
	}
	else show_text_news("Haven't make data connection   ");
	return TRUE;
}


/*set the data connection method through the radio button choice*/
void response_radio_button(GtkWidget * widget, gpointer data)
{
	data_connection_method = (gint)data;
}



void show_text_news(const char *msg)
{
	GtkTextIter iter;
	GtkTextBuffer *buf;
	char *endl = "\n";
	buf = gtk_text_view_get_buffer((GtkTextView*)text_view_interract);
	gtk_text_buffer_get_end_iter(buf,&iter);
	gtk_text_buffer_insert(buf,&iter,msg,-1);
	gtk_text_buffer_insert(buf,&iter,endl,-1);
	gtk_text_view_set_buffer((GtkTextView*)text_view_interract,buf);
	text_mark_log = gtk_text_buffer_create_mark(buf,NULL,&iter,1);

	//scrolle the text_view_interract
	gtk_text_buffer_move_mark(buf,text_mark_log,&iter);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view_interract),text_mark_log,0,1,1,1);
	return FALSE;
}


/*string change into utf-8*/
gchar *_(char *string)
{
	return (g_locale_to_utf8(string,-1,NULL,NULL,NULL));
}



/*response to the upload button clicked*/
void response_button_upload(GtkWidget *widget)
{
	show_text_news("go into upload!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	if(command_sock <= 0 )
	{
		show_text_news("Command connection Not Finished!!!");
		return;
	}
	
	if(upload_file.file_name == NULL)
	{
		show_text_news("Please select a file in local filelist first!!!");	
		return ;
	}
	char path[MAXPATHLEN];
	char send_buf[SENDBUFSIZE];
	char *directory;
	/*get the absolute path name*/
	directory = gtk_entry_get_text((GtkEntry*)entry_localwd);
	strcpy(path,directory);
	strcat(path,"/");
	strcat(path,upload_file.file_name);

	int fd;
	fd = open(path,O_RDONLY);
	if(fd == -1)
	{
		show_text_news("Can't open the upload file");
		return;
	}

	/*necessary info to stor command*/
	fs.fd = fd;
	fs.data_sock = data_sock;

	/*stor thread*/
	pthread_create(&thread_upload_stor, &detach_attr, upload_stor_thread, (void*)&fs);
}

void* upload_stor_thread(void *arg)
{
	pthread_cleanup_push(pthread_mutex_unlock, (void*)&gtk_mutex);

	int len = sprintf(command, CMD_STOR, upload_file.file_name);

	data_transfer_with_server(command, E_STOR, "150", len, (void*)arg);
	pthread_testcancel();

	//pthread_cleanup_push(pthread_mutex_unlock, (void*)&gtk_mutex);
	pthread_mutex_lock(&gtk_mutex);
	pthread_cond_wait(&gtk_cond, &gtk_mutex);

	/*list command update the server list*/
	len = sprintf(command, CMD_LIST, NULL);
	data_transfer_with_server(command, E_LIST, "150", len, NULL);
	pthread_testcancel();

	pthread_mutex_unlock(&gtk_mutex);
	pthread_cleanup_pop(0);

}

/*the core function handle with data transfer*/ 
void data_transfer_with_server(const char *command, int cmd,
			       const char *ok_str, int len, void *info)
{
	char command_info[SENDBUFSIZE];
	strcpy(command_info,command);
	make_data_connection(NULL);

	create_data_sock_with_command(command_info, ok_str, len);


	/*get the data sock*/

	switch(cmd)
	{
		case E_LIST:
			pthread_create(&thread_list, &detach_attr, remote_list_thread, NULL);
			break;
		case E_STOR:
			fs_info = (fd_sock*)info;
			fs_info->data_sock = data_sock;
			pthread_create(&thread_stor, &detach_attr, send_file_thread, (void*)fs_info);
			break;
		default:
			return;
	}
}

/*response to the filelist store selection*/
void on_changed(GtkWidget *widget, gpointer file)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	File *ptr = (File*)file;
	if(ptr->file_name != NULL)
	{
		free(ptr->file_name);
		free(ptr->own_name);
		free(ptr->group_name);
		free(ptr->date);
		free(ptr->file_state);
	}
	
	/*get the file will ben transfered*/
	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter))
	{
		gtk_tree_model_get(model,&iter,	
				COLUMN_FILENAME,&(ptr->file_name),
				COLUMN_FILESIZE,&(ptr->file_size),
				COLUMN_OWNNAME,&(ptr->own_name),
				COLUMN_GROUPNAME,&(ptr->group_name),
				COLUMN_DATE,&(ptr->date),
				COLUMN_FILESTATE,&(ptr->file_state),
				-1);
		show_text_news("new selection has been selected");
	
	}

}

/*thread func about get remote list*/
void *remote_list_thread(void *arg)
{

	
	pthread_cleanup_push(pthread_mutex_unlock, (void*)&gtk_mutex);
	pthread_mutex_lock(&gtk_mutex);

	pthread_testcancel();
	get_remote_list();
	pthread_testcancel();

	pthread_mutex_unlock(&gtk_mutex);
	pthread_cleanup_pop(0);
}

/*thread func about store file on server*/
void *send_file_thread(void *info)
{

	pthread_cleanup_push(pthread_mutex_unlock, (void*)&gtk_mutex);
	pthread_mutex_lock(&gtk_mutex);

	gboolean result;
	fd_sock *fs_info = (fd_sock*)info;
	
	pthread_testcancel();
	result = send_file_to_server(fs_info->fd, fs_info->data_sock);
	pthread_testcancel();
	if(!result)
	{
		char *msg = "Error occur in stor command";
		g_idle_add((GSourceFunc)show_text_news,msg);
	}

	pthread_testcancel();
	get_226_reply(command_sock);
	pthread_testcancel();

	pthread_mutex_unlock(&gtk_mutex);
	pthread_cleanup_pop(0);

	/* notify the upload thread to finish list thread*/
	pthread_cond_signal(&gtk_cond);
}

void clear_log()
{
	GtkTextBuffer *buf = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(buf, " ", 1);
	gtk_text_view_set_buffer((GtkTextView*)text_view_interract, buf);
}

/*create the UI window layout*/
GtkWidget *create_window(void)
{

	/*initial the mutex*/
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
	if(pthread_mutex_init(&gtk_mutex,&mutex_attr) != 0)
	{
		exit(0);
	}
	/*set the detach state to thread*/
	pthread_attr_setdetachstate(&detach_attr, PTHREAD_CREATE_DETACHED);

	/*condition initialization*/
	pthread_cond_init(&gtk_cond, NULL);

	/*main window of the ftp-client*/
	GtkWidget *main_window;

	/*vbox of the main_window*/
	GtkWidget *vbox_main;
	
	/*menu*/
	GtkWidget *hbox_menu;
	GtkWidget *menu_bar;
	GtkWidget *menu_ftp;
	GtkWidget *menu_log;
	GtkWidget *item_FTP;
	GtkWidget *item_Quit;
	GtkWidget *item_Log;

	GtkWidget *menu_help;
	GtkWidget *item_Help;
	GtkWidget *item_About;
	GtkWidget *item_Clear;

	/*login*/
	

	GtkWidget *hbox_login;
	GtkWidget *label_IP;
	GtkWidget *label_PORT;
	GtkWidget *label_username;
	GtkWidget *label_PASSWD;
	GtkWidget *button_login;
	GtkWidget *image_login;
	GtkWidget *button_abort;
	GtkWidget *image_abort;
	GtkWidget *vbox_radio_button;
	GtkWidget *radio_button_port;
	GtkWidget *radio_button_passive;

	/*file list*/
	GtkWidget *hbox_filelist;

	GtkWidget *vbox_locallist;
	GtkWidget *entry_buf_cw;

	/*finish the store and downlocad function*/
	GtkWidget *vbox_button;
	GtkWidget *button_upload;
	GtkWidget *button_download;

	GtkWidget *vbox_remotelist;
	GtkWidget *entry_remotewd;
	

	/*file transfer schedule*/
	GtkWidget *hbox_schedule;

	/*  C/S interract text view */
	GtkWidget *hbox_interract;
	GtkWidget *sw_interract;
	

	/*create the main_window*/
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_usize(main_window,1200,600);
	gtk_window_set_title((GtkWindow*)main_window,_("zhuhaichao-ftpclient"));
	gtk_window_set_position((GtkWindow*)main_window,GTK_WIN_POS_CENTER);
	gtk_window_set_resizable((GtkWindow*)main_window,TRUE);
	g_signal_connect(main_window,"delete_event",G_CALLBACK(delete_event_top_window),NULL);
	g_signal_connect(main_window,"destroy",G_CALLBACK(destroy),NULL);

	/*vbox_main*/
	vbox_main = gtk_vbox_new(FALSE,0);
	gtk_widget_show(vbox_main);
	gtk_container_add((GtkContainer*)main_window,vbox_main);



	/*create hbox_menu*/
	hbox_menu = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox_menu);
	gtk_box_pack_start((GtkBox*)vbox_main,hbox_menu,FALSE,FALSE,0);
	
	menu_bar = gtk_menu_bar_new();
	gtk_widget_show(menu_bar);
	gtk_box_pack_start((GtkBox*)hbox_menu,menu_bar,TRUE,TRUE,0);

	menu_ftp = gtk_menu_new();
	gtk_widget_show(menu_ftp);

	menu_help = gtk_menu_new();
	gtk_widget_show(menu_help);

	menu_log = gtk_menu_new();
	gtk_widget_show(menu_log);

	item_FTP = (GtkWidget*)gtk_menu_item_new_with_label(_("FTP"));
	gtk_widget_show(item_FTP);

	item_Quit = (GtkWidget*)gtk_menu_item_new_with_label(_("Quit"));
	gtk_widget_show(item_Quit);

	item_Help = (GtkWidget*)gtk_menu_item_new_with_label(_("Help"));
	gtk_widget_show(item_Help);

	item_About = (GtkWidget*)gtk_menu_item_new_with_label(_("about"));
	gtk_widget_show(item_About);

	item_Log = (GtkWidget*)gtk_menu_item_new_with_label(_("Log"));
	gtk_widget_show(item_Log);

	item_Clear = (GtkWidget*)gtk_menu_item_new_with_label(_("Clear"));
	gtk_widget_show(item_Clear);

	/*Ftp -> Quit*/
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),item_FTP);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_FTP),menu_ftp);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_ftp),item_Quit);

	g_signal_connect(item_Quit,"activate",G_CALLBACK(destroy_widget),main_window);

	/*Help -> about*/
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),item_Help);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_Help),menu_help);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_help),item_About);

	/*Log -> Clear*/
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar),item_Log);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item_Log),menu_log);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu_log),item_Clear);

	g_signal_connect(item_Clear,"activate",G_CALLBACK(clear_log),NULL);


	/*create hbox_login*/
	GtkEntryBuffer *entry_buffer_ip,*entry_buffer_port,*entry_buffer_username;
	gchar ip[10] = "127.0.0.1";
	gchar port[5] = "21";
	gchar username[20]="anonymous";
	entry_buffer_ip = gtk_entry_buffer_new(_(ip),sizeof(ip));
	entry_buffer_port = gtk_entry_buffer_new(_(port),sizeof(port));
	entry_buffer_username = gtk_entry_buffer_new(_(username),sizeof(username));

	hbox_login = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox_login);
	gtk_box_pack_start((GtkBox*)vbox_main,hbox_login,FALSE,FALSE,0);

	label_IP = gtk_label_new(_("IP:"));
	gtk_widget_show(label_IP);
	gtk_box_pack_start((GtkBox*)hbox_login,label_IP,FALSE,FALSE,0);

	entry_IP = gtk_entry_new_with_buffer(entry_buffer_ip);
	gtk_widget_show(entry_IP);
	gtk_box_pack_start((GtkBox*)hbox_login,entry_IP,TRUE,TRUE,0);

	label_PORT = gtk_label_new(_("  PORT:"));
	gtk_widget_show(label_PORT);
	gtk_box_pack_start((GtkBox*)hbox_login,label_PORT,FALSE,FALSE,0);

	entry_PORT = gtk_entry_new_with_buffer(entry_buffer_port);
	gtk_widget_show(entry_PORT);
	gtk_box_pack_start((GtkBox*)hbox_login,entry_PORT,TRUE,TRUE,0);


	label_username = gtk_label_new(_("  USER NAME:"));
	gtk_widget_show(label_username);
	gtk_box_pack_start((GtkBox*)hbox_login,label_username,FALSE,FALSE,0);

	entry_username = gtk_entry_new_with_buffer(entry_buffer_username);
	gtk_widget_show(entry_username);
	gtk_box_pack_start((GtkBox*)hbox_login,entry_username,TRUE,TRUE,0);

	label_PASSWD = gtk_label_new(_("  PASSWD"));
	gtk_widget_show(label_PASSWD);
	gtk_box_pack_start((GtkBox*)hbox_login,label_PASSWD,FALSE,FALSE,0);

	entry_PASSWD = gtk_entry_new();
	gtk_widget_show(entry_PASSWD);
	gtk_entry_set_visibility((GtkEntry*)entry_PASSWD,FALSE);
	gtk_entry_set_invisible_char((GtkEntry*)entry_PASSWD,'*');
	gtk_box_pack_start((GtkBox*)hbox_login,entry_PASSWD,TRUE,TRUE,0);

	button_login = gtk_button_new();
	gtk_widget_show(button_login);
	gtk_box_pack_start((GtkBox*)hbox_login,button_login,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(button_login),"clicked",G_CALLBACK(response_button_login_down),NULL);

	image_login = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image_login),GTK_STOCK_OK,GTK_ICON_SIZE_DND);
	gtk_widget_show(image_login);
	gtk_container_add(GTK_CONTAINER(button_login),image_login);


	button_abort = gtk_button_new();
	gtk_widget_show(button_abort);
	gtk_box_pack_start((GtkBox*)hbox_login,button_abort,FALSE,FALSE,0);
	g_signal_connect(G_OBJECT(button_abort),"clicked",G_CALLBACK(response_button_abort_down),NULL);

	image_abort = gtk_image_new();
	gtk_image_set_from_stock(GTK_IMAGE(image_abort),GTK_STOCK_CANCEL,GTK_ICON_SIZE_DND);
	gtk_widget_show(image_abort);
	gtk_container_add(GTK_CONTAINER(button_abort),image_abort);

	vbox_radio_button = gtk_vbox_new(FALSE,0);
	gtk_widget_show(vbox_radio_button);
	gtk_box_pack_start((GtkBox*)hbox_login,vbox_radio_button,TRUE,TRUE,0);

	radio_button_port = gtk_radio_button_new_with_label(NULL,_("PORT"));
	gtk_widget_show(radio_button_port);
	gtk_box_pack_start((GtkBox*)vbox_radio_button,radio_button_port,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(radio_button_port),"toggled",G_CALLBACK(response_radio_button),(gpointer)1);

	radio_button_passive = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button_port)),
								_("PASSIVE"));
	gtk_widget_show(radio_button_passive);
	gtk_box_pack_start((GtkBox*)vbox_radio_button,radio_button_passive,TRUE,TRUE,0);
	g_signal_connect(G_OBJECT(radio_button_port),"toggled",G_CALLBACK(response_radio_button),(gpointer)2);

	
	/*create hbox_filelist*/
	hbox_filelist = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox_filelist);
	gtk_box_pack_start((GtkBox*)vbox_main,hbox_filelist,TRUE,TRUE,0);
	gtk_widget_set_usize(hbox_filelist,1200,250);

	vbox_locallist = gtk_vbox_new(FALSE,0);
	gtk_widget_show(vbox_locallist);
	gtk_box_pack_start((GtkBox*)hbox_filelist,vbox_locallist,FALSE,FALSE,0);
	gtk_widget_set_usize(vbox_locallist,600,250);

	/*get current directory to entry_localwd*/
	if(getcwd(current_dir,MAXPATHLEN) == NULL)
	{
		exit(0);
	}
	int len = strlen(current_dir);
	entry_buf_cw = (GtkWidget*)gtk_entry_buffer_new(current_dir,len);

	entry_localwd = gtk_entry_new_with_buffer((GtkEntryBuffer*)entry_buf_cw);
	gtk_widget_show(entry_localwd);
	gtk_box_pack_start((GtkBox*)vbox_locallist,entry_localwd,FALSE,FALSE,0);

	sw_locallist = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show(sw_locallist);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_locallist),GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_locallist),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start((GtkBox*)vbox_locallist,sw_locallist,TRUE,TRUE,0);

	/*input data into local_file*/
	if(write_locallist(current_dir,&local_file) == FALSE)
	{
		exit(0);
	}
	model = create_list_model(local_file,sum_files);
	treeview_local = gtk_tree_view_new_with_model(model);
	gtk_widget_show(treeview_local);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_local),TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_local),NUM_COLUMNS); 
	gtk_container_add(GTK_CONTAINER(sw_locallist),treeview_local);
	add_columns(GTK_TREE_VIEW(treeview_local));
	g_object_unref(model);
	g_signal_connect(entry_localwd,"activate",G_CALLBACK(response_entry_enter),NULL);
	
	/*upload and download button*/
	vbox_button = gtk_vbox_new(FALSE,0);
	gtk_widget_show(vbox_button);
	gtk_box_pack_start((GtkBox*)hbox_filelist, vbox_button, FALSE, FALSE,0);

	button_upload = gtk_button_new_with_label("Upload");
	gtk_widget_show(button_upload);
	gtk_box_pack_start((GtkBox*)vbox_button, button_upload, FALSE, FALSE,50);
	g_signal_connect(button_upload,"clicked",G_CALLBACK(response_button_upload),NULL);

	button_download = gtk_button_new_with_label("Download");
	gtk_widget_show(button_download);
	gtk_box_pack_start((GtkBox*)vbox_button, button_download, FALSE, FALSE,50);


	/********************************************************/	
	vbox_remotelist = gtk_vbox_new(FALSE,0);
	gtk_widget_show(vbox_remotelist);
	gtk_box_pack_start((GtkBox*)hbox_filelist,vbox_remotelist,FALSE,FALSE,0);
	gtk_widget_set_usize(vbox_remotelist,500,250);

	entry_buffer_serverwd = gtk_entry_buffer_new(NULL,-1);

	entry_remotewd = gtk_entry_new_with_buffer(entry_buffer_serverwd);
	gtk_widget_show(entry_remotewd);
	gtk_box_pack_start((GtkBox*)vbox_remotelist,entry_remotewd,FALSE,FALSE,0);

	sw_remotelist = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show(sw_remotelist);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_remotelist),GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_remotelist),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start((GtkBox*)vbox_remotelist,sw_remotelist,TRUE,TRUE,0);

	model = create_list_model(remote_file,remote_filenum);
	treeview_remote = gtk_tree_view_new();
	gtk_widget_show(treeview_remote);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview_remote),TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(treeview_remote),NUM_COLUMNS); 
	gtk_container_add(GTK_CONTAINER(sw_remotelist),treeview_remote);
	add_columns(GTK_TREE_VIEW(treeview_remote));
	g_object_unref(model);

	/*get the local file list selection*/
	upload_file.file_name = NULL;
	upload_file.file_size = 0;
	upload_file.own_name = NULL;
	upload_file.group_name = NULL;
	upload_file.date = NULL;
	upload_file.file_state = NULL;

	GtkTreeSelection *selection_local;
	selection_local = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview_local));
	g_signal_connect(selection_local,"changed",G_CALLBACK(on_changed),(gpointer)&upload_file);



	/*create hbox_schedule*/
	hbox_schedule = gtk_hbox_new(FALSE,0);	
	gtk_widget_show(hbox_schedule);
	gtk_widget_set_usize(hbox_schedule,1200,50);
	gtk_box_pack_start((GtkBox*)vbox_main,hbox_schedule,TRUE,TRUE,0); 

	/*create hbox_interract*/
	hbox_interract = gtk_hbox_new(FALSE,0);
	gtk_widget_show(hbox_interract);
	gtk_widget_set_usize(hbox_interract,1200,300);
	gtk_box_pack_start((GtkBox*)vbox_main,hbox_interract,TRUE,TRUE,0);

	sw_interract = gtk_scrolled_window_new(NULL,NULL);
	gtk_widget_show(sw_interract);
	
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw_interract),GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_interract),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	gtk_container_set_border_width(GTK_CONTAINER(sw_interract),5);
	gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(sw_interract),GTK_CORNER_BOTTOM_LEFT);


	gtk_box_pack_start((GtkBox*)hbox_interract,sw_interract,TRUE,TRUE,0);

  	text_view_interract = gtk_text_view_new();
	gtk_widget_show(text_view_interract);
	gtk_text_view_set_editable((GtkTextView*)text_view_interract,FALSE);
	gtk_container_add(GTK_CONTAINER(sw_interract),text_view_interract);

	
	return main_window;
}
