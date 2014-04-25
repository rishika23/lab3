#include <comp421/filesystem.h>
#include <stdio.h>
#include<stdlib.h>
#include <string.h>
#include <math.h>
#include <comp421/yalnix.h>
#include "iolib.h"

#define DIRECTORYSIZE sizeof(struct dir_entry)

int num_inodes;
int num_blocks;
int max_inode_num;
int max_block_num;


struct free_block_list{
	
	int blk_num;
	struct free_block_list *next;

	}*head_blklist;

struct free_inode_list{
	
	int inode_num;
	struct free_inode_list *next;
	
	}*head_inodelist;


int Server_Read(int inode,int len,int offset,char *buff);
int GetBlock(int blk_num,char *ptr);
int WriteBlock(int blk_num,char *ptr);

int LookUp(char *pathname,int inode_n,int num_sym_link);
int GetIndirectBlk(int blk_num,int index);
int PutIndirectBlk(int blk_num,int index,int val);

void print_inode(int inode_num,struct inode *pinode);
int GetInode(int inode_num,struct inode *curr_inode);
int write_inode(int inode_num,struct inode *curr_inode);

int SearchInDir(int in_num,char *name,int num_sym_link);

int PutInDir(int d_inode,int inode_num,char *name);	//Still need to implement when a new block is to be added in indirect block
int Is_Dir_Block_Empty(int blk_num);
int Delete_from_dir(int d_inode,int inode_num);
void truncate_file(struct inode *curr_inode);


int Server_Create(char *pathname,int inode_num);
int Server_Open(char *pathname,int inode_num);
int Server_MkDir(char *,int );
int Server_ChDir(char *pathname,int inode_num);
int Server_RmDir(char *pathname,int inode_num);
int Server_Read(int inode,int len,int offset,char *buff);
int Server_Write(int inode,int len,int offset,char *buff);
int Server_Seek(int inode_num,int curr_offset,int new_offset,int whence);
int Server_Stat(int inode_num,char *name, struct Stat *stat_msg);
int Server_Link(int inode_num,char *oldname,char *newname);
int Server_UnLink(int inum,char *name);
int Server_SymLink(int inode_num,char *oldname,char *newname);
int Server_ReadLink(int inum,char *pathname,int len,char *buff);

int Get_free_inode();
void Add_free_inode(int num);
void Add_free_blknum(int num);
int Get_free_blknum();

int Initialize_Server();


int main (int argc, char **argv)
{	
	

	char pname[MAXPATHNAMELEN];
	char *temp_buff;
	int pid;

	struct my_msg msg;
	struct read_struct msg_read;
	struct read_struct msg_write;
	struct stat_struct msg_stat;
	struct old_new msg_link;
	struct Stat res_stat;
	struct readlink msg_readlink;
	struct seek_struct msg_seek;

	printf("\nYFS: Started File Server");
	Initialize_Server();

	if (Fork() == 0) {
	printf("\nCHILD\n");
	pid=Exec(argv[1],argv);
	printf("\nval returned by exec is %d",pid);
	return 0;
    	}
    	else
	 printf("\nPARENT\n");

	while(1)
	{
		pid=Receive(&msg);
		//printf("\nInside while");
		switch(msg.type)
		{	case CREATE:		//for create,type=1,data2=inode of current dir,ptr=ptr to path
			{	printf("\nInside Server Create");

				CopyFrom(pid,(void *)pname,msg.ptr,MAXPATHNAMELEN);
				msg.data2=Server_Create(pname,msg.data2);
				printf("\nInside Server Returning %d",msg.data2);
				if(msg.data2!=ERROR)
					msg.type=0;
				else
					msg.type=ERROR;
				printf("\nInside Server Returning %d",msg.type);

				Reply(&msg,pid);
				break;
			}
			case OPEN:		//for Open,type=1,data2=inode of current dir,ptr=ptr to path
			{
				CopyFrom(pid,(void *)pname,msg.ptr,MAXPATHNAMELEN);
				msg.data2=Server_Open(pname,msg.data2);
				printf("\nVal returned by open is %d\n",msg.data2);
				if(msg.data2!=ERROR)
					msg.type=0;
				else
					msg.type=ERROR;

				printf("\nInside Server Returning %d\n",msg.type);
				Reply(&msg,pid);
				break;
			}
			case MKDIR:		//for MkDir,type=1,data2=inode of current dir,ptr=ptr to path
			{
				CopyFrom(pid,(void *)pname,msg.ptr,MAXPATHNAMELEN);
				msg.data2=Server_MkDir(pname,msg.data2);
				printf("\nInside Server MkDir with name as %s and curr inode %d\n",pname,msg.data2);
				if(msg.data2!=ERROR)
					msg.type=0;
				else
					msg.type=ERROR;

				Reply(&msg,pid);
				break;
			}
			case CHDIR:		//for ChDir,type=1,data2=inode of current dir,ptr=ptr to new dir
			{
				CopyFrom(pid,(void *)pname,msg.ptr,MAXPATHNAMELEN);
				printf("\nInside Server ChDir with name as %s and curr inode %d\n",pname,msg.data2);
				msg.data2=Server_ChDir(pname,msg.data2);
				if(msg.data2!=ERROR)
					msg.type=0;
				else
					msg.type=ERROR;
				Reply(&msg,pid);
				break;
			}
			case RMDIR:		//for ChDir,type=1,data2=inode of current dir,ptr=ptr to dir
			{
				CopyFrom(pid,(void *)pname,msg.ptr,MAXPATHNAMELEN);
				msg.type=Server_RmDir(pname,msg.data2);
				Reply((void *)&msg,pid);
				break;
			}
			case READ:
			{	
				CopyFrom(pid,(void *)&msg_read,msg.ptr,sizeof(struct read_struct));
				temp_buff=(char *)malloc(msg_read.len+1);
				msg.type=Server_Read(msg.data2,msg_read.len,msg_read.offset,temp_buff);
				CopyTo(pid,msg_read.buff,temp_buff,msg_read.len);
				printf("\nInside Server ReturningRead %d",msg.type);
				printf("\n");
				Reply((void *)&msg,pid);
				//free(temp_buff);
				break;
			}
			case WRITE:
			{	printf("\nInside write in while");
				CopyFrom(pid,(void *)&msg_write,msg.ptr,sizeof(struct read_struct));
				temp_buff=(char *)malloc(msg_write.len+1);
				temp_buff[msg_write.len]='\0';
				CopyFrom(pid,temp_buff,msg_write.buff,msg_write.len);
				printf("\nData for writing is %s and len =%d",temp_buff,msg_write.len);
				msg.type=Server_Write(msg.data2,msg_write.len,msg_write.offset,temp_buff);
				//
				//free(temp_buff);
				Reply((void *)&msg,pid);
				
				break;
			}
			case STAT:
			{
			
				CopyFrom(pid,(void *)&msg_stat,msg.ptr,sizeof(struct stat_struct));
				msg.type=Server_Stat(msg.data2,msg_stat.pathname,&res_stat);
				if(msg.type!=ERROR)
					CopyTo(pid,(void *)msg_stat.statbuf,(void *)&res_stat,sizeof(struct Stat));
				Reply(&msg,pid);
				break;
			}
			
			case LINK:
			{
				CopyFrom(pid,(void *)&msg_link,msg.ptr,sizeof(struct old_new));
				msg.data2=Server_Link(msg.data2,msg_link.oldname,msg_link.newname);
				if(msg.data2==ERROR)
					msg.type=ERROR;
				else
					msg.type=0;
				Reply(&msg,pid);
				break;
			}
			case UNLINK:
			{
				temp_buff=(char *)malloc(MAXPATHNAMELEN*sizeof(char));
				CopyFrom(pid,(void *)temp_buff,msg.ptr,MAXPATHNAMELEN);
				msg.type=Server_UnLink(msg.data2,temp_buff);
				Reply(&msg,pid);
				free(temp_buff);
				break;
			}
			case READLINK:
			{
				CopyFrom(pid,(void *)&msg_readlink,msg.ptr,sizeof(struct readlink));
				temp_buff=(char *)malloc(msg_readlink.len+1);
				msg.type=Server_ReadLink(msg.data2,msg_readlink.pathname,msg_readlink.len,temp_buff);
				CopyTo(pid,msg_readlink.buf,temp_buff,msg_readlink.len);
				Reply(&msg,pid);
				break;
			}
			case SYMLINK:
			{
				CopyFrom(pid,(void *)&msg_link,msg.ptr,sizeof(struct old_new));
				msg.data2=Server_SymLink(msg.data2,msg_link.oldname,msg_link.newname);
				if(msg.data2==ERROR)
					msg.type=ERROR;
				else
					msg.type=0;
				Reply(&msg,pid);
				break;
			}
			case SEEK:
			{
				CopyFrom(pid,(void *)&msg_seek,msg.ptr,sizeof(struct stat_struct));
				msg.type=Server_Seek(msg.data2,msg_seek.curr_offset,msg_seek.new_offset,msg_seek.whence);
				Reply(&msg,pid);
				break;
			}
			case SHUTDOWN:
			{	//Server_Sync();
				TtyPrintf(0,"\nShutting Down Yalnix File Server!");
				msg.type=0;
				Reply((void *)&msg,pid);
				Exit(0);
				break;
			}
			case SYNC:
			{
			Reply((void *)&msg,pid);
			//Server_sync();
			
			break;
			}
			default:
			{
				msg.type=ERROR;
				Reply(&msg,pid);
				break;
			}

		}
	Reply(&msg,pid);
	}
	printf("\nExiting Yfs");
	return 0;;
	
}
int Server_Seek(int inode_num,int curr_offset,int new_offset,int whence)
{
	struct inode curr_inode;
	int size;
	int res_offset;
	
	GetInode(inode_num,&curr_inode);
	size=curr_inode.size;
	switch(whence)
	{
		case SEEK_SET:
		{

			if(new_offset<0||(new_offset>size))
				return ERROR;
			res_offset=new_offset;
			break;
		}
		case SEEK_CUR:
		{
			res_offset=curr_offset+new_offset;
			if(res_offset<0 || res_offset>size)
				return ERROR;
			
			break;
		}
		case SEEK_END:
		{
			res_offset=size+new_offset;
			if(new_offset>0 || res_offset <0 )
				return ERROR;
			
			break;
		}
	}
	return res_offset;

}

