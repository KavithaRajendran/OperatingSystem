/********************************************************************************************************************************************************
 *
 * File Name: fsaccess.c
 *
 * How to execute this file:
 *  gcc -o output_file_name fsaccess.c
 *  ./output_file_name
 *
 * Description:
 *  Implementation of Unix V6 filesystem
 *
 * Authors:
 *  Name                    
 *  Kavitha Rajendran       
 *  Rajkumar Panneer Selvam   
 *
*********************************************************************************************************************************************************/

#include <stdio.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <stdlib.h> 
#include <math.h>
#define MAX 1024

//SuperBlock Structure
typedef struct super_block
{
    unsigned short isize;
    unsigned short fsize;
    unsigned short nfree;
    unsigned short free[100];
    unsigned short ninode;
    unsigned short inode[100];
    char flock;
    char ilock;
    char fmod;
    unsigned short time[2];
}super_block;

//Inode structure
typedef struct inode 
{
    unsigned short flags;
    char nlinks;
    char uid;
    char gid;
    char size0;
    unsigned short size1;
    unsigned short addr[8];
    unsigned short actime[2];
    unsigned short modtime[2];
}inode;

//One entry in a data block if file type is directory; First 2 bytes --> File inode number; remaining 16 bytes --> Filename
typedef struct dir 
{
    unsigned short inode_no;
    char file_name[14];
}dir;

//File descriptor of V6FileSystem 
int fd;

super_block superblock = {0};
inode current_inode;

//This function returns the next available free block
unsigned short getFreeBlockk() 
{
    unsigned short freeBlock;
    lseek(fd, 512, SEEK_SET);
   
    read(fd, & superblock, sizeof(super_block));

    if (superblock.nfree > 0) 
    {
    
        freeBlock = superblock.free[superblock.nfree];
        superblock.nfree--;
        lseek(fd, 512, SEEK_SET);
        write(fd, & superblock, sizeof(superblock));
    } 
    else 
    {
        freeBlock = superblock.free[superblock.nfree];
        if (freeBlock == 0) 
		{
            printf(" Free Block over \n ");
            return 0;
        }
        int curpos = lseek(fd, 512 * superblock.free[superblock.nfree], SEEK_SET);
        unsigned short data;
        read(fd, & superblock.nfree, sizeof(superblock.nfree));
        int i;
        for (i = 0; i <= superblock.nfree; i++)
		{
            ssize_t bytes_read;
            bytes_read = read(fd, & data, sizeof(data));

            superblock.free[i] = data;

        }
        lseek(fd, 512, SEEK_SET);
        write(fd, & superblock, sizeof(superblock));

		lseek(fd, curpos, SEEK_SET);
		data = 0;
		write(fd, & data, 2);
		lseek(fd, curpos, SEEK_SET);
    }
	return freeBlock;
}

