/* Copyright (c) 2013
 *
 * fileutils.c
 *
 * function:ftp client file operation
 *
 */

#include "fileutils.h" 

int sum_files;
static gboolean first_open = TRUE;
file_info *fileinfo_head = NULL;
file_info *fileinfo_tail = NULL;

/*caculate the file num of a specified dir*/
gint total_filenum(struct dirent *d_next, DIR *dir)
{
	gint sum = 0;
	while(d_next != NULL)
	{
		sum++;
		d_next = readdir(dir);
	}
	return sum;
}

/*free the memory to store the struct File*/
void free_file(File **file,int sum_files)
{
	int i = 0;
	File *ptr = *file;
	for(i; i<sum_files; i++)
	{
		free((void*)(ptr->file_name));
		free((void*)(ptr->own_name));
		free((void*)(ptr->group_name));
		free((void*)(ptr->date));
		free((void*)(ptr->file_state));
		ptr++;
	}
	free(*file);
	*file = NULL;
}

/*allocate the memory to the Files inside*/
gboolean malloc_file( File *file, int sumfiles)
{
	int i = 0;
	for(i ; i<sumfiles; ++i)
	{
		file->file_name = (gchar*)malloc(MAXPATHLEN *sizeof(gchar));
		if(file->file_name == NULL)
		{
			return FALSE;
		}
		file->own_name = ( gchar*)malloc(MAXUSERLEN *sizeof(gchar));
		if(file->own_name == NULL)
		{
			return FALSE;
		}
		file->group_name = (gchar*)malloc(MAXUSERLEN *sizeof(gchar));
		if(file->group_name == NULL)
		{
			return FALSE;
		}
		file->date = (gchar*)malloc(20*sizeof( gchar));
		if(file->date == NULL)
		{
			return FALSE;
		}

		file->file_state = (gchar*)malloc(11*sizeof(gchar));
		if(file->file_state == NULL)
		{
			return FALSE;
		}
		file++;
	}
	return TRUE;
}


/*the first step to get the file info*/
gboolean get_file_info(char *file_name, File *local_file)
{
	struct stat s_buff;
	int status = stat(file_name,&s_buff);
	if(status != 0)
	{
		printf("stat error:%s\n",file_name);
		perror("stat:\n");
		return TRUE;
	}
	else return get_file_info_stat(file_name,&s_buff,local_file);
}

/*set the local files'value*/
gboolean get_file_info_stat(const char *file_name,struct stat *s_buff, File *local_file)
{
	char date[20];
	char mode[11] ="----------";
	if(s_buff == NULL || local_file == NULL)
	{
		printf("s_buff or local_file is NULL pointer:%s\n",file_name);
		return FALSE;
	}

	mode_t file_mode = s_buff->st_mode;
	struct passwd *passwd_info = getpwuid(s_buff->st_uid);
	if(passwd_info == NULL)
	{
		memset(&errno,0,sizeof(errno));
		printf("errno:%s",strerror(errno));
		printf("%s\n",file_name);
		return TRUE;
	}
	if(passwd_info != NULL)
	{
		struct group *group_info = getgrgid(s_buff->st_gid);
		if(group_info == NULL)
		{
			printf("getgrgid error:%s\n",file_name);
		}
		if(group_info != NULL)
		{
			/*file types*/
			if(S_ISDIR(file_mode))
				mode[0] = 'd';
			else if(S_ISREG(file_mode))
				mode[0] = 'r';
			else if(S_ISCHR(file_mode))
				mode[0] = 'c';
			else if(S_ISBLK(file_mode))
				mode[0] = 'b';
			else if(S_ISFIFO(file_mode))
				mode[0] = 'p';
			else if(S_ISLNK(file_mode))
				mode[0] = 'l';
			else if(S_ISSOCK(file_mode))
				mode[0] = 's';
			else
			{
				printf("file mode error:%s\n",file_name);
				return FALSE;
			}

			
			/*file state*/
			mode[1] = (file_mode & S_IRUSR) ? 'r' : '-';
			mode[2] = (file_mode & S_IWUSR) ? 'w' : '-';
			mode[3] = (file_mode & S_IXUSR) ? 'x' : '-';
			mode[4] = (file_mode & S_IRGRP) ? 'r' : '-';
			mode[5] = (file_mode & S_IWGRP) ? 'w' : '-';
			mode[6] = (file_mode & S_IXGRP) ? 'x' : '-';
			mode[7] = (file_mode & S_IROTH) ? 'r' : '-';
			mode[8] = (file_mode & S_IWOTH) ? 'w' : '-';
			mode[9] = (file_mode & S_IXOTH) ? 'x' : '-';
			
			strftime(date,20,"%b %d %H:%M",localtime(&(s_buff->st_mtime)));

			/*give the value to local_file pointer*/
			strcpy(local_file->file_name,file_name);
			local_file->file_size = (gint)s_buff->st_size;
			strcpy(local_file->own_name,passwd_info->pw_name);
			strcpy(local_file->group_name,group_info->gr_name);
			strcpy(local_file->date,date);
			strcpy(local_file->file_state,mode);
			return TRUE;

		}
			
	}
	return FALSE;
}