int Server_ReadLink(int inum,char *pathname,int len,char *buff)
{
	int i,k,j;
	int len_read=0,res;
	int total_blk,blk_num;
	char str[BLOCKSIZE];
	struct inode curr_inode;
	int inode_num,size;
	
	if(inum<1||inum>max_inode_num||buff==NULL)
		return ERROR;

	
	printf("\nReadLink :Reached 1");
	inode_num=LookUp(pathname,inum,0);
	
	if((inode_num==ERROR)||(inode_num==0))
	{	printf("\nERROR: Path Not Found!");
		return ERROR;
	}

	res=GetInode(inode_num,&curr_inode);
	if(res==ERROR)
		return ERROR;
	if(curr_inode.type!=INODE_SYMLINK)
	{	printf("\nERROR: Given file is not a SymLink!");
		return ERROR;
	}

	
	
	printf("\nReadLink :Reached 2");
	
	total_blk=(curr_inode.size+BLOCKSIZE-1)/BLOCKSIZE;
	
	res=GetBlock(curr_inode.direct[0],&str);
	size=curr_inode.size;
	printf("\nBlock data is %s of size",str,size);
	
	if(len<size)
		len_read=len;
	else 	
		len_read=size;
	strncpy(buff,str,len_read);
	buff[len_read]=0;
	printf("\nData in buff %s\n",buff);
	return len_read;
	
}

int Server_SymLink(int inode_num,char *oldname,char *newname)
{
	int i,blk_num;
	int last_slash=-1;
	int num;
	int res;
	int resF;
	int len=strlen(newname);
	char dir_path[MAXPATHNAMELEN+1];
	struct inode curr_inode;
	char block[BLOCKSIZE];
	
	if(inode_num<1||inode_num>max_inode_num||newname==NULL)
		return ERROR;

	for(i=0;i<len;i++)
		if(newname[i]=='/')
			last_slash=i;
	
	
	printf("\nReached in SymLink 1");
	if(last_slash==0)
		res=ROOTINODE;
	else
		res=inode_num;
	if(last_slash>0)
	{	
		printf("\nSymLink: Reached in Link_file 2");
		
		strncpy(dir_path,newname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",dir_path);
			free(dir_path);
			return ERROR;
		}
	}	
		
	strncpy(dir_path,&newname[last_slash+1],len-last_slash-1);
	
	dir_path[len-last_slash-1]='\0';
	
	printf("\nSymLink : Value of dir_path is %s",dir_path);
	
	resF=LookUp(dir_path,res,0);
	
	printf("\nSymLink :Reached in Link_file 4 with resF as %d",resF);
	
	if(resF==ERROR)
	{
		printf("\nSymLink ERROR: Path %s does not exist ",dir_path);
		return ERROR;
	}
	if(resF!=0)
	{
		printf("\nSymLink ERROR: Pathname %s already exists ",newname);
		return ERROR;
	}
	resF=Get_free_inode();
	if(resF==0)
		return ERROR;
	
	//GetInode(resF,&curr_inode);
	curr_inode.type=INODE_SYMLINK;
	curr_inode.nlink=1;
	curr_inode.size=strlen(oldname);
	blk_num=Get_free_blknum();
	if(blk_num==0)
	{
		printf("\nSymLink ERROR: Not enough memory available!");
		Add_free_inode(resF);
		return ERROR;
	}

	curr_inode.direct[0]=blk_num;
	GetBlock(blk_num,&block);
	strncpy(&block,oldname,strlen(oldname)+1);
	
	WriteBlock(blk_num,&block);
	
	PutInDir(res,resF,dir_path);
	write_inode(resF,&curr_inode);
	curr_inode.size=0;
	GetInode(resF,&curr_inode);
	print_inode(resF,&curr_inode);
	printf("\nReturning from SymLink");
	return 0;



}

