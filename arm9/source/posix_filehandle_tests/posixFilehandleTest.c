#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "c_regression.h"
#include "c_partial_mock.h"
#include "opmock.h"
#include "fatfslayerTGDS.h"
#include "posixHandleTGDS.h"
#include "clockTGDS.h"

/*
 * Captures problem with warning about unexpected call
*/

int testPosixFilehandle_dummy_method() __attribute__ ((optnone)) {
	int param1 = 1;
	int res = -1;
	return res;
}

int testPosixFilehandle_fopen_fclose_method() __attribute__ ((optnone)) {
	int res = -1;
	char * fname = NULL;
	if(__dsimode == true){
		fname = "0:/ToolchainGenericDS-UnitTest.srl";
	}
	else{
		fname = "0:/ToolchainGenericDS-UnitTest.nds";
	}
	int StructFD = OpenFileFromPathGetStructFD(fname);	//fopen -> fileno (generates internal TGDS Struct FileDescriptor index->object)
	if(StructFD != structfd_posixInvalidFileDirOrBufferHandle){
		struct fd *pfd = getStructFD(StructFD);
		if(pfd->cur_entry.d_ino == StructFD){	
			if(closeFileFromStructFD(StructFD) == true){	//fdopen (gets FILE * handle from internal TGDS Struct FileDescriptor index) -> fclose
				//internal TGDS Struct FileDescriptor object should be invalid (structfd_posixInvalidFileDirOrBufferHandle) by now
				if(pfd->cur_entry.d_ino == StructFD){
					res = -1;	// 2/2 = Fail. Object still exists even when it was destroyed (FS bug)
				}
				else{
					res = 0; // 2/2 = OK
				}
			}
			else{
				// 2/2 = Fail. Could not close file handle
			}
		}
		else{
			// 1/2 = Fail. Couldn't open internal TGDS Struct FileDescriptor index->object
		}
	}
	return res;
}

int testPosixFilehandle_sprintf_fputs_fscanf_method() __attribute__ ((optnone)) {
	int res = -1;
	int i  =0;
	char str1[10], str2[10];
    int yr;
    FILE* fileName;
    fileName = fopen("0:/test.txt", "w+");
    struct tm * NDSClock = getTime();
	char * readWriteCharFormat = "%s %s %d";
	char outBuf[32];
	sprintf(outBuf, readWriteCharFormat,"Welcome", "to", ((sint32)NDSClock->tm_year));
	fputs(outBuf, fileName);
    rewind(fileName);
    fscanf(fileName, readWriteCharFormat, str1, str2, &yr);
    //printf("1st word %s \t", str1);	//1st word "Welcome"
    //printf("2nd word  %s \t", str2);	//2nd word  "to"
    //printf("Year-Name  %d \t", yr);	//Year-Name  "2022"
	if(
		(yr == ((sint32)NDSClock->tm_year))
	){
		int len1 = strlen(str1) + 1;
		int len2 = strlen(str2) + 1;
		if( 
			(strncmp(str1, "Welcome", len1) == 0)
			&&
			(strncmp(str2, "to", len2) == 0)
		){
			res = 0;
		}
	}
	fclose(fileName);
	return res;
}

int testPosixFilehandle_fread_fwrite_method() __attribute__ ((optnone)) {
	int res = -1;
	int i = 0;
	int bufSize = (128*1024);
	FILE* fileTest;
	char * fileNameTest = "0:/test.txt";
    fileTest = fopen(fileNameTest, "w+");
    
	u8 * bufferTestRead = TGDSARM9Malloc(bufSize);
	u8 * bufferTestWrite = TGDSARM9Malloc(bufSize);
	for(i = 0; i < bufSize; i++){
		bufferTestRead[i] = (u8)((u8)rand() & 0xFF);
	}
	
	//1: read random buffer into target file
	if(fwrite(bufferTestRead, 1, bufSize, fileTest) == bufSize){
		
		//2: close file. And read it again
		sint32 FDToSync = fileno(fileTest);
		fsync(FDToSync);
		fclose(fileTest);
		fileTest = fopen(fileNameTest, "r");
		if(fileTest != NULL){
			int readBuf = fread(bufferTestWrite, 1, bufSize, fileTest);
			if(readBuf == bufSize){
				bool correctCheck = true;
				
				for(i = 0; i < bufSize; i++){
					if(bufferTestRead[i] != bufferTestWrite[i]){
						correctCheck = false;
					}
				}
				
				if(correctCheck == true){
					res = 0;
				}
			}
			else{
				printf("failed reading complete buffer: %d bytes", readBuf);
			}
		}
	}
	else{
		printf("failed writing complete buffer");
	}
	
	fclose(fileTest);
	TGDSARM9Free(bufferTestWrite);
	TGDSARM9Free(bufferTestRead);
	return res;
}

int testPosixFilehandle_fgetc_feof_method() __attribute__ ((optnone)) {
	int param1 = 1;
	int res = -1;
	FILE *fp;
	u8 * writeBuf = TGDSARM9Malloc(256);
	int loop=0;
	char * fileNameTest = "0:/test.txt";
	char * charTest = "TGDS NTR/TWL App from coto88.bitbucket.io";
	fp = fopen(fileNameTest, "w+");
	if(fp != NULL) {
		fputs(charTest, fp);
		sint32 FDToSync = fileno(fp);
		fsync(FDToSync);
		fclose(fp);
		fp = fopen(fileNameTest, "r");
		if(fp != NULL){
			rewind(fp);
			while(1) {
				char a = fgetc(fp);
				writeBuf[loop] = a;
				if( feof(fp) ) {
					break ;
				}
				loop++;
			}
			if(strncmp((const char*)writeBuf, (char*)&charTest[0], strlen(charTest)) == 0){
				res = 0;
			}
		}
		fclose(fp);
	}
	TGDSARM9Free(writeBuf); 
	return res;
}

int testPosixFilehandle_fgets_method() __attribute__ ((optnone)) {
	int res = -1;
	FILE* tmpf = fopen("0:/test.txt", "w+");
    fputs("Alan Turing\n", tmpf);
    fputs("John von Neumann\n", tmpf);
    fputs("Alonzo Church\n", tmpf);
    rewind(tmpf);
	bool passTest = false;
    char buf[8];
    while (fgets(buf, sizeof buf, tmpf) != NULL){
		if(
			(strcmp((const char*)buf, (const char*)"Alan Tu") == 0)
			||
			(strcmp((const char*)buf, (const char*)"ring") == 0)
			||
			(strcmp((const char*)buf, (const char*)"John vo") == 0)
			||
			(strcmp((const char*)buf, (const char*)"n Neuma") == 0)
			||
			(strcmp((const char*)buf, (const char*)"nn") == 0)
			||
			(strcmp((const char*)buf, (const char*)"Alonzo\n") == 0)
			||
			(strcmp((const char*)buf, (const char*)"Church\n") == 0)
		){
			passTest = true;
		}
		else{
			passTest = false;
		}
	}
    if (feof(tmpf)){
		if(passTest == true){
			res = 0;
		}
	}
	fclose(tmpf);
	return res;
}