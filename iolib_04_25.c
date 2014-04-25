#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iolib.h"
// <comp421/iolib.h>
#include <comp421/filesystem.h>
#include<comp421/yalnix.h>


struct file_info  // this structure stores info representing an open or created file.
{
	int inode_num; // the file's inode number
	int offset; // the current position within the file. Initialized value is 0.
	
};

/* Store open file's info.*/
struct file_info * files[MAX_OPEN_FILES];  // MAX_OPEN_FILES is the maxmum number of open files at a time.

/* Used to check if a file descriptor is valid. O invalide; 1 valid.*/
int track_fd[MAX_OPEN_FILES] ;// 

/* Utils */
void reduce_slash(char *pathname);
void concat_path(char *rootpath, char *pathname);

/* Global varables */
int current_inode_num = ROOTINODE;
unsigned int fd_counter;


/*
 *  Function prototypes for YFS calls:
 */
int find_fd(int fd)
{
	int i;
	for(i = 0; i<MAX_OPEN_FILES-1; i++)
	{
		/* find an available file descriptor number. */
		
		if(track_fd[i] == fd) 
		{
			return i;
		}
	}

	return -1;

}
extern int Open(char *pathname)
{
	/* Check if the length of pathname is legal.*/
	if(strlen(pathname) > (MAXPATHNAMELEN - 1))
		return ERROR;
	

	/* Create a fixed-length buffer to contain pathname for server's using. */
	char *path = (char *)malloc(MAXPATHNAMELEN);
	memcpy(path, pathname, strlen(pathname)+1);
	/* Tell there's available file descriptor number.*/
	int index = -1;
	index=find_fd(0);
	
	/* No available file descriptor number.*/
	if(index == -1)
	{ 	TtyPrintf(0,"\nERROR:Cannot Open the file! Already Opened %d Files",MAX_OPEN_FILES);

		return ERROR;
	}

	/* Process multiple slashes first */
	reduce_slash(path);

	/* Create msg to be sent to server.*/
	struct my_msg *msg_open;
	msg_open = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_open->type = OPEN;
	msg_open->data2 = current_inode_num;
	msg_open->ptr = (void *)path;

	/* Get result from file system. */
	/* if pid is positive, normal.*/
	/* if pid is negative, it is interpreted as the process ID of the process to which to send the message.*/
	int result;
	Send((void *)msg_open, -FILE_SERVER);
	if(msg_open->type ==0)
	{	
		struct file_info *file;
		file = (struct file_info *)malloc(sizeof(struct file_info));
		file->inode_num = (msg_open->data2); // inode number is from server.
		file->offset = 0; // be initialized as 0 when a file is open.

		files[index] = file;
		
		track_fd[index]=fd_counter++;
		printf("\nvalue of fd is %d with inode_num %d",track_fd[index],files[index]->inode_num);
		result= track_fd[index];
	}
	else
	{
		TtyPrintf(0,"\nERROR:File %s does not exist!",pathname);
		result= ERROR;
	}
	free(msg_open);
	return result;
}

extern int Close(int fd)
{
	/* fd is not valid*/
	int index=find_fd(fd);
	if(index== -1)
	{
		TtyPrintf(0,"\nERROR: Invalid File Descriptor");

		return ERROR;
	}
	/* if valid */
	else  
	{
		/* make it invalid.*/
		track_fd[index]=0;
		free(files[index]);
		return 0;
	}
}

extern int Create(char *pathname)
{	int i;
	/* Check if the length of pathname is legal.*/
	if(strlen(pathname) > (MAXPATHNAMELEN - 1))
		return ERROR;
	
	/* Create a fixed-length buffer to contain pathname for server's using. */
	
	char *path = (char *)malloc(MAXPATHNAMELEN);
	memcpy(path, pathname, strlen(pathname)+1);

	/* Tell there's available file descriptor number.*/
	int index = find_fd(0);
	printf("\nval of index is %d\n",index);
	/* No available file descriptor number.*/
	if(index == -1) 
	{
		return ERROR;
	}

	/* Process multiple slashes first */
	//reduce_slash(path);
	
	/* Create msg to be sent to server.*/
	struct my_msg *msg_create;
	msg_create = (struct my_msg *)malloc(sizeof(struct my_msg));
	msg_create->type = CREATE;
	msg_create->data2 = current_inode_num;
	msg_create->ptr = (void *)path;

	/* What the server should do is to check if the named file exsits. if yes, truncate it to 0 and open an 
	   empty file. If not, create a new file with the given name and assign it a descriptor number.*/

	int res=Send((void *)msg_create,FILE_SERVER);
	if(msg_create->type == 0)
	{
		struct file_info *file;
		file = (struct file_info *)malloc(sizeof(struct file_info));
		file->inode_num = msg_create->data2; // inode number is from server.
		file->offset = 0; // be initialized as 0 when a file is created.
		

		files[index] = file;
		track_fd[index] = ++fd_counter; // make this file desciptor number valid.
		return track_fd[index];
	}
	else
	{
		TtyPrintf(0,"\nERROR: File %s could not be created!",pathname);
		return ERROR;
	}

}