int Server_UnLink(int inum,char *name)
{	int inode_num,i;
	int dir_inum=inum;
	int last_slash=-1;
	struct inode curr_inode;
	char dir_path[MAXPATHNAMELEN];
	
	if(inum<1||inum>max_inode_num||name==NULL)
		return ERROR;

	inode_num=LookUp(name,inum,0);
	if((inode_num==ERROR)||(inode_num==0))
	{	printf("\nERROR: Path Not Found!");
		return ERROR;
	}

	GetInode(inode_num,&curr_inode);
	if(curr_inode.type==INODE_DIRECTORY)
	{	printf("\nERROR: Cannot Unlink a Directory!");
		return ERROR;
	}

	for(i=0;i<strlen(name);i++)
		if(name[i]=='/')
			last_slash=i;
	if(last_slash==-1)
		dir_inum=inum;
	else if (last_slash==0)
		dir_inum=ROOTINODE;
	else
	{
		strncpy(dir_path,name,last_slash);
		dir_path[last_slash]='\0';
		dir_inum=LookUp(dir_path,inum,0);
	}

	if(curr_inode.nlink>1)
	{
		curr_inode.nlink=curr_inode.nlink-1;
		
	}
	/*Else Delete the file inode and all the blocks allocated to it*/
	else 
	{
		truncate_file(&curr_inode);
		curr_inode.type=INODE_FREE;
		Add_free_inode(inode_num);
		
	}
	Delete_from_dir(dir_inum,inode_num);
	write_inode(inode_num,&curr_inode);
	return 0;

}

int Server_Link(int inode_num,char *oldname,char *newname)
{
	int i,inum;
	int last_slash=0;
	int num;
	int res;
	int resF;
	int len=strlen(newname);
	char *dir_path;
	struct inode curr_inode;
	struct inode dir_inode;
	
	if(inode_num<1||inode_num>max_inode_num)
		return ERROR;


	res=LookUp(oldname,inode_num,0);
	
	if((res==0)||(res==ERROR))	
		return ERROR;
	GetInode(res,&curr_inode);
	if((curr_inode.type==INODE_DIRECTORY)||(curr_inode.type==INODE_FREE))
	{	TtyPrintf(0,"\nERROR: Cannot Link To a Directory!");
		return ERROR;
	}

	inum=res;
	printf("\nReached Inside Link with len=%d\n",len);

	dir_path=(char *)malloc(len+1);

	for(i=0;i<len;i++)
		if(newname[i]=='/')
			last_slash=i;
	

	printf("\nReached in Link_file 1");
	res=inode_num;

	if(last_slash!=0)
	{	
		printf("\nReached in Link_file 2");
		
		strncpy(dir_path,newname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",dir_path);
			free(dir_path);
			return ERROR;
		}
	}	
	printf("\nReached in Link_file 3 with dinode=%d",res);
	
	strncpy(dir_path,&newname[last_slash+1],len-last_slash-1);
	
	dir_path[len-last_slash-1]=0;
	
	printf("\nValue of dir_path is %s",dir_path);
	
	resF=LookUp(dir_path,res,0);
	
	printf("\nReached in Link_file 4 with resF as %d",resF);
	
	if(resF==ERROR)
	{
		printf("\nERROR: Path %s does not exist ",dir_path);
		free(dir_path);
		return ERROR;
	}
	if(resF!=0)
	{
		printf("\nERROR: Pathname %s already exists ",newname);
		free(dir_path);
		return ERROR;
	}

	PutInDir(res,inum,dir_path);
	curr_inode.nlink+=1;
	write_inode(inum,&curr_inode);
	free(dir_path);
	return inum;;
}