/*give the directory info to the list store*/
gboolean write_locallist(const char *current_dir, File **local_file)
{
	/*the num of files in directory*/
	gboolean flag;
	char new_filename[MAXPATHLEN];

	/*change current dir*/
	if(chdir(current_dir) == -1)
	{
		return FALSE; 
	}

	DIR *dir = opendir(current_dir);
	if(dir == NULL)
	{
		return FALSE;
	}

	struct dirent *first_file = readdir(dir);
	/*global variable sum_files's defination*/

	if(*local_file != NULL)
	{
		free_file(local_file,sum_files);
	}

	sum_files = total_filenum(first_file,dir) -1;
	if(sum_files == 0)
	{
		return FALSE;
	}

	*local_file = malloc((sum_files)*sizeof(File));
	File * ptr = *local_file;

	if(*local_file == NULL)
	{
		return FALSE;
	}
	/*allocate the memory to local_fils's inside*/
	flag = malloc_file(*local_file,sum_files);
	if(!flag)
	{
		return FALSE;
	}

	/*chage the dir location to the first file*/
	rewinddir(dir);

	

	/*start read the directory*/
	while(1)
	{
		struct dirent *d_next = readdir(dir);
		if(d_next == NULL)
		{
			break;
		}
		if(strcmp(d_next->d_name,".") == 0)
		{
			continue;
		}
		if(!get_file_info(d_next->d_name,ptr))
		{
			return FALSE;
		} ptr++;
	}
	return TRUE;
}

/*put the remote file info into remote_fiie*/
gboolean write_remotelist( File ** remote_file)
{
	gboolean flag;
	if(*remote_file != NULL)
	{
		free_file(remote_file,remote_filenum);
	}

	remote_filenum = 0;
	/*create the file info link*/
	parse_file_buf(filebuf_head);

	/*malloc the File memory*/
	*remote_file = malloc(remote_filenum*sizeof(File));
	if(*remote_file == NULL)
	{
		return FALSE;
	}
	File *f_ptr = *remote_file;

	flag = malloc_file(*remote_file,remote_filenum);
	if(!flag)
	{
		return FALSE;
	}


	file_info *fi_ptr = fileinfo_head->next;
	while(fi_ptr != NULL)
	{
		parse_file_info(fi_ptr,f_ptr);
		f_ptr++;
		fi_ptr = fi_ptr->next;
	}
	return TRUE;
}