extern int Read(int fd, void *buf, int size)
{
	struct read_struct read_m;
	/* Create msg to be sent to server.*/
	struct my_msg *msg_read;
	char *temp_buf;
	int index = find_fd(fd);
	
	/* No match for provided file descriptor number.*/
	if(index == -1) 
	{
		TtyPrintf(0,"\nERROR: Invalid Fd %d",fd);
		return ERROR;
	}
	if(buf==NULL)
	{
		TtyPrintf(0,"\nERROR: Invalid memory pointer!");
		return ERROR;

	}
	msg_read = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_read->type = READ;
	msg_read->data2 = files[index]->inode_num;
	printf("\nassigned value is %d\n",msg_read->data2);
	msg_read->ptr = &read_m;
	
	temp_buf=(char *)malloc(size);						
	read_m.offset=files[index]->offset;
	read_m.len=size;
	read_m.buff=temp_buf;
	
	Send(msg_read,-FILE_SERVER);
	int result=msg_read->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nERROR: Read Unsuccessful!");
	}
	else
	{
		memcpy(buf,temp_buf,result);
		files[index]->offset+=msg_read->type;
	}
	free(temp_buf);
	free(msg_read);
	return result;
}

extern int Write(int fd, void *buf, int size)
{
	/* Create msg to be sent to server. */
	struct my_msg *msg_write;
	struct read_struct write_m;
	
	int index = find_fd(fd);
	
	/* No match for provided file descriptor number.*/
	if(index == -1) 
	{
		TtyPrintf(0,"\nERROR: Invalid Fd %d",fd);
		return ERROR;
	}
	if(buf==NULL)
	{
		TtyPrintf(0,"\nERROR: Invalid memory pointer!");
		return ERROR;

	}
	char *msg_buf=(char *)malloc(size*sizeof(char));
	msg_write = (struct my_msg *)malloc(sizeof(struct my_msg));
	memcpy(msg_buf,buf,strlen(buf));
	msg_write->type = WRITE;
	msg_write->data2 = files[index]->inode_num;
	//msg_write->ptr = (void *)files[index];
	msg_write->ptr = &write_m;
										
	write_m.offset=files[index]->offset;
	write_m.len=size;
	write_m.buff=msg_buf;


	Send((void *)msg_write,-FILE_SERVER); 
	int result=msg_write->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nWrite Unsuccessful");
	}
	else
	{	files[index]->offset+=msg_write->type;
		TtyPrintf(0,"\nWrite Successfully Completed!");
	}
	free(msg_buf);
	free(msg_write);
	return result;
}

extern int Seek(int fd, int offset, int whence)
{
	int index = find_fd(fd);
	struct seek_struct seek_msg;
	struct my_msg *msg;
	
	/* No match for provided file descriptor number.*/
	if(index == -1) 
	{
		TtyPrintf(0,"\nERROR: Invalid Fd %d",fd);
		return ERROR;
	}
	msg= (struct my_msg *)malloc(sizeof(struct my_msg));
	msg->type=SEEK;
	msg->data2=index;
	msg->ptr=&seek_msg;

	seek_msg.curr_offset=files[index]->offset;
	seek_msg.new_offset=offset;
	seek_msg.whence=whence;
	
	Send((void *)msg, -FILE_SERVER); 
	int result=msg->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nSeek Unsuccessful");
	}
	else
	{
		files[index]->offset=result;
	}
	
	free(msg);
	return result;
}

	