int Server_Stat(int inode_num,char *name, struct Stat *stat_msg)
{

	struct inode curr_inode;
	int res;
	
	
	printf("\nStat Reached with pathname =%s and inoenum= %d",name,inode_num);
	
	if(inode_num<1||inode_num>max_inode_num||name==NULL)
		return ERROR;
	
	res=LookUp(name,inode_num,0);
	printf("\nServer_Stat: got value %d",res);
	if((res==0)||(res==ERROR))	
		return ERROR;
	GetInode(res,&curr_inode);
	stat_msg->inum=res;		
    	stat_msg->type=curr_inode.type;		
    	stat_msg->size=curr_inode.size;		
    	stat_msg->nlink=curr_inode.nlink;
	
	return 0;
}
int Server_Write(int inode,int len,int offset,char *buff)
{
	int size,blk_num,sec_num,i,k,j;
	int len_write=0,res;
	int total_blk,num_blk;
	int blk_offset;
	char str[BLOCKSIZE];
	struct inode curr_inode;
	int final_size,final_total_blk;
	printf("\nWrite:Reached 1");

	if(inode<1||inode>max_inode_num||buff==NULL)
		return ERROR;

	
	printf("\nWrite:Reached 1 with inode=%d",inode);
	
	res=GetInode(inode,&curr_inode);
	
	if(res==ERROR)
		return ERROR;
	
	printf("\nWrite :Reached 2 with inode type as %d",curr_inode.type);
	
	if(curr_inode.type!=INODE_REGULAR)			
		return ERROR;
	
	printf("\nWrite :Reached 3");
	
	size=curr_inode.size;
	printf("\nWrite :Reached 3");

	if(offset>size+1)
		return ERROR;
	print_inode(inode,&curr_inode);
	
	
	printf("\nWrite :Reached 4 size=%d",size);
	
	
	
	total_blk=(size+BLOCKSIZE-1)/BLOCKSIZE;
	
	final_size=size+len;
	final_total_blk=(final_size+BLOCKSIZE-1)/BLOCKSIZE;
	printf("\nWrite :Reached 5 final block =%d and total blk =%d",final_total_blk,total_blk);
	/*res=Has_N_Free_Blocks(final_total_blk-total_blk);
	if(res==0)
		return ERROR;*/
	for(i=total_blk;i<final_total_blk;i++)
	{
		blk_num=Get_free_blknum();
		if(i<NUM_DIRECT)
			curr_inode.direct[i]=blk_num;
		else
			PutIndirectBlk(curr_inode.indirect,i-NUM_DIRECT,blk_num);

	}
	printf("\nWrite :Reached 5 ");
	num_blk=offset/BLOCKSIZE;
	for(k=num_blk;k<final_total_blk;k++)
	{	
		if(k<NUM_DIRECT)
				blk_num=curr_inode.direct[k];
			else
				blk_num=GetIndirectBlk(curr_inode.indirect,k-NUM_DIRECT);
		
		if(k==num_blk)
		{	blk_offset=offset%BLOCKSIZE;
			res=GetBlock(blk_num,&str);
			if(res==ERROR)
				return ERROR;
		}
		else
			blk_offset=0;
		
		for(i=blk_offset;i<BLOCKSIZE/sizeof(char);i++)
			if(len_write<len)
			{	str[i]=buff[len_write++];
				curr_inode.size++;
				printf("\nCopied %c",buff[len_write-1]);	
			}
			else 
			{	WriteBlock(blk_num,&str);
				write_inode(inode,&curr_inode);
				printf("\nReturning from Write()");
				return len_write;
			}
		WriteBlock(blk_num,&str);

	}
	write_inode(inode,&curr_inode);
	return len_write;
}
int Server_Read(int inode,int len,int offset,char *buff)
{
	int size,blk_num,sec_num,i,k,j;
	int len_read=0,res;
	int total_blk,num_blk;
	int blk_offset;
	char str[BLOCKSIZE];
	struct inode curr_inode;
	printf("\nRead :Reached 0");

	if(inode<1||inode>max_inode_num)
	{	printf("\nERROR: Invalid Inode Number %d",inode);
		return ERROR;
	}

	
	printf("\nRead :Reached 1");
	
	res=GetInode(inode,&curr_inode);
	if(res==ERROR)
		return ERROR;
	
	printf("\nRead :Reached 2");
	
	if((curr_inode.type!=INODE_REGULAR)&&(curr_inode.type!=INODE_DIRECTORY))			
		return ERROR;
	printf("\nRead :Reached 3");
	
	size=curr_inode.size;
	print_inode(inode,&curr_inode);
	if(offset>size)
	{
		printf("\noffset is more than file size");
		return ERROR;
	}
	
	printf("\nRead :Reached 4 size=%d",size);
	num_blk=offset/BLOCKSIZE;
	total_blk=(size+BLOCKSIZE-1)/BLOCKSIZE;
	
	
	printf("\nRead :Reached 5 ");
	
	for(k=num_blk;k<total_blk;k++)
	{	
		if(k<NUM_DIRECT)
				blk_num=curr_inode.direct[k];
			else
				blk_num=GetIndirectBlk(curr_inode.indirect,k-NUM_DIRECT);
		res=GetBlock(blk_num,&str);
		if(res==ERROR)
			return ERROR;
		if(k==num_blk)
			blk_offset=offset%BLOCKSIZE;
		else
			blk_offset=0;
		
		for(i=blk_offset;i<BLOCKSIZE/sizeof(char);i++)
			if((len_read<len)&&(len_read+offset<size))
			{	buff[len_read++]=str[i];
				printf("\nCopied %x",buff[len_read-1]);
					
			}
			else 
			{	buff[len_read]='\0';
				printf("\nReturning from Read()with buff=%s\n",buff);
				return len_read;
			}
	}
	return len_read;
}
int GetBlock(int blk_num,char *ptr)
{
	int sec_num,len=0;
	int j,k;
	char *dir;


	if(blk_num<0 || blk_num>max_block_num||ptr==NULL)
		return ERROR;
	dir=(char *) malloc(SECTORSIZE*sizeof(char));


	sec_num=(blk_num*BLOCKSIZE)/SECTORSIZE;
		
	for(j=0;j<BLOCKSIZE/SECTORSIZE;j++)
	{	
		ReadSector(sec_num,(void *)dir);

		for(k=0;k<(SECTORSIZE/sizeof(char));k++)
		{	
			ptr[len++]=dir[k];
		}
		sec_num++;
	}
	return (dir);
	return len;
}
int WriteBlock(int blk_num,char *ptr)
{
	int sec_num,len=0;
	int j,k;
	char *dir;


	if(blk_num<0 || blk_num>max_block_num||ptr==NULL)
		return ERROR;
	dir=(char *) malloc(SECTORSIZE*sizeof(char));


	sec_num=(blk_num*BLOCKSIZE)/SECTORSIZE;
		
	for(j=0;j<BLOCKSIZE/SECTORSIZE;j++)
	{	
		
		for(k=0;k<(SECTORSIZE/sizeof(char));k++)
		{	
			dir[k]=ptr[len++];
		}
		WriteSector(sec_num,(void *)dir);

		sec_num++;
	}
	return (dir);
	return len;
}

int LookUp(char *pathname,int inode_n,int num_sym_link)
{
	
	printf("\nReached Inside LookUp 1\n");
	
	int len=strlen(pathname);
	
	char dir_name[MAXPATHNAMELEN];
	
	int i=0,j=0,inode_num=inode_n ,res;
	
	struct inode curr_inode;
	
	int dir_length;
	
	printf("Reached Inside LookUp 2\n");
	
	if(num_sym_link>MAXSYMLINKS)
		return ERROR;
	if(pathname[0]=='/')
	{	j=1;
		i=1;
		inode_num=ROOTINODE;
	}
	
	
	
	while(j<len)
	{
		printf("Reached inside LookUp 3\n");

		while(i<len)
		{	if(pathname[i]=="/")
				break;
			
			i++;
				
		}
		
		printf("Reached inside Loopkp 3.5\n");
		
		strncpy(dir_name,&pathname[j],i-j);
		
		dir_name[i-j]='\0';

		printf("\nReached inside LookUp 4 with dir_name =%s",dir_name);
		GetInode(inode_num,&curr_inode);
		
		res=SearchInDir(inode_num,dir_name,num_sym_link);
		if((res==0)&&(i==len))					//case of a file
			return res;
		
		if((res==ERROR)||(res==0))
			return ERROR;
		if((i==len-2)&&(pathname[len-1]=='/'))		//case of a directory
			return res;
		if(i==len)
			return res;
		inode_num=res;
		
		i++;
		j=i;
			
		printf("Reached inside Lookup 5\n");
	}
	return inode_num;
	
	printf("Leaving LookUp\n");
}

