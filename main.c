/* Copyright (c)  2013 3.16*
 * zhuhaichao
 * 
 * file: main.c
 *
 * description:the main functions about the
 * gtk-UI 
 */

#include "ftpclient_UI.h"

int
main(int argc, char *argv[])
{
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(&argc, &argv);
	GtkWidget *main_window = create_window();
	gtk_widget_show(main_window);
	
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	return 0;

}