extern int Link(char *oldname, char *newname)
{
	if(oldname==NULL||newname==NULL)
		return ERROR;

	if(strlen(oldname) > (MAXPATHNAMELEN-1) || strlen(newname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Process multiple slashes first */
	reduce_slash(oldname);
	reduce_slash(newname);

	struct old_new *name;
	name = (struct old_new *)malloc(sizeof(struct old_new));

	memcpy(name->oldname, oldname, strlen(oldname));
	memcpy(name->newname, newname, strlen(newname));

	/* Create msg to be sent to server.*/
	struct my_msg *msg_link;
	msg_link = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_link->type = LINK;
	msg_link->data2 = current_inode_num;
	msg_link->ptr = name;

	Send((void *)msg_link, -FILE_SERVER);
	int result=msg_link->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nLink Unsuccessful!");
	}
	free(name);
	free(msg_link);

	return result;
}

extern int Unlink(char *pathname)
{
	if(pathname==NULL)
		return ERROR;

	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Process multiple slashes first */
	reduce_slash(pathname);

	/* Create a buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	/* Create msg to be sent to server.*/
	struct my_msg *msg_unlink;
	msg_unlink = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_unlink->type = UNLINK;
	msg_unlink->data2 = current_inode_num;
	msg_unlink->ptr = path;

	Send((void *)msg_unlink, -FILE_SERVER);
	
	int result=msg_unlink->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nUnLink Unsuccessful!");
	}
	free(msg_unlink);
	free(path);
	return result;

}

extern int SymLink(char *oldname, char *newname)
{
	struct old_new *name;
	if(oldname==NULL||newname==NULL)
		return ERROR;

	if(strlen(oldname) > (MAXPATHNAMELEN-1) || strlen(newname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Process multiple slashes first */
	reduce_slash(oldname);
	reduce_slash(newname);


	name = (struct old_new *)malloc(sizeof(struct old_new));

	memcpy(name->oldname, oldname, strlen(oldname));
	memcpy(name->newname, newname, strlen(newname));

	/* Create msg to be sent to server.*/
	struct my_msg *msg_symlink;
	msg_symlink = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_symlink->type = SYMLINK;
	msg_symlink->data2 = current_inode_num;
	msg_symlink->ptr = name;

	Send((void *)msg_symlink, -FILE_SERVER);
	int result=msg_symlink->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nSymLink Unsuccessful!");
	}
	free(msg_symlink);
	free(name);
	return result;
}

extern int ReadLink(char *pathname, char *buf, int len)
{
	if(pathname==NULL||buf==NULL)
		return ERROR;
	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;
	if(len==0)
		return 0;
	/* Create a fixed-size buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	struct readlink *new_read_link;
	new_read_link = (struct readlink *)malloc(sizeof(struct readlink));

	strcpy(new_read_link->pathname, path);
	char *read_buf=(char *)malloc(len);
	new_read_link->buf = read_buf;
	new_read_link->len = len;

	/* Create msg to be sent to server.*/
	struct my_msg *msg_readlink;
	msg_readlink = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_readlink->type = READLINK;
	msg_readlink->data2 = current_inode_num;
	msg_readlink->ptr = new_read_link;

	Send((void *)msg_readlink, -FILE_SERVER);
	int result=msg_readlink->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nUnLink Unsuccessful!");
	}
	else 
		memcpy(buf,read_buf,result);

	free(msg_readlink);
	free(read_buf);
	free(new_read_link);
	return result;

}

extern int MkDir(char *pathname)
{
	if(pathname==NULL)
		return ERROR;
	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Create a fixed-size buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	/* Create msg to be sent to server.*/
	struct my_msg *msg_mkdir;
	msg_mkdir = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_mkdir->type = MKDIR;
	msg_mkdir->data2 = current_inode_num;
	msg_mkdir->ptr = path;

	Send((void *)msg_mkdir, -FILE_SERVER);
	int result=msg_mkdir->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nMkDir Unsuccessful!");
	}
	free(msg_mkdir);
	return result;
}

extern int RmDir(char *pathname)
{
	if(pathname==NULL)
		return ERROR;

	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Create a fixed-size buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	/* Create msg to be sent to server. */
	struct my_msg *msg_rmdir;
	msg_rmdir = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_rmdir->type = RMDIR;
	msg_rmdir->data2 = current_inode_num;
	msg_rmdir->ptr = path;

	Send((void *)msg_rmdir, -FILE_SERVER);
	int result=msg_rmdir->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nRmDir Unsuccessful!");
	}
	free(msg_rmdir);

	return result;
}