int Server_RmDir(char *pathname,int inode_num)
{
	char str[3];
	int i;
	int last_slash=-1;
	int num;
	int res=ROOTINODE;
	int resF;
	int len=strlen(pathname);
	char *dir_path;
	struct inode curr_inode;
	int count=0;
	int blk_num,total_blocks;
	char str2[]=".";
	char str1[]="..";

	
	if(inode_num<1||inode_num>max_inode_num||pathname==NULL)
		return ERROR;

	
	for(i=0;i<len;i++)			//I am assuming that pathname can never end with a "/". check in library
		if(pathname[i]=='/')
			last_slash=i;

	dir_path=(char *)malloc (sizeof(char)*len+1);
	if(last_slash==0)
		res=ROOTINODE;
	else
		res=inode_num;
	if(last_slash>0)
	{	
		printf("\nReached in RmDir 2");
		
		strncpy(dir_path,pathname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",pathname);
			free(dir_path);
			return ERROR;
		}
	}
	
	printf("\nReached in RmDir 3 with dinode=%d",res);
	
	strncpy(dir_path,&pathname[last_slash+1],len-last_slash-1);
	

	dir_path[len-last_slash-1]='\0';
	printf("\nValue of dir_path is %s",dir_path);
	if((!strcmp(dir_path,str1))||(!strcmp(str2,dir_path)))
	{
			printf("\nERROR: Cannot Delete that directory ");
			free(dir_path);
			return ERROR;
	}

	printf("\nValue of dir_path is %s",dir_path);
	
	resF=LookUp(dir_path,res,0);
	
	printf("\nReached in MkDir 4 with resF as %d",resF);
	free(dir_path);
	if((resF==ERROR)||(resF==0))
	{
		printf("\nERROR: Path %s does not exist.",pathname);
		return ERROR;
	}
	
	GetInode(resF,&curr_inode);
	if(curr_inode.type!=INODE_DIRECTORY)
	{
		printf("\nERROR: Path %s does not exist!",pathname);
		return ERROR;
	}
	if(curr_inode.nlink>2)
	{
		printf("\nERROR:Directory is not empty!");
		return ERROR;
	}

	
		/*Actually Deleting the directory blocks*/

		total_blocks=(curr_inode.size+BLOCKSIZE-1)/BLOCKSIZE;
		for(i=0;i<total_blocks;i++)
		{
			if(i<NUM_DIRECT)
				blk_num=curr_inode.direct[i];
			else
				blk_num=GetIndirectBlk(curr_inode.indirect,i-NUM_DIRECT);
			count+=Is_Dir_Block_Empty(blk_num);
		}
		if(count==0)
		{
			for(i=0;i<total_blocks;i++)
			{
				if(i<NUM_DIRECT)
					blk_num=curr_inode.direct[i];
				else
					blk_num=GetIndirectBlk(curr_inode.indirect,i-NUM_DIRECT);
				Add_free_blknum(blk_num);
			}
			Delete_from_dir(res,resF);
			Add_free_inode(resF);
			curr_inode.type=INODE_FREE;
			write_inode(resF,&curr_inode);

		}

		printf("\nReturning from RmDir");

		return count;
}

int Server_ChDir(char *pathname,int inode_num)
{
	char str[3];
	int i;
	int last_slash=-1;
	int num;
	int res=ROOTINODE;
	int resF;
	int len=strlen(pathname);
	char *dir_path;
	struct inode *curr_inode=NULL;
	

	
	if(inode_num<1||inode_num>max_inode_num||pathname==NULL)
		return ERROR;

	
	for(i=0;i<len;i++)			//I am assuming that pathname can never end with a "/". check in library
		if(pathname[i]=='/')
			last_slash=i;
	res=inode_num;
	dir_path=(char *)malloc (sizeof(char)*len);
	if(last_slash==-1)
		res=inode_num;
	else if(last_slash==0)
		res=ROOTINODE;
		
	if(last_slash>0)
	{	
		printf("\nReached in ChDir 2");
		
		strncpy(dir_path,pathname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",pathname);
			free(dir_path);
			return ERROR;
		}
	}
	
	printf("\nReached in ChDir 3 with dinode=%d",res);
	
	strncpy(dir_path,&pathname[last_slash+1],len-last_slash-1);
	printf("\nValue of dir_path is %s",dir_path);

	dir_path[len-last_slash-1]='\0';
	
	printf("\nValue of dir_path is %s",dir_path);
	
	resF=LookUp(dir_path,res,0);
	
	printf("\nReached in ChDir 4 with resF as %d",resF);
	
	if((resF==ERROR)||(resF==0))
	{
		printf("\nERROR: Path %s does not exist.",pathname);
		free(dir_path);
		return ERROR;
	}
	curr_inode=(struct inode *)malloc(INODESIZE);
	GetInode(resF,curr_inode);
	if(curr_inode->type!=INODE_DIRECTORY)
	{
		printf("\nERROR: Path %s does not exist.",pathname);
		free(dir_path);
		free(curr_inode);
		return ERROR;
	}
		free(dir_path);
		free(curr_inode);
		printf("\nReturning from ChDir");

		return resF;

}


int Server_MkDir(char *pathname,int inode_num)
{
	char str[3];
	int i;
	int last_slash=-1;
	int num;
	int res,dinode_num;
	int resF;
	int len=strlen(pathname);
	char *dir_path;
	struct inode *curr_inode=NULL;
	

	
	if(inode_num<1||inode_num>max_inode_num||pathname==NULL)
		return ERROR;

	
	for(i=0;i<len;i++)			//I am assuming that pathname can never end with a "/". check in library
		if(pathname[i]=='/')
			last_slash=i;
	res=inode_num;
	dir_path=(char *)malloc (sizeof(char)*len);
	if(last_slash>0)
	{	
		printf("\nReached in MkDir 2");
		
		strncpy(dir_path,pathname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",dir_path);
			free(dir_path);
			return ERROR;
		}
		inode_num=res;
	}
	else if(last_slash==0)
		res=ROOTINODE;
	
	printf("\nReached in MkDir 3 with dinode=%d",res);
	
	strncpy(dir_path,&pathname[last_slash+1],len-last_slash-1);
	printf("\nValue of dir_path is %s",dir_path);

	dir_path[len-last_slash-1]='\0';
	
	printf("\nValue of dir_path is %s and inode_num %d",dir_path,inode_num);
	
	resF=LookUp(dir_path,inode_num,0);
	
	printf("\nReached in MkDir 4 with resF as %d",resF);
	
	if(resF==ERROR)
	{
		printf("\nERROR: Path %s does not exist.",dir_path);
		free(dir_path);
		return ERROR;
	}
	if(resF!=0)
	{
		printf("\nERROR: %s File/Directory already exists.",dir_path);
		free(dir_path);
		return ERROR;
	}


	
	printf("\nTesting :Reached here!");
	inode_num=Get_free_inode();
	if(inode_num==0)
	{
		printf("\nERROR: Not enough Inodes left for creating Directory");
		Wait(50);
		free(dir_path);
		return ERROR;
	}
	printf("\nTesting :Reached here 2 with new inode_num %d!",inode_num);
	
	curr_inode=(struct inode *)malloc(INODESIZE);
	
	curr_inode->type=INODE_DIRECTORY;
	curr_inode->size=0;
	curr_inode->nlink=2;

	printf("\nValue of blk num returned is %d",curr_inode->direct[0]);

	write_inode(inode_num,curr_inode);
		
	printf("\nValue of inode_num is %d",inode_num);
	printf("\nValue of pathname is %s",dir_path);
	
	/*making a directory entry in current directory*/
	printf("\nPutting %s with inum %d in dir %d",dir_path,inode_num,res);
	
	PutInDir(res,inode_num,dir_path);	
	
	str[0]='.';
	str[1]='\0';

	PutInDir(inode_num,inode_num,str);
	str[1]='.';
	str[2]='\0';
	PutInDir(inode_num,res,str);	
	
	GetInode(res,curr_inode);		
	curr_inode->nlink+=1;
	write_inode(res,curr_inode);
	printf("\nNew Size is %d and size of directory size is %d",curr_inode->size,DIRECTORYSIZE);
	free(curr_inode);
	free(dir_path);
	printf("Leaving MkDir\n");
	return inode_num;

	
}


