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