/*free the file info*/
void free_remote_fileinfo(file_info **head)
{
	file_info *temp;
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

void print_package(file_buf *head)
{
	file_buf *ptr;
	ptr = head->next;
	while(ptr != NULL)
	{
		printf("%s\n",ptr->file_data);
		printf("\n");
		ptr = ptr->next;
	}
}

/*parse the file buf and assign the pags into file info link*/
void parse_file_buf(file_buf *filebuf_head)
{
	file_buf *ptr;
	if(filebuf_head != NULL)
	{
		ptr = (filebuf_head)->next;
	}
	else 
	{
		return;
	}
	if(fileinfo_head != NULL)
	{
		free_remote_fileinfo(&fileinfo_head);
	}

	fileinfo_head = malloc(sizeof(file_info));
	if(fileinfo_head == NULL)
	{
		return ;
	}
	fileinfo_tail = fileinfo_head;

	print_package(filebuf_head);
	char info[READBUFSIZE];
	char file_data[READBUFSIZE];
	char left[READBUFSIZE];

	memset(info,0,READBUFSIZE);
	memset(file_data,0,READBUFSIZE);
	memset(left,0,READBUFSIZE);

	while(ptr != NULL)
	{
		strcpy(info,ptr->file_data);	
		set_file_data_into_fileinfo(info,file_data,left,fileinfo_head,&fileinfo_tail);
		ptr = ptr->next;
	}
}

/*analyse the file buf and put the value into the file_info struct*/
void set_file_data_into_fileinfo(char *info, char *file_data, char *left,
			file_info *fileinfo_head, file_info **fileinfo_tail)
{
	file_info *fileinfo_new;
	int i = 0;
	int j = 0;
	int meet_r = 0;   
	int len;
	for( ;i<READBUFSIZE; i++)
	{
		if(info[i] != '\r' && info[i] != '\0' && info[i] != '\n')
		{
			file_data[j] = info[i];
			j++;
		}

		if(info[i] == '\r')
		{
			file_data[j] = '\0';
			meet_r++;
			if(meet_r == 1)
			{
				/*check whether there is something left in the former pag*/
				if(left[0] != '\0')		
				{
					strcat(left,file_data);
					len =strlen(left);
					left[len] = '\0';
					strcpy(file_data,left);
					file_data[len] = '\0';
				}
				memset(left,0,READBUFSIZE);
			}

			fileinfo_new = malloc(sizeof(file_info));
			if(fileinfo_new == NULL)
			{
				return;
			}

			/*initialize the file info struct*/
			strcpy(fileinfo_new->file_data,file_data);
			fileinfo_new->next = NULL;

			/*file_data go to the beginning*/
			j = 0;
			memset(file_data,0,READBUFSIZE);

			/*create the link*/
			if(fileinfo_head == *fileinfo_tail)
			{
				fileinfo_head->next = fileinfo_new;
				*fileinfo_tail = fileinfo_new;
			}
			else
			{
				(*fileinfo_tail)->next = fileinfo_new;
				*fileinfo_tail = fileinfo_new;
			}

			/*log the file num*/
			remote_filenum++;
		}

		/*meet the package end*/
		if(i == READBUFSIZE-1 && info[i] != '\0')
		{
			if(file_data[0] != '\0')
			{
				/*former package han's nothing left*/
				if(left[0] == '\0')
				{
					strcpy(left,file_data);
					len = strlen(left);
					left[len] = '\0';
				}
				else
				{
					printf("there must be something wrong");
				}
			}
		}

		if(info[i] == '\0')
		{
			break;
		}
	}
}

/*pare the file info throw the blanks*/
void set_data_into_file(int *j, int *i, char *dest,const char *line) 
{
	while(line[*i] == ' ' || line[*i] == '  ')
	{
		++(*i);
	}
	while(line[*i] !=  ' ' && line[*i] != '  ')
	{
		dest[*j] = line[*i];
		++(*j);
		++(*i);
	}
}

/*paser the file info*/
void parse_file_info(file_info *fi_ptr, File *f_ptr)
{
	char *result;
	char line[READBUFSIZE];
	int i = 0;
	int j = 0;
	char mode[11],link[5],own[10],group[10],
	     size[10],date[16],file_name[30];
	strcpy(line,fi_ptr->file_data);

	for(i = 0; i<10; ++i)
	{
		mode[i] = line[i];
	}
	mode[10] = '\0';

	/*file link */
	set_data_into_file(&j,&i,link,line);
	link[j] = '\0';
	j = 0;

	/*own name*/
	set_data_into_file(&j,&i,own,line);
	own[j] = '\0';
	j = 0;

	/*group name*/
	set_data_into_file(&j,&i,group,line);
	group[j] = '\0';
	j = 0;

	/*file size*/
	set_data_into_file(&j,&i,size,line);
	size[j] = '\0';
	j = 0;

	/*file date*/
	char month[5] = { 0 };
	char day[5] = { 0 };
	char day_time[10] = { 0 };

	set_data_into_file(&j,&i,month,line);
	j = 0;
	set_data_into_file(&j,&i,day,line);
	j = 0;
	set_data_into_file(&j,&i,day_time,line);
	j = 0;

	/*file name*/
	while(line[i++] == " ");
	while(line[i] != '\0')
	{
		file_name[j++] = line[i++];
	}
	file_name[j] = '\0';

	strcpy(f_ptr->file_name,file_name);

	f_ptr->file_size = atoi(size);

	strcpy(f_ptr->own_name,own);

	strcpy(f_ptr->group_name,group);

	sprintf(date,"%s %s %s",month,day,day_time);
	strcpy(f_ptr->date,date);

	strcpy(f_ptr->file_state,mode);

}