int Server_Open(char *pathname,int inode_num)
{

	int res;
	char block[BLOCKSIZE];
	int sym_links=0;
	struct inode curr_inode;
	printf("\nOpen_file Reached with pathname =%s and inoenum= %d",pathname,inode_num);
	if(inode_num<1||inode_num>max_inode_num||pathname==NULL)
		return ERROR;
	res=inode_num;
	while(sym_links<MAXSYMLINKS)
	{
		res=LookUp(pathname,res,sym_links);
		printf("\nServer_Open: got value %d",res);
		if((res==0)||(res==ERROR))	
			return ERROR;
		GetInode(res,&curr_inode);
		if(curr_inode.type!=INODE_SYMLINK)
			break;
		GetBlock(curr_inode.direct[0],&block);
		strncpy(pathname,&block,MAXPATHNAMELEN);

	}
	printf("\nOpen_file Returning %d",res);
	
	return res;


}


int Server_Create(char *pathname,int inode_num)
{
	
	int i;
	int last_slash=-1;
	int num;
	int res=ROOTINODE;
	int resF;
	int len=strlen(pathname);
	char *dir_path;
	struct inode *curr_inode;
	struct inode *dir_inode;
	
	if(inode_num<1||inode_num>max_inode_num||pathname==NULL)
		return ERROR;

	printf("\nReached Inside Create_file with len=%d\n",len);

	dir_path=(char *)malloc(len+1);

	for(i=0;i<len;i++)
		if(pathname[i]=='/')
			last_slash=i;
	

	printf("\nReached in Create_file 1");


	if(last_slash>0)
	{	
		printf("\nReached in Create_file 2");
		
		strncpy(dir_path,pathname,last_slash);
		
		dir_path[last_slash]='\0';
		
		res=LookUp(dir_path,inode_num,0);
		
		if((res==ERROR)||(res==0))
		{
			printf("\nERROR: Path %s does not exist ",dir_path);
			free(dir_path);
			return ERROR;
		}
	}
	
	printf("\nReached in Create_file 3 with dinode=%d",res);
	
	strncpy(dir_path,&pathname[last_slash+1],len-last_slash-1);
	
	dir_path[len-last_slash-1]=0;
	
	printf("\nValue of dir_path is %s",dir_path);
	
	resF=LookUp(dir_path,res,0);
	
	printf("\nReached in Create_file 4 with resF as %d",resF);
	
	if(resF==ERROR)
	{
		printf("\nERROR: Path %s does not exist ",dir_path);
		free(dir_path);
		return ERROR;
	}

	curr_inode=(struct inode *)malloc(INODESIZE);
	
	if(resF==0)
	{
		inode_num=Get_free_inode();
		
		if(inode_num==0)
		{
			printf("\nERROR: Not enough Inodes left for creating file");
			free(curr_inode);
			return ERROR;
		}
		
		curr_inode->type=INODE_REGULAR;
		curr_inode->size=0;
		curr_inode->nlink=1;

		write_inode(inode_num,curr_inode);
		
		printf("\nValue of inode_num is %d",inode_num);
		printf("\nValue of pathname is %s",pathname);
		
		/*making a directory entry in current directory*/
		PutInDir(res,inode_num,dir_path);	
			
		
		free(curr_inode);
		
		printf("Leaving Create_file\n");

		return inode_num;
	}
	
	GetInode(resF,curr_inode);
	if(curr_inode->type!=INODE_REGULAR)
	{
		free(curr_inode);
		free(dir_path);
		printf("\nval od file type is %d",curr_inode->type);
		printf("\nERROR : Cannot Create the file!!\nLeaving Create File");

		return ERROR;
	
	}
	truncate_file(curr_inode);
	write_inode(resF,curr_inode);
	free(curr_inode);
	free(dir_path);

	printf("Leaving Create_file\n");
	return resF;
	

}

void truncate_file(struct inode *curr_inode)
{
	int size,total_blocks,blk_num,i;
	//size=curr_inode->size;
	printf("\nReached Inside Truncate_File");
	size=(curr_inode->size + BLOCKSIZE - 1) & ~(BLOCKSIZE - 1);
	total_blocks=size/BLOCKSIZE;
	for(i=0;i<total_blocks;i++)
	{
		if(i<NUM_DIRECT)
			blk_num=curr_inode->direct[i];
		else
			blk_num=GetIndirectBlk(curr_inode->indirect,i-NUM_DIRECT);
		Add_free_blknum(blk_num);
		if((i==total_blocks-1)&&(i>=NUM_DIRECT))
		{
			blk_num=curr_inode->indirect;
			Add_free_blknum(blk_num);
		}
	}
	curr_inode->size=0;
	printf("\nReached end of Truncate_File");
}

int Delete_from_dir(int d_inode,int inode_num)
{	
		
	int i,k,p,res;
	int blk_num,total_blks;
	struct dir_entry * dir;
	struct inode dir_inode;
	dir=(struct dir_entry * )malloc(BLOCKSIZE);
	
	res=GetInode(d_inode,&dir_inode);
	
	total_blks=(dir_inode.size+BLOCKSIZE-1)/BLOCKSIZE;
	
	for(i=0;i<total_blks;i++)
	{
		if(i<NUM_DIRECT)
			blk_num=dir_inode.direct[i];
		else
			blk_num=GetIndirectBlk(dir_inode.indirect,i-NUM_DIRECT);
		res=GetBlock(blk_num,(char *)dir);
		if(res==ERROR)	
			return ERROR;
			
		for(k=0;k<(BLOCKSIZE/DIRECTORYSIZE);k++)
			{	
				if(dir[k].inum==inode_num)
				{	dir[k].inum=0;						
					WriteBlock(blk_num,dir);
					Add_free_inode(inode_num);
					free(dir);
					
					return 0 ;
				}
			}
	}
	
	free(dir);
	return 0;
 }

