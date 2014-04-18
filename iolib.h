/*
 *  External definitions for the Yalnix File System user interface
 */

#ifndef _iolib_h
#define _iolib_h

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  The values used for 'whence' on a Seek call:
 */
#define	SEEK_SET	0	/* Set position to 'offset' */
#define	SEEK_CUR	1	/* Set position to current plus 'offset' */
#define	SEEK_END	2	/* Set position to EOF plus 'offset' */

/*
 *  The structure used to return information on a Stat call:
 */
struct Stat {
    int inum;		/* inode number of file */
    int type;		/* type of file (e.g., INODE_REGULAR) */
    int size;		/* size of file in bytes */
    int nlink;		/* link count of file */
};

struct my_msg{
       int type; // 1 by default; after returning from server, if file exists, 0; if not exist -1;
       int data2; // not used for now.
       char data3[16];
       void *ptr;  // l->s: pathname; s->l: inode num and current position in file.
};

/* Server does not use this struct. Just place here to show my partner what file info are stored in library.*/
struct file_info  // this structure stores info representing an open or created file.
{
	int inode_num; // the file's inode number
	int cur_position; // the current position within the file. Initialized value is 0.
	char *fullpath; // fullpath of this file.
	void *readbuf;  // specifies the address of the buffer in the requesting process into which to perform the read.
	int readsize;  // the number of bytes to be read from the file.
	void *writebuf; // specifies the address of the buffer in the requesting process into which to perform the write.
	int writesize; //the number of bytes to be writen to the file.
	int filesize;  // the size of the file 
};

/* Specifically used for Stat method.*/
struct stat_struct
{
	struct stat_struct * statbuf;
	char *pathname;
};

/* Specifically used for Link and SymLink. */
struct old_new
{
	char *oldname;
	char *newname;
};

/* Specifically used for ReadLink. */
struct readlink
{
	char *pathname;
	char *buf;
	int len;
};

/*
 *  Function prototypes for YFS calls:
 */
extern int Open(char *);
extern int Close(int);
extern int Create(char *);
extern int Read(int, void *, int);
extern int Write(int, void *, int);
extern int Seek(int, int, int);
extern int Link(char *, char *);
extern int Unlink(char *);
extern int SymLink(char *, char *);
extern int ReadLink(char *, char *, int);
extern int MkDir(char *);
extern int RmDir(char *);
extern int ChDir(char *);
extern int Stat(char *, struct Stat *);
extern int Sync(void);
extern int Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* _iolib_h */