extern int ChDir(char *pathname)
{
	if(pathname==NULL)
		return ERROR;

	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Create a fixed-size buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	/* The file pathname must be a directory. Send a msg to server to check if the pathname is valid.*/
	struct my_msg *msg_chdir;
	msg_chdir = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_chdir->type = CHDIR;
	msg_chdir->data2 = current_inode_num;
	msg_chdir->ptr = (void *)path;

	/* Result from file system. */
	Send((void *)msg_chdir, -FILE_SERVER);
	int result=msg_chdir->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nChDir Unsuccessful!");
	}
	else	if(result == 0)
	{
		
		current_inode_num = msg_chdir->data2;
		
	}

	free(msg_chdir);
	return result;
}

extern int Stat(char *pathname, struct Stat *statbuf)
{
	if(pathname==NULL||statbuf==NULL)
		return ERROR;

	if(strlen(pathname) > (MAXPATHNAMELEN-1))
		return ERROR;

	/* Create a fixed-size buffer for server's using. */
	char path[MAXPATHNAMELEN];
	memcpy(path, pathname, strlen(pathname));

	struct stat_struct *stat;
	stat = (struct stat_struct *)malloc(sizeof(struct stat_struct));

	stat->statbuf = statbuf;
	strcpy(stat->pathname , path);

	/* Create msg sent to file server. */
	struct my_msg *msg_stat;
	msg_stat = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_stat->type = STAT;
	msg_stat->data2 = current_inode_num;
	msg_stat->ptr = stat;

	Send((void *)msg_stat, -FILE_SERVER);
	int result=msg_stat->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nStat Unsuccessful!");
	}
	
	free(msg_stat);
	free(stat);

	return result;
}

extern int Sync(void)
{
	/* Create msg sent to file server. */
	struct my_msg *msg_sync;
	msg_sync = (struct my_msg *)malloc(sizeof(struct my_msg));

	msg_sync->type = SYNC;

	Send((void *)msg_sync, -FILE_SERVER);
		return 0; //Remove when sync is implemented
	int result=msg_sync->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nSync Unsuccessful!");
	}
	
	free(msg_sync);
	return result;
}

extern int Shutdown(void)
{
	/* Create msg sent to file server.*/
	struct my_msg *msg_shutdown;
	msg_shutdown = (struct my_msg *) malloc(sizeof(struct my_msg));

	msg_shutdown->type = SHUTDOWN;

	Send((void *)msg_shutdown, -FILE_SERVER);
	int result=msg_shutdown->type;
	if(result==ERROR)
	{
		TtyPrintf(0,"\nShutdown Unsuccessful!");
	}
	
	free(msg_shutdown);

	return result;
}

/* Utils */
void reduce_slash(char *pathname)
{

	if (pathname == NULL)
		return;  // the path is empty

	/* if path is not null */
	char *temp = pathname;
	char path[256] = "";
	char pre_char = *temp;
	char cur_char = *temp;
	int counter = 0;
	path[counter] = cur_char;
	printf("%c \n",cur_char);
	temp++;
	while(*temp != '\0')
	{
		cur_char = *temp;
		temp++;

		if(pre_char == '/' && cur_char == '/')
		{
			/* do nothing. */
		}
		else
		{
			pre_char = cur_char;
			printf("%c \n",cur_char);
			counter++;
			path[counter] = cur_char;
		}
	}
	path[counter] = cur_char;

	/* If the path is ended with "/", add a "."*/
	if(cur_char == '/')
	{
		counter++;
		path[counter] = '.';
	}
	counter++;
	path[counter] = '\0';

	memcpy(pathname, path, counter+1);
}

/* Get absolute path if given relative path. Result is still in pathname.*/
void concat_path(char *rootpath, char *pathname)
{
	size_t len1 = strlen(rootpath);
	size_t len2 = strlen(pathname);

	char *fullpath = malloc(len1 + len2 + 1);

	memcpy(fullpath, rootpath, len1);
	memcpy(fullpath+len1, pathname, len2+1);
	memcpy(pathname, fullpath, strlen(fullpath)+1);

	printf("%s \n", pathname);
}