int Is_Dir_Block_Empty(int blk_num)
{
	int k,p,res;
	int count=0;
	char *str1=".";
	char *str2="..";
	char dname[DIRNAMELEN+1];
	struct dir_entry *dir=(struct dir_entry *)malloc(BLOCKSIZE);
		
	res=GetBlock(blk_num,(char *)dir);
	if(res==ERROR)
		return ERROR;
	for(k=0;k<(BLOCKSIZE/DIRECTORYSIZE);k++)
	{	
		if(dir[k].inum!=0)
		{		
			for(p=0;p<DIRNAMELEN;p++)
				dname[p]=dir[k].name[p];
			dname[p]='\0';
			if((strcmp(dname,str1))&&(strcmp(dname,str2)))
			{	printf("\nIncrementing count due to %s",dname);
				count++;
			}
		}
	}
	
	free(dir);
	printf("\nIs_Dir_Block_EMPty returing %d",count);
	return count;
}

int PutInDir(int d_inode,int inode_num,char *name)
{	
	
	int i,k,p,res;
	int blk_num,total_blks;
	struct dir_entry * dir;
	struct inode *dir_inode=(struct inode *)malloc(INODESIZE);

	printf("\nInside PutInDir : Reached 0");
	
	if(dir_inode==NULL)
	{	
		printf("\nPutInDir ERROR : Not enough memory %p",dir_inode);
		return ERROR;
	}
	
	printf("\nInside PutInDir : Reached 0.5");
	res=GetInode(d_inode,dir_inode);
	
	if(res==ERROR||dir_inode->type!=INODE_DIRECTORY)
	{	
		printf("\nERROR: Coud not get node");
		free(dir_inode);
		return ERROR;
	}
	printf("\nInside PutInDir : Reached 1 with size as %d",dir_inode->size);
	dir=(struct dir_entry *)malloc(BLOCKSIZE);
	
	total_blks=(dir_inode->size+BLOCKSIZE-1/BLOCKSIZE);
	
	for(i=0;i<total_blks;i++)
	{
		if(i<NUM_DIRECT)
			blk_num=dir_inode->direct[i];
		else
			blk_num=GetIndirectBlk(dir_inode->indirect,i-NUM_DIRECT);
		res=GetBlock(blk_num,(char *)dir);
		if(res==ERROR)
			return res;
		for(k=0;k<(BLOCKSIZE/DIRECTORYSIZE);k++)
			{	
				if(dir[k].inum==0)
				{		
					dir[k].inum=inode_num;
					for(p=0;p<DIRNAMELEN;p++)
						if(name[p]=='\0')
							break;
						else
							dir[k].name[p]=name[p];		
					if(p<DIRNAMELEN)
						dir[k].name[p]='\0';
					

					printf("\nInside PutInDir : putting (%s)%d in %d  ",name,inode_num,d_inode);
					
					dir_inode->size+=DIRECTORYSIZE;
					printf("\nInside PutInDir : Reached inside loop with size as %d",dir_inode->size);
					write_inode(d_inode,dir_inode);
					WriteBlock(blk_num,(char *)dir);
					free(dir);
					free(dir_inode);
					return 0;

				}
			}
			
	}
	
	blk_num=Get_free_blknum();
	
	for(k=0;k<(BLOCKSIZE/DIRECTORYSIZE);k++)
		dir[k].inum=0;


	dir[0].inum=inode_num;
	for(p=0;p<DIRNAMELEN;p++)
		if(name[p]=='\0')
			break;
		else
			dir[k].name[p]=name[p];		
	if(p<DIRNAMELEN)
		dir[k].name[p]='\0';

	WriteBlock(blk_num,(char *)dir);
	
	dir_inode->direct[total_blks]=blk_num;
	dir_inode->size+=DIRECTORYSIZE;
	write_inode(d_inode,dir_inode);
	
	printf("\nInside PutInDir : Reached outside loop with size as %d",dir_inode->size);
	printf("\nInside PutInDir : Reached 5");
	free(dir);
	free(dir_inode);
	return 0;
 }
int GetIndirectBlk(int blk_num,int index)
{
	int sec_num,res;
	int *sec;
	if(index>BLOCKSIZE/sizeof(int))
		return ERROR;
	sec=(int *)malloc(BLOCKSIZE);
	res=GetBlock(blk_num,(char *)sec);
	
	sec_num=sec[index];
	free(sec);
	return sec_num;


}
int PutIndirectBlk(int blk_num,int index,int val)
{
	int sec_num,res;
	int *sec;
	if(index>BLOCKSIZE/sizeof(int))
		return ERROR;
	sec=(int *)malloc(BLOCKSIZE);
	res=GetBlock(blk_num,(char *)sec);
	
	sec[index]=val;
	WriteBlock(blk_num,(char *)sec);
	free(sec);
	return sec_num;



}
void print_inode(int inode_num,struct inode *pinode)
{
	printf("\ninode no is %d",inode_num);
	printf("\ninode type is %d",pinode->type);
	printf("\ninode size is %d",pinode->size);
	printf("\ninode 1st block is %d",pinode->direct[0]);

}

int write_inode(int inode_num,struct inode *curr_inode)
{
	int sec_num,res,i;
	struct inode *fs;
	
	//printf("\nInside Write_inode:Reached");
	
	fs=(struct inode *)malloc(SECTORSIZE);
	
	if(fs==NULL)
	{
		printf("\nNot enough space in memory");
		return ERROR;
	}
	

	
	sec_num=BLOCKSIZE/SECTORSIZE+((inode_num*INODESIZE)/SECTORSIZE);
	
	i=(inode_num*INODESIZE)%SECTORSIZE;
	i=i/INODESIZE;
	//printf("\nvalue of written inode is %d",i);
	res=ReadSector(sec_num,(void *)fs);
	if(!res)
	{
		
		memcpy((fs+i),curr_inode,INODESIZE);
		WriteSector(sec_num,(void *)fs);
	}
	else
		printf("\nReadSector Unsuccessful!");

	
	
	
	free(fs);
	return 0;
	
}
int GetInode(int inode_num,struct inode *curr_inode)
{
	//printf("\nGetInode Reached Inside");
	//print_inode(inode_num,curr_inode);
	int res,i,sec_num;
	struct inode *fs=(struct inode *)malloc(SECTORSIZE);

	if(fs==NULL)
	{	printf("\nNot enough memory available ");
		return ERROR;

	}
	//printf("\nGetInode:Reached 1 ");

	sec_num=BLOCKSIZE/SECTORSIZE+((inode_num*INODESIZE)/SECTORSIZE);

	//printf("\nGetInode: Reached 2");

	i=(inode_num*INODESIZE)%(SECTORSIZE);
	i=i/INODESIZE;

	res=ReadSector(sec_num,(void *)fs);

	if(res==0)
		{
		memcpy(curr_inode,&(fs[i]),INODESIZE);
		//printf("\fs[i].type=%d",fs[i].type);
		}
	else
	{	printf("\nUnsuccessful ReadSector");
		free(fs);
		return ERROR;
	}
	

	//printf("\nGetInode: Reached end");
	free(fs);
	return 0;
}

