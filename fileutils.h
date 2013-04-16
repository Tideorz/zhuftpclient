/* Copyright (c) 2013
 *
 * fileutils.h
 *
 * function:ftp client file operation
 *
 */

#include "defines.h"
#ifndef __FILEUTILS_H
#define __FILEUTILS_H

#ifdef __cplusplus
extern "C"
{
#endif

	gint total_filenum(struct dirent *d_next, DIR *dir);
	void free_file( File **file,int sum_files);
	gboolean malloc_file( File *file, int sum_files);
	gboolean get_file_info(char *file_name,  File *local_file);
	gboolean get_file_info_stat(const char *file_name,struct stat *s_buff, File *local_file);
	gboolean write_locallist(const char *current_dir,  File **local_file);
	gboolean write_remotelist( File ** remote_file);
	void set_file_data_into_fileinfo(char *info, char *file_data, char *left,file_info *fileinfo_head, file_info **fileinfo_tail);
	void parse_file_buf(file_buf *filebuf_head);
	void parse_file_info(file_info *fi_ptr, File *f_ptr);


#ifdef __cplusplus
}
#endif
#endif
