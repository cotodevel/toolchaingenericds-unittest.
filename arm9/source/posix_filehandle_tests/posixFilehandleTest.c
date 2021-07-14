#include "c_regression.h"
#include "c_partial_mock.h"
#include "opmock.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fatfslayerTGDS.h"
#include "posixHandleTGDS.h"

/*
 * Captures problem with warning about unexpected call
*/

void testVerifyTGDSPosixFilehandle_dummy_method() __attribute__ ((optnone)) {
	int param1 = 1;
	int res = -1;
	OP_ASSERT_EQUAL_INT(-1, res);
	OP_VERIFY();// should not fail the test
}



void testVerifyTGDSPosixFilehandle_fopen_fclose_method() __attribute__ ((optnone)) {
	int res = -1;
	int StructFD = OpenFileFromPathGetStructFD("0:/ToolchainGenericDS-UnitTest.nds");	//fopen -> fileno (generates internal TGDS Struct FileDescriptor index->object)
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
	OP_ASSERT_EQUAL_INT(0, res);
	OP_VERIFY(); // should pass the test
}