int SearchInDir(int in_num,char *name,int num_sym_link)
{
	
	int num_blocks,blk_num;
	int i,k,p;
	
	int res;
	
	struct dir_entry *dir;
	
	char dname[DIRNAMELEN+1];
	char sympath[MAXPATHNAMELEN+1];
	char *block;
	struct inode *cinode=(struct inode*)malloc(INODESIZE);
	
	GetInode(in_num,cinode);
	if(cinode->type==INODE_SYMLINK)
	{
		printf("\nERROR: Given inode is a symbolic_link!");
		num_sym_link++;
		if(num_sym_link>MAXSYMLINKS)
			return ERROR;
		block=(char *)malloc (BLOCKSIZE);
		GetBlock(cinode->direct[0],block);
		strncpy(sympath,block,MAXPATHNAMELEN);
		sympath[MAXPATHNAMELEN]='\0';
		in_num=LookUp(sympath,in_num,num_sym_link);
		if(in_num==ERROR)
			return ERROR;
		
		GetInode(in_num,cinode);
	}
	else if(cinode->type!=INODE_DIRECTORY)
	{
		printf("\nERROR: Given inode is not a directory!");
		free(cinode);

		return ERROR;
	}
	
	
	num_blocks=(cinode->size+BLOCKSIZE-1)/BLOCKSIZE;
	
	dir=(struct dir_entry *)malloc(BLOCKSIZE);
	
	printf("\nSearchInDir : Reached 1");
	
	for(i=0;i<num_blocks;i++)
	{	
		//printf("\nSearchInDir : Reached 1 with cinode= %d and name = %s",cinode->type,name);
		if(i<NUM_DIRECT)
			blk_num=cinode->direct[i];
		else 
			blk_num=GetIndirectBlk(cinode->indirect,i-NUM_DIRECT);
				
		printf("\nSearchInDir : Reached 2 with blk num=%d",blk_num);
		GetBlock(blk_num,(char *)dir);
			
		for(k=0;k<(BLOCKSIZE/DIRECTORYSIZE);k++)
		{	
			//printf("\nSearchInDir : Reached 4");
			if(dir[k].inum==0)
				continue;
			for(p=0;p<DIRNAMELEN;p++)
				dname[p]=dir[k].name[p];
			dname[DIRNAMELEN]='\0';
			//printf("\nSearchInDir : Reached 4.5 with dname=%s and name =%s",dname,name);

			if(!strcmp(dname,name))
			{	printf("\nSearchInDir : Reached 5");
					printf("\nthe found dir has num = %d and name = %s ",dir[k].inum,dir[k].name);
					res=dir[k].inum;
					free(cinode);
					free(dir);
					return res;
			}
		}

	}
	free(cinode);
	free(dir);
	printf("\nSearchInDir : Reached 6");
	return 0;
}
int Get_free_inode()
{	struct free_inode_list *temp;
	int res;
	if(head_inodelist==NULL)
		return 0;
	temp=head_inodelist;
	res=temp->inode_num;
	head_inodelist=head_inodelist->next;
	free(temp);
	return res;
}
void Add_free_inode(int num)
{
	struct free_inode_list *temp=(struct free_inode_list *) malloc( sizeof(struct free_inode_list ));

	temp->inode_num=num;
	if(head_inodelist!=NULL)
		temp->next=head_inodelist;
	head_inodelist=temp;
	return;

}
int Get_free_blknum()
{	struct free_block_list *temp;
	int res;
	if(head_blklist==NULL)
		return 0;
	temp=head_blklist;
	res=temp->blk_num;
	head_blklist=head_blklist->next;
	free(temp);
	
	return res;
}
int Has_N_Free_Blocks(int num){
	int count=0;
	struct free_block_list *temp=head_blklist;
	while(temp!=NULL)
	{	count++;
		if(count==num)
			return 1;
		temp=temp->next;		
	}
	return 0;

}
void Add_free_blknum(int num)
{
	struct free_block_list *temp=(struct free_block_list *) malloc( sizeof(struct free_block_list ));
	temp->blk_num=num;

	if(head_blklist!=NULL)
		temp->next=head_blklist;
	head_blklist=temp;
	return;

}
Initialize_Server()
{
	void *fs=malloc(SECTORSIZE);

	
	int nfree_block;
	int i,k;
	int sec_num=BLOCKSIZE/SECTORSIZE;
	
	int size,fsec_num,inum,blk_num,index,nblocks;

	struct inode *inode_entry;
	
	int *free_blocks=NULL;
	int *free_inodes=NULL;
		
	ReadSector(sec_num,fs);
	
	num_inodes=(((struct fs_header *)fs)->num_inodes);
	num_blocks=((struct fs_header *)fs)->num_blocks;
	nfree_block= num_blocks-2-((num_inodes*INODESIZE)/BLOCKSIZE);
	

	max_inode_num=num_inodes;
	max_block_num=num_blocks-1;
	num_inodes++;
	free_blocks=(int *)malloc(sizeof(int)*num_blocks);
	
	free_inodes=(int *)malloc(sizeof(int)*num_inodes);
	
	
	for(i=0;i<num_blocks;i++)
		if(i<(num_blocks-nfree_block))
			free_blocks[i]=0;
		else
			free_blocks[i]=1;
	free_inodes[0]=0;
	

	sec_num=BLOCKSIZE/SECTORSIZE;
	fsec_num=BLOCKSIZE/SECTORSIZE+(num_inodes*INODESIZE)/SECTORSIZE;
	inum=1;
	

	while(sec_num<fsec_num)
	{	
		ReadSector(sec_num,fs);
		inode_entry=(struct inode *)fs;
		
		for(i=0;i<(SECTORSIZE/INODESIZE);i++)
		{	
			index=inum%(SECTORSIZE/INODESIZE);
			free_inodes[inum]=1;
			
			if(inode_entry[index].type!=INODE_FREE)
			{	
				free_inodes[inum]=0;
				size=inode_entry[index].size;
				nblocks=size/BLOCKSIZE;
			
				for(k=0;k<=nblocks;k++)
				{	
					if(k<NUM_DIRECT)
						blk_num=inode_entry[index].direct[k];
					else 
						blk_num=GetIndirectBlk(inode_entry[index].indirect,k-NUM_DIRECT);
					free_blocks[blk_num]=0;
				}

			}
			inum++;
	
		}
		sec_num++;
	}
	
	free(fs);
		
	for(i=num_inodes-1;i>1;i--)
		if(free_inodes[i]==1)
			Add_free_inode(i);
	
	for(i=num_blocks-1;i>=num_blocks-nfree_block;i--)
		if(free_blocks[i]==1)
			Add_free_blknum(i);
	
	free(free_inodes);
	free(free_blocks);
	printf("\nValue returned by register is %d\n",Register(FILE_SERVER));
	
	return 0;

}


