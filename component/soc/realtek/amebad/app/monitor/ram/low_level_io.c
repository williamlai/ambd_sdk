#include <stdio.h>
#include "ameba_soc.h"

#if !defined (__ICCARM__)
extern u8 RAM_IMG1_VALID_PATTEN[];
void *tmp = RAM_IMG1_VALID_PATTEN;
#endif

#if defined ( __ICCARM__ )
#include <LowLevelIOInterface.h>
// Lowlevel IO redirect wrapper for IAR
#include <stdio.h>
#include <sntp/sntp.h>
#include "ff.h"

static int handle = 3;

#define MAX_FILES 8 	// 0, 1, 2 is NULL
static FIL* __fatfs_fil[MAX_FILES] = {0};

int __open(const char * filename, int mode)
{
	uint8_t mode_mapping = 0;
	uint8_t do_seekend = 0;

	if (mode & _LLIO_CREAT)
	{
		/* Create a file if it doesn't exists. */
		mode_mapping |= FA_CREATE_ALWAYS; 
		
		/* Check what we should do with it if it exists. */
		if (mode & _LLIO_APPEND)
		{
			/* Append to the existing file. */
			do_seekend = 1;
		}
		
		if (mode & _LLIO_TRUNC)
		{
			/* Truncate the existsing file. */
			mode_mapping |= FA_OPEN_ALWAYS;
		}
	}
	
	if (mode & _LLIO_TEXT)
	{
		/* The file should be opened in text form. */
	}
	else
	{
		/* The file should be opened in binary form. */
	}
	
	switch (mode & _LLIO_RDWRMASK)
	{
	case _LLIO_RDONLY:
		/* The file should be opened for read only. */
		mode_mapping |= FA_READ;
		break;
		
	case _LLIO_WRONLY:
		/* The file should be opened for write only. */
		mode_mapping |= FA_WRITE;
		break;
		
	case _LLIO_RDWR:
		/* The file should be opened for both reads and writes. */
		mode_mapping |= (FA_READ | FA_WRITE);
		break;
		
	default:
		return -1;
	}
	
	/*
	* Add the code for opening the file here.
	*/
	int free_handle = -1;
	for(int i = handle;i<MAX_FILES;i++){
		if(__fatfs_fil[i] == NULL){
			free_handle = i;
			break;
		}
	}
	
	if(free_handle == -1)	return -1;
	
	__fatfs_fil[free_handle] = (FIL *)malloc(sizeof(FIL));
	if(__fatfs_fil[free_handle]==NULL)	return -1;
	
	FRESULT res = f_open(__fatfs_fil[free_handle], filename, mode_mapping);
	if(res != 0){
		free(__fatfs_fil[free_handle]);
		__fatfs_fil[free_handle] = NULL;
		return -1;
	}
	if(do_seekend == 1) {
		f_lseek(__fatfs_fil[free_handle], f_size(__fatfs_fil[free_handle])); // Move r/w pointer to end of the file
	}
	//printf("Open file mode 0x%x\n\r", mode_mapping);
	
	return free_handle;
}

int __close(int handle)
{
	FRESULT res = f_close(__fatfs_fil[handle]);
	free(__fatfs_fil[handle]);	
	__fatfs_fil[handle] = NULL;
	
	if(res == 0)
		return 0;
	else
		return -res;
}

size_t __read(int handle, unsigned char * buffer, size_t size)
{
	
	if (handle != _LLIO_STDIN)
	{	
		// FATFS
		size_t br;
		FRESULT res = f_read(__fatfs_fil[handle], buffer, size, &br);
		if(res > 0)
			return -1;
		
		return br;		
	}else{
		// STDIN
		int nChars = 0;
		for (/* Empty */; size > 0; --size)
		{
			int c = DiagGetChar(_FALSE);
			if (c < 0)
				break;
			
			*buffer++ = c;
			++nChars;
		}
		return nChars;
	}
}

size_t __write(int handle, const unsigned char * buffer, size_t size)
{
	
	size_t nChars = 0;
	
	if (buffer == 0)
	{
		/*
		* This means that we should flush internal buffers.  Since we
		* don't we just return.  (Remember, "handle" == -1 means that all
		* handles should be flushed.)
		*/
		if( handle >= 3){
			FRESULT res = f_sync(__fatfs_fil[handle]);	
			return -res;
		}
		
		if( handle == -1){
			FRESULT res = 0;
			for(int i=3;i<MAX_FILES;i++){
				res += f_sync(__fatfs_fil[i]);
			}
			return -res;
		}
				
		return 0;
	}
	
	/* This template only writes to "standard out" and "standard err",
	* for all other file handles it returns failure. */
	if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR)
	{
		// only support size = 1byte
		size_t bw;
		
		FRESULT res = f_write(__fatfs_fil[handle], buffer, size, &bw);
		if(res > 0)
			return -1;
		
		return bw;
	}
	
	for (/* Empty */; size != 0; --size)
	{
		if(*buffer=='\n')
			DiagPutChar('\r');
		DiagPutChar(*buffer++);
		++nChars;
	}
	
	return nChars;
}

long __lseek(int handle, long offset, int whence)
{
	
	if( handle <= _LLIO_STDERR)	return 0;
	
	int size = f_size(__fatfs_fil[handle]);
	int curr = f_tell(__fatfs_fil[handle]);
	FRESULT res = -1;
	
	switch(whence){
	case SEEK_SET:
		res = f_lseek(__fatfs_fil[handle], offset);	
		break;
	case SEEK_CUR:
		res = f_lseek(__fatfs_fil[handle], curr + offset);	
		break;
	case SEEK_END:
		res = f_lseek(__fatfs_fil[handle], size - offset);
		break;
	}
	
	if(res < 0)	return 0;
	return f_tell(__fatfs_fil[handle]);
}

int remove(const char * filename)
{
	FRESULT res = f_unlink(filename);
	return -res;
}

int rename(const char *old, const char *new)
{
	FRESULT res = f_rename(old, new);	
	return -res;
}

time_t __time32 (time_t * p)
{
    unsigned int update_tick = 0;
    long update_sec = 0, update_usec = 0, current_sec = 0, current_usec = 0;
    unsigned int current_tick = xTaskGetTickCount();
    long tick_diff_sec, tick_diff_ms;

    sntp_get_lasttime(&update_sec, &update_usec, &update_tick);
    tick_diff_sec = (current_tick - update_tick) / configTICK_RATE_HZ;
    tick_diff_ms = (current_tick - update_tick) % configTICK_RATE_HZ / portTICK_RATE_MS;
    current_sec = update_sec + update_usec / 1000000;
    current_usec = update_usec % 1000000;

    return (uint32_t)current_sec;
}

#else
int _write(int file, char * ptr, int len)
{
 	int nChars = 0;
 	/* Check for stdout and stderr 
 	(only necessary if file descriptors are enabled.) */
 	if (file != 1 && file != 2)
 	{
 		return -1;
 	}
 	for (/*Empty */; len > 0; --len)
 	{
 		DiagPutChar(*ptr++);
 		++nChars;
 	}
 	return nChars;
}

int _read(int file, char * ptr, int len)
{
	int nChars = 0;
	/* Check for stdin
	(only necessary if FILE descriptors are enabled) */
	if (file != 0)
	{
		return -1;
	}
	for (/*Empty*/; len > 0; --len)
	{
		int c = DiagGetChar(_TRUE);
		if ((c < 0) || (c == '\r')) {
			*ptr = '\0';
			break;
		}
		*(ptr++) = c;
		++nChars;
	}
	return nChars;	
}
#endif
