#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <time.h>
#include <sys/timeb.h>

extern const char* UsageHelp;

DWORD ChunkSize=2048,//256,
      RetryLimit=3,
	  ProgressBar=20,
	  UpdateInterval=750;


DWORD progress=0;
DWORD filesize=0;

void PrintFileSize(DWORD size,LPCSTR fmt="%.0f");
DWORD CALLBACK UpdateProc(LPVOID);

///////////////////////////////////////////////////////////////////////////////
int main(int argc,char *argv[])
{
	if(argc<3)
	{
		puts(UsageHelp);
		return 0;
	}

	LPSTR chunk1=new char[ChunkSize],
		  chunk2=new char[ChunkSize];

	FILE *fp;

	fp=fopen(argv[2],"rb");
	if(fp)
	{
		fclose(fp);
		printf("%s exists. Overwrite? ",argv[2]);
		overwrite_prompt:
		switch(_getche())
		{
			case 'y':
			case 'Y':
				break;
			case 'n':
			case 'N':
				delete[] chunk1; delete[] chunk2;
				return 0;
			default:
				printf("\b");
				goto overwrite_prompt;
		}
		puts("");
	}

	HANDLE hIn,hOut;

	// Input file
	hIn=CreateFile(argv[1],GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if(hIn==INVALID_HANDLE_VALUE)
	{
		printf("Error opening %s. Error code %i.",argv[1],GetLastError());
		delete[] chunk1; delete[] chunk2;
		return 0;
	}
		
	filesize=GetFileSize(hIn,0);
	printf("Input file: %s : ",argv[1]);
	PrintFileSize(filesize,"%.2f");
	puts("");

	// Output file
	hOut=CreateFile(argv[2],GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,0,0);
	if(hOut==INVALID_HANDLE_VALUE)
	{
		printf("Error opening %s. Error code %i.",argv[2],GetLastError());
		delete[] chunk1; delete[] chunk2; CloseHandle(hIn);
		return 0;
	}

	// Start copying

	DWORD written;
	DWORD read1,read2;
	DWORD x;

	DWORD Retries_R=0,
		  Retries_W=0;

	HANDLE hThread=CreateThread(0,0,UpdateProc,(LPVOID)UpdateInterval,0,0);
	SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);

	ZeroMemory(chunk1,ChunkSize);
	ZeroMemory(chunk2,ChunkSize);

	while(ReadFile(hIn,chunk1,ChunkSize,&read1,0))
	{
		SetFilePointer(hIn,-((signed)read1),0,FILE_CURRENT);
		ReadFile(hIn,chunk2,ChunkSize,&read2,0);

		if(!read1 && !read2)
			break;

		if(read1!=read2 || memcmp(chunk1,chunk2,read1))
		{
			++Retries_R;

			if(Retries_R>RetryLimit)
			{
				SuspendThread(UpdateProc);
				printf("Too many read errors on chunk! Choose an action:\n d  Details\n x  Exit\n r  Keep retrying\n i  Ignore errors\n > ");
				retries_r_prompt:
				switch(_getche())
				{
					case 'd': case 'D':
						printf("\nChunk sizes: %i, %i. Comparison to %iB: %i.",read1,read2,memcmp(chunk1,chunk2,read1));
						if(read1!=read2)
							printf(" Comparison to %iB: %i.",memcmp(chunk1,chunk2,read2));
						printf("\n > ");
						goto retries_r_prompt;
					case 'x': case 'X':
						delete[] chunk1; delete[] chunk2; CloseHandle(hIn); CloseHandle(hOut);
						return 0;
					case 'r': case 'R':
						Retries_R=0;
						continue;
					case 'i': case 'I':
						SetFilePointer(hIn,((signed)read1)-((signed)read2),0,FILE_CURRENT);
						goto post_read_errors;
					default:
						goto retries_r_prompt;
				}					
			}

			x=SetFilePointer(hIn,-((signed)read2),0,FILE_CURRENT);
			printf("Read error at %i-%i. Retrying chunk.\n",x,x+ChunkSize);

			ZeroMemory(chunk1,ChunkSize);
			ZeroMemory(chunk2,ChunkSize);
			continue;
		}
		post_read_errors:
		ResumeThread(UpdateProc);
		Retries_R=0;

		WriteFile(hOut,chunk1,read1,&written,0);
		if(written!=read1)
		{
			SuspendThread(UpdateProc);

			++Retries_W;

			if(Retries_W>RetryLimit)
			{
				printf("Too many write errors on chunk! Choose an action:\n d  Details\n x  Exit\n r  Keep retrying\n i  Ignore errors\n > ");
				retries_w_prompt:
				switch(_getche())
				{
					case 'd': case 'D':
						printf("\nGetLastError code: %i\n > ",GetLastError());
						goto retries_r_prompt;
					case 'x': case 'X':
						delete[] chunk1; delete[] chunk2; CloseHandle(hIn); CloseHandle(hOut);
						return 0;
					case 'r': case 'R':
						Retries_W=0;
						continue;
					case 'i': case 'I':
						goto post_write_errors;
					default:
						goto retries_w_prompt;
				}					
			}

			x=SetFilePointer(hOut,-((signed)written),0,FILE_CURRENT);
			SetFilePointer(hIn,-((signed)read1),0,FILE_CURRENT);
			printf("Write error at %i-%i. Retrying chunk.",x,x+ChunkSize);

			ZeroMemory(chunk1,ChunkSize);
			ZeroMemory(chunk2,ChunkSize);
			continue;
		}
		post_write_errors:
		ResumeThread(UpdateProc);
		Retries_W=0;

		progress+=written;



		ZeroMemory(chunk1,ChunkSize);
		ZeroMemory(chunk2,ChunkSize);
	}

	CloseHandle(hIn);
	CloseHandle(hOut);


	delete[] chunk1;
	delete[] chunk2;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
const char *FileSizes[]={ "B", "KB", "MB", "GB", "TB", 0 };
void PrintFileSize(DWORD size,LPCSTR fmt)
{
	double s=(double)size;
	for(int x=0;FileSizes[x+1] && s>1024.0;++x)
		s/=1024.0;

	printf(fmt,s);
	printf(" %s",FileSizes[x]);
}

///////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK UpdateProc(LPVOID sleeptime)
{
	float f;
	_timeb tmstr;
	int x;
	DWORD lastClock=0;
	DWORD lastSize=0;

	for(;;)
	{
		Sleep((int)sleeptime);


		printf("\r[");

		f=(float)progress;
		f/=(float)filesize;
		f*=(float)ProgressBar;
		for(x=(int)f;x>=0;--x)
			printf("%c",0xB2);
		
		for(x=(int)f;x<(int)ProgressBar;++x)
			printf(" ");

		printf("] ");

		if(progress>=filesize)
			printf("DONE : ");
		else
			printf("%02.0f %% : ",(((float)progress/(float)filesize)*100.0));
		PrintFileSize(progress,"%.2f");
		printf("/");
		PrintFileSize(filesize,"%.2f");
		printf(" : ");

		_ftime(&tmstr);

		if(!lastClock)
		{
			printf("???? B/s");
			lastClock=tmstr.time*1000+tmstr.millitm;
			lastSize=progress;
			////lastClock=time(0);
		}
		else
		{
			///////printf("%i-%i; %i/%i=%f",time(0),lastClock,progress,time(0)-lastClock,((float)progress)/((float)(time(0)-lastClock)));
			f=(float)(tmstr.time*1000+tmstr.millitm);
			f-=(float)lastClock;
			f/=1000.0;
			////f=(float)(time(0)-lastClock);
			//f=((float)written)/f;
			f=((float)(progress-lastSize))/f;
			//printf("%f",f);
			if(int(f))
			{
				PrintFileSize((int)f,"%.2f");
				f=((float)(filesize-progress))/f;
				printf("/s %i:%02i  \r",(int)(f/60),(int)f%60);
			}

			lastClock=tmstr.time*1000+tmstr.millitm;
			lastSize=progress;
		}
	}

	return 0;
}