// Initializes the file system with the given total number of blocks & total number of inodes
// Also initializes the super block contents & creates root directory
initializeFS(int totalBlocks, int no_of_Inodes)
{

	fd = open("V6FileSystem", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	initializeSuperBlock(totalBlocks, no_of_Inodes);

	initializeRootInode();
    
    printf(" V6FileSystem initialized successfully \n");
}

//Initialize the inode for root directory
initializeRootInode()
{
	int offset = 512 * 2;
	int curpos = lseek(fd, offset, SEEK_SET);

	inode rootInodeData;
	ssize_t bytes_read = read(fd, & rootInodeData, sizeof(inode));
	setAllocatedBitINode( & rootInodeData);

	setDirectoryTypeFile( & rootInodeData);

	dir dirData;
	dirData.inode_no = 1;
	strcpy(dirData.file_name, ".");

	writeDirBlock(fd, & dirData, & rootInodeData);

	dirData.inode_no = 1;
	strcpy(dirData.file_name, "..");
	writeDirBlock(fd, & dirData, & rootInodeData);

	curpos = lseek(fd, 512 * 2, SEEK_SET);

	bytes_read = write(fd, & rootInodeData, sizeof(inode));
    current_inode = rootInodeData;

}

//Write data into Directory Data Block
int writeDirBlock(int fd, void * data, inode * i_node) 
{
	unsigned short freeBlockNo = 0;
	int i = 0;
	if (i_node->addr[0] > 0)
	{
		freeBlockNo = i_node->addr[0];
	}
	else 
	{
		freeBlockNo = getFreeBlockk();
	}

	while (!(freeBlockNo == 0) && (writeBlock(fd, data, freeBlockNo * 512, 1) < 0) && i < 8)
	{
		i++;
		if (i_node->addr[i] > 0) 
		{
			freeBlockNo = i_node->addr[i];
		}
		else 
		{
			freeBlockNo = getFreeBlockk();
		}
	}
	if (freeBlockNo != 0)
	{
		i_node->addr[i] = freeBlockNo;
	}
    else
    {
        return -1;
    }
    return 0;

}

//Sets the allocated bit for the given inode
setAllocatedBitINode(inode * i_node)
{
	i_node->flags |= (1 << 15);
}

//Resets the allocated bit for the given inode
resetAllocatedBitInode(inode * i_node) 
{
	i_node->flags &= ~(1 << 15);
}

//Sets the Largefile bit for the given inode
setLargeFileBitINode(inode * i_node)
{
	i_node->flags |= (1 << 12);
}

//Resets the Largefile bit for the given inode
resetLargeFileBitInode(inode * i_node)
{
	i_node->flags &= ~(1 << 12);
}

//Sets the filetype bits for the given inode
setDirectoryTypeFile(inode * i_node)
{
	i_node->flags |= (1 << 14);
	i_node->flags &= ~(1 << 13);
}

//Checks given inode is allcoated to any file or not
int isAllocatedInode(inode * i_node) 
{
    return ((i_node->flags >> 15) & 1);
}

//Checks given file is small or large
int isLargeFile(inode * i_node)
{
    return ((i_node->flags >> 12) & 1);
}

//Checks given inode is for file or directory
int isDirectory(inode * i_node)
{
    return (((i_node->flags >> 14) & 1) & ~((i_node->flags >> 13) & 1));
}

//Get next available free inode
int getFreeInode()
{

	int curpos = lseek(fd, 512 * 2, SEEK_SET);
	int inode_no = 1;
	int offset = 512 * 2;
	inode node;
	ssize_t nbytes;
	while ((nbytes = read(fd, & node, sizeof(inode))) != -1 && isAllocatedInode( & node)) 
	{
		lseek(fd, offset + (32 * inode_no), SEEK_SET);
		inode_no++;
		//search for new inode
	}
	return inode_no;

}

//Sets all the fields in single and double indirect blocks to zero
initializeToZero(unsigned short block)
{
    lseek(fd,block*512,SEEK_SET);
    int size=0;
    while ( size < 512)
    {
        unsigned short   temp =0;
        write(fd, & temp, sizeof(temp));
        size+=sizeof(temp);
    }
}

//Write the addr[] of inode into single indirect block till single indirect blocks are available (ie) from addr[0] to addr[6]; 
//Else try for double indirection
int writetSingleIndirectBlock(inode * i_node, unsigned short indirectblock[], unsigned short freeBlockNo, int isFromDoubleIndirection) 
{
	int i = 0;
	if (freeBlockNo == 0 && isFromDoubleIndirection == 0)
	{
		if (indirectblock[0] > 0) 
		{
			freeBlockNo = indirectblock[0];
		} 
		else 
		{
			freeBlockNo = getFreeBlockk();
			if(freeBlockNo!=0)
			    initializeToZero(freeBlockNo);
			else
			    return 0;
		}
	}


	while (!(freeBlockNo == 0) && (writeBlock(fd, i_node, freeBlockNo * 512, 2) < 0) && i < 7)
	{
		if (isFromDoubleIndirection == 1)
			return -1;
		i++;

		if (indirectblock[i] > 0 && i < 7) 
		{
			freeBlockNo = indirectblock[i];
		} 
		else if(i<7) 
		{
			freeBlockNo = getFreeBlockk();
            
			initializeToZero(freeBlockNo);
		}
	}

	if (isFromDoubleIndirection == 1)
			return 0;

	if (freeBlockNo != 0 && i != 7) 
	{
		indirectblock[i] = freeBlockNo;
	}

	if (i == 7) 
	{
		doubleIndirection(i_node, indirectblock);
	}

	for (i = 0; i < 8; i++) 
	{
		i_node->addr[i] = 0;
	}
	return 0;
}

//Write addr[] of given inode into double indirect block
doubleIndirection(inode * i_node, unsigned short indirectblock[])
{
	unsigned short freeBlockNo;
	int i = 0, nofBlocks=0;
	if (indirectblock[7] <= 0)
	{
	    int temp=0;
	    temp=getFreeBlockk();
	    if(temp!=0)
	    {
		    indirectblock[7] =temp;
		    initializeToZero(indirectblock[7]);
		}
		else
		{
			return;
		}
	}
	lseek(fd, indirectblock[7] * 512, SEEK_SET);
	read(fd, & freeBlockNo, sizeof(freeBlockNo));
	if (freeBlockNo > 0) 
	{
		while (writetSingleIndirectBlock(i_node, indirectblock, freeBlockNo, 1) < 0) 
		{
			i++;
			nofBlocks++;
			lseek(fd, (indirectblock[7] * 512) + (2 * i), SEEK_SET);
			read(fd, & freeBlockNo, sizeof(freeBlockNo));
			if (freeBlockNo <= 0) 
			{
				if(nofBlocks< 249)//For 32MB file size limit
				{
				    int temp=0;
				    temp=getFreeBlockk();
				    if(temp!=0)
				    {
					    freeBlockNo = temp;
					    initializeToZero(freeBlockNo);
					    lseek(fd, (indirectblock[7] * 512) + (2 * (i)), SEEK_SET);
					    write(fd, & freeBlockNo, sizeof(freeBlockNo));
					}
					else
					{
					    return;
					}
				}
				else
				{
					printf("Max file size 32 MB reached");
					break;
				}
			}
		}
	}
	else 
	{
        int temp=0;
        temp=getFreeBlockk();
        if(temp!=0)
        {
            freeBlockNo = temp;
		    initializeToZero(freeBlockNo);
		    lseek(fd, indirectblock[7] * 512, SEEK_SET);
		    write(fd, & freeBlockNo, sizeof(freeBlockNo));
		    while (writetSingleIndirectBlock(i_node, indirectblock, freeBlockNo, 1) < 0) 
		    {
			    i++;
			    nofBlocks++;
			    lseek(fd, (indirectblock[7] * 512) + (2 * (i)), SEEK_SET);
			    read(fd, & freeBlockNo, sizeof(freeBlockNo));
			    if (freeBlockNo <= 0) 
			    {   
			        if(nofBlocks< 249) //For 32MB file size limit
                    {
				        int temp=0;
		                temp=getFreeBlockk();
				        if(temp!=0)
				        {	
				            freeBlockNo =temp;
				            initializeToZero(freeBlockNo);
				            lseek(fd, (indirectblock[7] * 512) + (2 * (i)), SEEK_SET);
				            write(fd, & freeBlockNo, sizeof(freeBlockNo));
				        }
				        else
				        {
					        return;
				        }
			        }
		 		    else
		            {
		                printf("Max file size 32 MB reached");
		                break;
			        }
			    }
		    }   
	    }
	    else
	    {
		    return;
	    }
    }
}

//Write the given data into addr[] of inode until addr[] gets filled; 
//Else write filled addr[] into single indirect block
int writeToFile(char data[], inode * i_node, unsigned short indirectblock[])
{
    unsigned short freeBlockNo = 0;
    int i = 0;
	int isLargeBlock=0,isLargeWritten=0;
	isLargeBlock=isLargeFile(i_node);
    while (i_node->addr[i] > 0 && i_node->addr[i] != 65535 && i < 8)
	{
			i++;
    }
    if (i < 8)
    {
        freeBlockNo = getFreeBlockk();
    } 
    else
	{
	    isLargeWritten=1;
		setLargeFileBitINode(i_node);
		unsigned short s = 0;
		writetSingleIndirectBlock(i_node, indirectblock, s, 0);
		int p=writeToFile(data, i_node, indirectblock);
		return p;
    }
    if (!(freeBlockNo == 0) && (writeBlock(fd, data, freeBlockNo * 512, 0) < 0))
	{
        printf(" WriteBlock Failed , Error occured while writing into V6FileSystem\n");
    }
    if (freeBlockNo != 0) 
    {
        i_node->addr[i] = freeBlockNo;
    }
    else
    {
        return -1;
    }

	if((isLargeBlock==1 && isLargeWritten!=0) )
	{
		unsigned short s = 0;
        writetSingleIndirectBlock(i_node, indirectblock, s, 0);
	}
return 0;
}

//Copies the given source file into destination file in the V6filesystem
copyin(char * source, char * dest)
{
    char token[1000];
    char *temptoken;
    strcpy(token,dest);
	int isFile ;
	isFile=isFileAlreadyExist(dest);
	if (isFile > 0 ) 
	{
        setInode1asCurrent();
        printf("File name already exist \n");
	    return;
	} 
	else if (isFile == -2)
	{
        setInode1asCurrent();
        printf("one of the Directory in the given path not exist \n");
        return;
	}
	else
	{
	    temptoken=strtok(token,"/");
		while(temptoken)
		{
		    dest=temptoken;
		    temptoken=strtok(0,"/");
	    }
	}
	int inodeNo = getFreeInode();
	if (inodeNo > superblock.isize)
	{
		printf(" \n Inode limit reached, no more files or directory can be created \n ");
		return;
	}

    writeFileNameinDir(inodeNo, dest);

	lseek(fd, ((inodeNo - 1) * 32) + (512 * 2), SEEK_SET);
	inode new_inode;
	ssize_t nbytes = read(fd, & new_inode, sizeof(inode));
	if (nbytes > 0)
	{
		int sourceFd = open(source, O_RDWR);
		char buf[512];
		unsigned short indirectblock[8] = {0,0,0,0,0,0,0,0};
		setAllocatedBitINode( & new_inode);
		int bytes = 512;
        int isSuccess=0;
		while (read(sourceFd, & buf, 512) > 0)
		{
		if(	(isSuccess=writeToFile(buf, & new_inode, indirectblock))<0)
        {
            printf(" cpin Failed\n");
            break;
            
        }
			bytes += 512;
		}
		if(isLargeFile(&new_inode)==1)
		{
		       unsigned short s = 0;
		       writetSingleIndirectBlock(&new_inode, indirectblock, s, 0);

			int i;
			for(i=0;i<8;i++)
			{
				new_inode.addr[i]=indirectblock[i];
			}
		}
        
		lseek(fd, ((inodeNo - 1) * 32) + (512 * 2), SEEK_SET);
		write(fd, & new_inode, sizeof(inode));
        if(isSuccess==0)
        {
        readFileInodeAddr(inodeNo);
        readDirInodeAddr(getCurrentDirectoryInodeNo());
        printf(" Given File copied into V6FileSystem successfully \n");

        }
        else
        {
            printf("Given file is partially written into the Filesystem till free data block exists \n");

        }
        
		setInode1asCurrent();
	}
}

//Returns the inode number of given file
int getInodeNumber(char *path)
{
    int i;
	for(i=0;i<8;i++)
	{
		int curpos = lseek(fd, 512*current_inode.addr[i], SEEK_SET);
		int size=0;
		int count=0;
		while(size <512 )
		{
			dir tempdir;
			ssize_t bytes_read = read(fd, &tempdir, sizeof(dir));
			count++;
			if(bytes_read==16)
			{
				if(strcmp(tempdir.file_name,path)==0)
				{
				return tempdir.inode_no;
				}
			}
			size += 16;
		}
	}
    for (i = 0; i < 8; i++)
    {
        int curpos = lseek(fd, 512 * current_inode.addr[i], SEEK_SET);
        int size = 0;
        int count = 0;
        while (size < 512)
        {
            dir tempdir;
            ssize_t bytes_read = read(fd, & tempdir, sizeof(dir));
            count++;
            if (bytes_read == 16)
            {
                if (strcmp(tempdir.file_name, path) == 0 && tempdir.inode_no >0)
                {
                    return 1;
                }
            }
            size += 16;
        }
    }
    return 0;
}

//Return 1 if the file/directory exist in filesystem; else returns negative numbers
int isFileAlreadyExist(char *fullPath)
{
    char* token;
    char temptoken[1000];
    int i=0,j=0;
    strcpy(temptoken,fullPath);
    token=strtok(temptoken,"/");

    while(token)
    {
        token=strtok(0,"/");
        i++;
    }
    token = strtok(fullPath,"/");
    while(token && j < i)
    {
        if( j==i-1)
        {
            if(isDirAlreadyExist(token)==1)
            {
                printf(" exist \n");
	            int inodeNumber= getInodeNumber(token);
                return inodeNumber;
            }
            else
            {
                printf(" Given File  not exist \n");
                return -1;
            }
        }
        if (isDirAlreadyExist(token)==1)
        {
            int inodeNumber= getInodeNumber(token);
            lseek(fd,((inodeNumber-1)*32)+(512*2),SEEK_SET);
            read(fd,&current_inode,sizeof(inode));
        }
        else
        {
            printf("Directory %s not exist \n", token);
            return -2;
        }
        token=strtok(0,"/");
        j++;
   }
}
//Creates the new directory inside V6filesystem
mkdirV6(char * path)
{
    char* token;
    char temptoken[1000];
    int i=0,j=0;
    strcpy(temptoken,path);
    token=strtok(temptoken,"/");
	
	while(token)
	{
		token=strtok(0,"/");
		i++;
	}

    token = strtok(path,"/");

	while(token && j < i)
	{
		if( j==i-1)
		{
			if(isDirAlreadyExist(token)==1)
			{
				printf("Directory Already exist \n");
				return;
			}
			else
			{
				break;
			}
		}
		
		if (isDirAlreadyExist(token)==1)
		{
			int inodeNumber= getInodeNumber(token);
			lseek(fd,((inodeNumber-1)*32)+(512*2),SEEK_SET);
			read(fd,&current_inode,sizeof(inode));
		}
		else
		{
			printf("Directory %s not exist \n", token);
			return;
		}
		token=strtok(0,"/");
		j++;
	}
	int inodeNo = getFreeInode();

	if (inodeNo > superblock.isize)
	{
		printf(" \n Inode limit reached, no more files or directory can be created \n ");
		return;
	}

	lseek(fd, ((inodeNo - 1) * 32) + (512 * 2), SEEK_SET);
	inode new_inode;
	ssize_t nbytes = read(fd, & new_inode, sizeof(inode));
	if (nbytes > 0)
	{
		setAllocatedBitINode( & new_inode);

		setDirectoryTypeFile( & new_inode);

		dir dirData;
		dirData.inode_no = inodeNo;
		strcpy(dirData.file_name, ".");

	if(	writeDirBlock(fd, & dirData, & new_inode)==-1)
    {
        printf( "  Given Directory not created \n");
        resetAllocatedBitInode(& new_inode);
        return;
    }

		dirData.inode_no = getCurrentDirectoryInodeNo();

		strcpy(dirData.file_name, "..");

		if(writeDirBlock(fd, & dirData, & new_inode)==-1)
        {
              printf( "  Given Directory not created \n");
                return;
        }

		int curpos = lseek(fd, ((inodeNo - 1) * 32) + (512 * 2), SEEK_SET);

		ssize_t bytes_read = write(fd, & new_inode, sizeof(inode));

		writeFileNameinDir(inodeNo, token);

		readDirInodeAddr(inodeNo);
		readDirInodeAddr(getCurrentDirectoryInodeNo());
		readDirInodeAddr(1);
		setInode1asCurrent();
        printf(" Given Directory created successfully \n");
	}
}

//Returns current directory inode number
int getCurrentDirectoryInodeNo()
{
	dir tempdir;
	lseek(fd, 512 * current_inode.addr[0], SEEK_SET);
	read(fd, & tempdir, sizeof(dir));
	return tempdir.inode_no;
}

//Check given directory exists in file system
int isDirAlreadyExist(char * path) 
{
	int i;
	for (i = 0; i < 8; i++) 
	{
		int curpos = lseek(fd, 512 * current_inode.addr[i], SEEK_SET);
		int size = 0;
		int count = 0;
		while (size < 512) 
		{
			dir tempdir;
			ssize_t bytes_read = read(fd, & tempdir, sizeof(dir));
			count++;
			if (bytes_read == 16) 
			{
		        if (strcmp(tempdir.file_name, path) == 0 && tempdir.inode_no >0) 
			    {
					return 1;
			    }
			}
			size += 16;
		}
	}
	return 0;
}

//Write given filenames inside the directory data block
writeFileNameinDir(int inode_no, char * path) 
{
	int i;
	for (i = 0; i < 8; i++) 
	{
		int curpos = lseek(fd, 512 * current_inode.addr[i], SEEK_SET);
		int size = 0;
		int count = 0;
		while (size < 512) 
		{
			dir tempdir;
			ssize_t bytes_read = read(fd, & tempdir, sizeof(dir));
			count++;
			if (tempdir.inode_no == 0) 
			{
					tempdir.inode_no = inode_no;
					strcpy(tempdir.file_name, path);
					writeDirBlock(fd, & tempdir, & current_inode);
					return;
			}
			size += 16;
		}
	}
	return 0;
}

//Sets root node as current inode
setInode1asCurrent()
{
    int curpos = lseek(fd, 512*2, SEEK_SET);
    ssize_t bytes_read = read(fd, &current_inode, sizeof(inode));
}

//Read existing initiazlised V6filesystem file
readV6FS() 
{
	fd = open("V6FileSystem", O_RDWR);
	int curpos = lseek(fd, 512 * 2, SEEK_SET);
	ssize_t bytes_read = read(fd, & current_inode, sizeof(inode));
	if (!isAllocatedInode( & current_inode)) 
	{
		printf("V6FileSystem not initialized \n");
		return;
	}
	curpos = lseek(fd, 512 * 1, SEEK_SET);
	bytes_read = read(fd, & superblock, sizeof(super_block));
}

void main() 
{
    
    char input[MAX];
    char* commandsArgv[256];
    int res;
    while(1)
    {
        // Printing command prompt
        printf(">>");
        // Gets user input
        if(fgets(input,sizeof(input),stdin))
        {
            char* s;
            int spaceCharIndex = strlen(input)-1;
            int j=0;
            input[spaceCharIndex]='\0';
            commandsArgv[j] = strtok(input, " " );

            while( commandsArgv[j]!=NULL)
            {
                commandsArgv[++j]=strtok(NULL, " " );
            }
 
            if(commandsArgv[0]!=NULL)
            {
            // if user enters 'q', comeout of loop
            res = strcmp(input,"q");
            if (res == 0)
            {
                printf("Exiting from file system... \n");
                break;
            }
            else
            {
                if(!strcmp(commandsArgv[0],"initfs"))
                {
                    printf("Initiating File System \n");
                    initializeFS(atoi(commandsArgv[1]), atoi(commandsArgv[2]));
                }
                else if(!strcmp(commandsArgv[0],"cpin"))
                {
                    printf("Copying external file into filesystem \n");
                    readV6FS();
                    copyin(commandsArgv[1], commandsArgv[2]);
                }
                else if(!strcmp(commandsArgv[0],"cpout"))
                {
                    printf("Copying out a file from filesystem \n");
                    readV6FS();
                    copyout(commandsArgv[1], commandsArgv[2]);
                }
                else if(!strcmp(commandsArgv[0],"mkdir"))
                {
                    printf("Creating directory inside file system \n");
                    readV6FS();
                    mkdirV6(commandsArgv[1]);
                }
                else if(!strcmp(commandsArgv[0],"rm"))
                {
                    printf("Deleting a file from filesystem \n");
                    readV6FS();
                    removeFileDir(commandsArgv[1]);

                }
                else
                {
                    printf("Please enter valid input \n");
                    printf("Below are options:\n");
                    printf("    initfs <fsize> <total_num_of_inodes> \n");
                    printf("    cpin <external_sourceFilePath> <destination_path>\n");
                    printf("    cpout <internal_sourceFilePath> <external_destPath>\n");
                    printf("    mkdir <DirectoryPath>\n");
                    printf("    rm <FilePath>     \n");
                    printf("Or type q to exit \n");
                }
                }
            }
        }
    }
}

//Reads directory data block of given directory inode
readDirInodeAddr(int inode_number) 
{
    printf(" Displaying the contents of Directory with I_node no %d \n",inode_number);
	int i;
	int curpos = lseek(fd, ((inode_number - 1) * 32) + (512 * 2), SEEK_SET);
	inode node;
	ssize_t bytes_read = read(fd, & node, sizeof(inode));
	printf(" /n -- Inode no %d --/n", inode_number);
	printf(" inode flags isDirec %d , isAlloc %d , isLarge %d ", isAllocatedInode( & node), isDirectory( & node), isLargeFile( & node));
	printf("/n Address array");
	for (i = 0; i < 8; i++) 
	{
		printf(" \n array[%d] is %d \n", i, node.addr[i]);
		curpos = lseek(fd, 512 * node.addr[i], SEEK_SET);
		int size = 0;
		int count = 0;
		while (size < 512 && i == 0) 
		{
			dir tempdir;
			bytes_read = read(fd, & tempdir, sizeof(dir));
			count++;
			if (bytes_read == 16)
					printf(" bytes read %d ,  Directory inode_no %d , file name is %s \n ", bytes_read, tempdir.inode_no, tempdir.file_name);
			size += 16;
		}
		printf(" No of dir %d \n", count);
	}
}

//Reads content of given inode
readFileInodeAddr(int inode_number)
{
    int i;
    int curpos = lseek(fd, ((inode_number - 1) * 32) + (512 * 2), SEEK_SET);
    inode node;
    ssize_t bytes_read = read(fd, & node, sizeof(inode));
    printf(" /n -- Inode no %d --/n", inode_number);
    printf(" inode flags isDirec %d , isAlloc %d , isLarge %d ", isAllocatedInode( & node), isDirectory( & node), isLargeFile( & node));
    printf("/n Address array");
    for (i = 0; i < 8; i++)
    {
        printf(" \n array[%d] is %d \n", i, node.addr[i]);
    }
}

//Initializes super block of V6FileSystem
initializeSuperBlock(int totalBlocks, int no_of_Inodes)
{
	initializeInode(no_of_Inodes);
	int no_Of_Inodes_Blocks = no_of_Inodes / 16;
	if (no_of_Inodes % 16 > 0) 
	{
		no_Of_Inodes_Blocks++;
	}
	int freeNodeStartPoint = no_Of_Inodes_Blocks + 2;
	initializeFreeBlock(totalBlocks, freeNodeStartPoint, 1);
	superblock.nfree--;

	superblock.isize = no_of_Inodes;
	int curpos = lseek(fd, 512, SEEK_SET);
	write(fd, & superblock, sizeof(superblock));
}
//Initialize the given number of inodes in the V6 filesystem
initializeInode(int no_of_Inodes)
{
	int offset = 512 * 2;
	int curpos = lseek(fd, offset, SEEK_SET);
	int i;
	no_of_Inodes++;
	while (no_of_Inodes) 
	{
		inode inodeData = {0};
		for (i = 0; i < 8; i++)
		{
		    inodeData.addr[i] = 0;
	    }
		write(fd, & inodeData, sizeof(inode));
		no_of_Inodes--;
		curpos = lseek(fd, 0, SEEK_CUR);
	}
}
//Initialize free list of the V6filesystem 
initializeFreeBlock(int totalBlocks, int freeNodeStartPoint, int isFirstBlock) 
{
	int i;
	int curpos = lseek(fd, 0, SEEK_CUR);
	if (isFirstBlock)
    {
		superblock.nfree = 1;
		superblock.free[0] = 0;
	} 
	while (freeNodeStartPoint < totalBlocks) 
	{
		while (superblock.nfree != 100 && freeNodeStartPoint < totalBlocks) 
		{
			superblock.free[superblock.nfree] = freeNodeStartPoint;
			superblock.nfree++;
			freeNodeStartPoint++;
		}
		int curpos = lseek(fd, freeNodeStartPoint * 512, SEEK_SET);
		if (freeNodeStartPoint < totalBlocks)
		{
            superblock.nfree--;
			write(fd, & superblock.nfree, sizeof(superblock.nfree));
			for (i = 0; i <=superblock.nfree; i++) 
			{
			write(fd, & superblock.free[i], sizeof(superblock.free[i]));
			}
			superblock.nfree = 0;
		}
	}
}
//Writes the data block depends on the isDir values
//if isDir = 1, write the given data as a directory content
//else if isDir=2, writes the given addresses into single/double indirect block
//else, writes it as a plain file 512 bytes
int writeBlock(int fd, void * data, int offset, int isDir) 
{
	unsigned int size = 0;
	int curpos = lseek(fd, offset, SEEK_SET);
	if (curpos == offset && isDir == 1) 
	{
		dir temp;
		ssize_t nbytesRead;
		while ((nbytesRead = read(fd, & temp, sizeof(dir))) > 0 && size < 512) 
		{
			if (temp.inode_no > 0) 
			{
				size += sizeof(dir);
			} 
			else 
			{
				break;
			}
		}
		if (size < 512) 
		{
			lseek(fd, -nbytesRead, SEEK_CUR);
			write(fd, data, sizeof(dir));

		} 
		else 
		{
			return -1;
		}
	} 
	else if (isDir == 2) 
	{
		unsigned short temp;
		ssize_t nbytes;
		int i=0;
		while (((nbytes = read(fd, & temp, sizeof(temp))) > 0) && temp > 0 && size < 512) 
		{i++;
			size += sizeof(temp);
		}
		if (size < 512) 
		{	
			lseek(fd, -nbytes, SEEK_CUR);
			int i;
			for (i = 0; i < 8; i++) 
			{
				temp = ((struct inode * ) data)->addr[i];
				if(temp >0 )
				write(fd, & temp, sizeof(temp));
			}
		} 
		else 
		{
			return -1;
		}
	} 
	else 
	{
	    int i;
	    char buf[512];
        strncpy(buf,data, 512);
		ssize_t nbytes = write(fd, buf, sizeof(char)*512);
	}
	return 1;
}

//Add given free block into freelist
addFreeBlocks(unsigned short freeBlockNo)
{
    int i;
    off_t addr;
    addr=lseek(fd, 0, SEEK_CUR);
    if (superblock.nfree != 100 )
    { 
	    superblock.nfree++;
        superblock.free[superblock.nfree] = freeBlockNo;
	    lseek(fd, 512, SEEK_SET);
	    write(fd, & superblock, sizeof(super_block));
        lseek(fd, addr, SEEK_SET);
    }
    else
    {
        int curpos = lseek(fd, freeBlockNo * 512, SEEK_SET);
        write(fd, & superblock.nfree, sizeof(superblock.nfree));
        for (i = 0; i <= superblock.nfree; i++)
        {
            write(fd, & superblock.free[i], sizeof(superblock.free[i]));
        }
        superblock.nfree = 0;
        superblock.free[superblock.nfree]=freeBlockNo;
        lseek(fd, addr, SEEK_SET);
   }
}

//Frees all 256 addresses of single indirect block and also given block; And add them into free list 
removeBlock(unsigned short  blockNo)
{

   unsigned short temp,size=0;
   ssize_t nbytes;
   lseek(fd, blockNo*512, SEEK_SET);
   int i=0;
   while (((nbytes = read(fd, & temp, sizeof(temp))) > 0) && temp > 0 && size < 512)
    {
	addFreeBlocks(temp);
	
               size += sizeof(temp);
	       i++;
         }
	 addFreeBlocks(blockNo);
}

//Frees all 256 addresses of double indirect block and add it into free list
removeDoubleIndirect(inode *i_node)
{
    unsigned short temp,size=0;
    off_t addr;
    ssize_t nbytes;
	if(i_node->addr[7]>0 && i_node->addr[7] != 65535 )
	{
	     lseek(fd, i_node->addr[7]*512, SEEK_SET);
	    while (((nbytes = read(fd, & temp, sizeof(temp))) > 0) && temp > 0 && temp!= 65535  && size < 512)
	    {

	        addr=lseek(fd, 0, SEEK_CUR);
            removeBlock(temp);
	        size += sizeof(temp);
	        lseek(fd, addr, SEEK_SET);
	    }
         addFreeBlocks(i_node->addr[7]);
	}	
}

//Deletion of large file
removeLargeFie(inode * i_node)
{
    int i = 6;
    removeDoubleIndirect(i_node);
    while (i!=-1 && i_node->addr[i]>0 && i_node->addr[i] != 65535 )
    {
	    removeBlock(i_node->addr[i]);
        i--;
    }
    resetLargeFileBitInode(i_node);
}

//Removes file name entry from the directory data block
removeFileNameinDir(int inode_no)
{
        int i;
        for (i = 0; i < 8; i++)
        {
              int curpos = lseek(fd, 512 * current_inode.addr[i], SEEK_SET);
              int size = 0;
           int count = 0;
              while (size < 512)
             {
                 dir tempdir;
                ssize_t bytes_read = read(fd, & tempdir, sizeof(dir));
                  count++;
                 if (tempdir.inode_no == inode_no)
                    {
                       tempdir.inode_no = 0;
		       strcpy(tempdir.file_name,"");
		        lseek(fd,-bytes_read, SEEK_CUR);
			 write(fd, &tempdir, sizeof(dir));
                      return;
                    }
                size += 16;
             }
      }
    return 0;
}

//Removes file from v6filesystem of given inode
rmfile(  inode *i_node)
{
    int i=0;
	if(isLargeFile(i_node)==1)
	{	
		removeLargeFie(i_node);
	}	
	else
	{
  		while(i_node->addr[i] > 0 && i_node->addr[i] != 65535 && i<8)
		{		
			addFreeBlocks(i_node->addr[i]);
			i++;
		}
	}
    resetAllocatedBitInode(i_node);
        for(i=0;i<8;i++)
        {
            i_node->addr[i] =0;
        }
}

//Removes the given file from v6filesystem using given file path
removeFileDir(char * path)
{
    inode new_inode;
    int i_node_no;
    i_node_no=isFileAlreadyExist(path);
	if(i_node_no>0)
	{
		 lseek(fd, ((i_node_no - 1) * 32) + (512 * 2), SEEK_SET);
		 ssize_t nbytes = read(fd, & new_inode, sizeof(inode));
		 if(isDirectory(&new_inode))
		 {
		 	printf("Given file is the directory, only file remove is allowed \n");
			
		 }
		 else
		 {
		 	rmfile(&new_inode);
			removeFileNameinDir(i_node_no);
          int curpos = lseek(fd, ((i_node_no - 1) * 32) + (512 * 2), SEEK_SET);

        ssize_t bytes_read = write(fd, & new_inode, sizeof(inode));

			readDirInodeAddr(getCurrentDirectoryInodeNo());
			setInode1asCurrent();
        }
        printf("Given file removed from the V6FileSystem successfully \n");
	}
	else
	{
		printf("Given file or directory not exist %s \n",path);
	}
}
/**************************************************************************************
*This function returns whole inode structure when inode number is given as an argument
 **************************************************************************************/
inode getInodeInfoFromInodeNum(int inode_no)
{
    lseek(fd, ((inode_no - 1) * 32) + (512 * 2), SEEK_SET);
    inode new_inode;
    ssize_t nbytes = read(fd, & new_inode, sizeof(inode));
    return new_inode;
}

/********************************************************************************************
 * This function Reads 512 bytes of v6filesystem from given offset and write into output file 
 *********************************************************************************************/
readOneBlockAndWriteIntoFile(int fd, int offset, int fd_outputFile)
{
    int curpos = lseek(fd, offset, SEEK_SET);
    char cbuf[512];
    int nbytes= read(fd,cbuf,511);
    //printf("Data  %s  \n",cbuf);
    int count = write(fd_outputFile,cbuf,512);
}
/**************************************************************************************
* For small file - Gets file's inode as input & copies the file content to output file
* *************************************************************************************/
copyoutSmallFile(int fd_outputFile, inode * inputFileinode)
{
    int i;
    for(i=0;i<8;i++)
    {
        if((inputFileinode->addr[i]!=0)&&(inputFileinode->addr[i]!=65535))
        {
            int offset = (inputFileinode->addr[i])*512;
            readOneBlockAndWriteIntoFile(fd,offset,fd_outputFile);
        }
    }
         printf("File copied completely \n");
}
/**************************************************************************************
* For Large file - Gets file's inode as input & copies the file content to output file
* *************************************************************************************/
copyoutLargeFile(int fd_outputFile, inode * inputFileinode)
{
    int i,j,k;
    for(i=0;i<8;i++)
    {
        //Handling single indirect block
        if(inputFileinode->addr[i]!=0 && (i<7) &&(inputFileinode->addr[i]!=65535) )
        {
            unsigned short nextDataBlockAddr=0;
            int offset = (inputFileinode->addr[i])*512;
            for(j=0;j<256;j++)
            {
                int curpos = lseek(fd, offset, SEEK_SET);
                read(fd,&nextDataBlockAddr,sizeof(nextDataBlockAddr));
                if(nextDataBlockAddr!=0&&nextDataBlockAddr!=65535)
                {
                    readOneBlockAndWriteIntoFile(fd,(nextDataBlockAddr*512),fd_outputFile);
                    offset=offset+sizeof(nextDataBlockAddr);
                }
                else
                {
                    printf("File copied completely \n");
                    break;
                }
            }
            if(nextDataBlockAddr==0 || nextDataBlockAddr==65535)
            {
                printf("cpout involving single indirect block completed successfully \n");
                break;
            }
        }
        //Handling double indirect block
        else if(inputFileinode->addr[i]!=0 && i==7&&(inputFileinode->addr[i]!=65535))
        {
            int first_offset = (inputFileinode->addr[i])*512;
            if(first_offset!=0)
            {
            for(j=0;j<256;j++)
            {
                unsigned short secondIndirectBlockAddr=0;
                int curpos = lseek(fd, first_offset, SEEK_SET);
                read(fd,&secondIndirectBlockAddr,sizeof(secondIndirectBlockAddr));
                int second_offset = (secondIndirectBlockAddr*512);
                for(k=0;k<256;k++)
                {
                    if(secondIndirectBlockAddr!=0 && secondIndirectBlockAddr!=65535)
                    {
                        readOneBlockAndWriteIntoFile(fd,(secondIndirectBlockAddr*512),fd_outputFile);
                        second_offset=second_offset+sizeof(secondIndirectBlockAddr);
                        curpos = lseek(fd, second_offset, SEEK_SET);
                        read(fd,&secondIndirectBlockAddr,sizeof(secondIndirectBlockAddr)); 
                    }
                    else
                    {
                        printf("File copied completely \n");
                        break;
                    }
                }
                first_offset=first_offset+sizeof(secondIndirectBlockAddr);
                if((secondIndirectBlockAddr==0) || (secondIndirectBlockAddr==65535))
                {
                    printf("cpout involving double indirect block completed successfully \n");
                    break;
                }
            }
            }
        }
    }
}

/**************************************************************************************
* Copies source file from fileSystem to external path
* *************************************************************************************/
copyout(char * source, char * dest)
{
    printf("cpout %s , %s", source, dest);
    int fd_outputFile;
    int sourceFileiNodeNum=isFileAlreadyExist(source);
    if (sourceFileiNodeNum != -1) //Check source file exist in file system
    {
        printf("Source directory exist in the file system. Proceeding..\n");
        fd_outputFile = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        inode new_node;
        new_node = getInodeInfoFromInodeNum(sourceFileiNodeNum);
        //check file is small or big
        if(isLargeFile(&new_node)==0)
        {
            printf("source is a small file \n");
            copyoutSmallFile(fd_outputFile,&new_node);
        }
        else
        {
            printf("Source file is large \n");
            copyoutLargeFile(fd_outputFile,&new_node);
        }
    }
    else
    {
        printf("Source directory doesnt exist in the file system. Cannot proceed..\n");
    }
}

