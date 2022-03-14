/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include "main.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"
#include "opmock.h"
#include "c_partial_mock_test.h"
#include "c_regression.h"
#include "cpptests.h"
#include "libndsFIFO.h"
#include "xenofunzip.h"
#include "cartHeader.h"
#include "VideoGL.h"
#include "VideoGLExt.h"
#include "videoTGDS.h"
#include "math.h"
#include "posixFilehandleTest.h"
#include "loader.h"
#include "ndsDisplayListUtils.h"

//true: pen touch
//false: no tsc activity
static bool get_pen_delta( int *dx, int *dy ){
	static int prev_pen[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
	
	// TSC Test.
	struct touchPosition touch;
	XYReadScrPosUser(&touch);
	
	if( (touch.px == 0) && (touch.py == 0) ){
		prev_pen[0] = prev_pen[1] = 0x7FFFFFFF;
		*dx = *dy = 0;
		return false;
	}
	else{
		if( prev_pen[0] != 0x7FFFFFFF ){
			*dx = (prev_pen[0] - touch.px);
			*dy = (prev_pen[1] - touch.py);
		}
		prev_pen[0] = touch.px;
		prev_pen[1] = touch.py;
	}
	return true;
}

struct FileClassList * menuIteratorfileClassListCtx = NULL;
char curChosenBrowseFile[256+1];
char globalPath[MAX_TGDSFILENAME_LENGTH+1];
static int curFileIndex = 0;
static bool pendingPlay = false;

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM
static struct fd * _FileHandleVideo = NULL; 
static struct fd * _FileHandleAudio = NULL;

bool stopSoundStreamUser(){
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

GLuint	texture[1];			// Storage For 1 Texture
GLuint	box;				// Storage For The Box Display List
GLuint	top;				// Storage For The Top Display List
GLuint	xloop;				// Loop For X Axis
GLuint	yloop;				// Loop For Y Axis

GLfloat	xrot;				// Rotates Cube On The X Axis
GLfloat	yrot;				// Rotates Cube On The Y Axis

 GLfloat boxcol[5][3]=
{
	{1.0f,0.0f,0.0f},{1.0f,0.5f,0.0f},{1.0f,1.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,1.0f,1.0f}
};

GLfloat topcol[5][3]=
{
	{.5f,0.0f,0.0f},{0.5f,0.25f,0.0f},{0.5f,0.5f,0.0f},{0.0f,0.5f,0.0f},{0.0f,0.5f,0.5f}
};

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("ToolchainGenericDS-CPPUnitTest: ");
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(Y): Run CPPUTest. >%d", TGDSPrintfColor_Yellow);
	printf("(X): Test libnds FIFO subsystem. >%d", TGDSPrintfColor_Yellow);
	printf("(Touch): Move Simple 3D Triangle. >%d", TGDSPrintfColor_Cyan);
	
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
}

static bool ShowBrowserC(char * Path, char * outBuf, bool * pendingPlay, int * curFileIdx){	//MUST be same as the template one at "fileBrowse.h" but added some custom code
	scanKeys();
	while((keysDown() & KEY_START) || (keysDown() & KEY_A) || (keysDown() & KEY_B)){
		scanKeys();
		IRQWait(0, IRQ_VBLANK);
	}
	
	*pendingPlay = false;
	
	//Create TGDS Dir API context
	cleanFileList(menuIteratorfileClassListCtx);
	
	//Use TGDS Dir API context
	int pressed = 0;
	struct FileClass filStub;
	{
		filStub.type = FT_FILE;
		strcpy(filStub.fd_namefullPath, "");
		filStub.isIterable = true;
		filStub.d_ino = -1;
		filStub.parentFileClassList = menuIteratorfileClassListCtx;
	}
	char curPath[MAX_TGDSFILENAME_LENGTH+1];
	strcpy(curPath, Path);
	
	if(pushEntryToFileClassList(true, filStub.fd_namefullPath, filStub.type, -1, menuIteratorfileClassListCtx) != NULL){
		
	}
	
	int j = 1;
	int startFromIndex = 1;
	*curFileIdx = startFromIndex;
	struct FileClass * fileClassInst = NULL;
	fileClassInst = FAT_FindFirstFile(curPath, menuIteratorfileClassListCtx, startFromIndex);
	
	//Sort list alphabetically
	bool ignoreFirstFileClass = true;
	sortFileClassListAsc(menuIteratorfileClassListCtx, (char**)ARM7_PAYLOAD, ignoreFirstFileClass);
	
	//actual file lister
	clrscr();
	
	j = 1;
	pressed = 0 ;
	int lastVal = 0;
	bool reloadDirA = false;
	bool reloadDirB = false;
	char * newDir = NULL;
	
	#define itemsShown (int)(15)
	int curjoffset = 0;
	int itemRead=1;
	
	while(1){
		int fileClassListSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx) + 1;	//+1 the stub
		int itemsToLoad = (fileClassListSize - curjoffset);
		
		//check if remaining items are enough
		if(itemsToLoad > itemsShown){
			itemsToLoad = itemsShown;
		}
		
		while(itemRead < itemsToLoad ){		
			if(getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
				printfCoords(0, itemRead, "--- %s >%d",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath, TGDSPrintfColor_Yellow);
			}
			else{
				printfCoords(0, itemRead, "--- %s",getFileClassFromList(itemRead+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
			}
			itemRead++;
		}
		
		scanKeys();
		pressed = keysDown();
		if (pressed&KEY_DOWN && (j < (itemsToLoad - 1) ) ){
			j++;
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//downwards: means we need to reload new screen
		else if(pressed&KEY_DOWN && (j >= (itemsToLoad - 1) ) && ((fileClassListSize - curjoffset - itemRead) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_DOWN){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//LEFT, reload new screen
		else if(pressed&KEY_LEFT && ((curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset - itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_LEFT){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//RIGHT, reload new screen
		else if(pressed&KEY_RIGHT && ((fileClassListSize - curjoffset - itemsToLoad) > 0) ){
			
			//list only the remaining items
			clrscr();
			
			curjoffset = (curjoffset + itemsToLoad - 1);
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_RIGHT){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		else if (pressed&KEY_UP && (j > 1)) {
			j--;
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//upwards: means we need to reload new screen
		else if (pressed&KEY_UP && (j <= 1) && (curjoffset > 0) ) {
			//list only the remaining items
			clrscr();
			
			curjoffset--;
			itemRead = 1;
			j = 1;
			
			scanKeys();
			pressed = keysDown();
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysDown();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		//reload DIR (forward)
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR) ){
			struct FileClass * fileClassChosen = getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx);
			newDir = fileClassChosen->fd_namefullPath;
			reloadDirA = true;
			break;
		}
		
		//file chosen
		else if( (pressed&KEY_A) && (getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_FILE) ){
			break;
		}
		
		//reload DIR (backward)
		else if(pressed&KEY_B){
			reloadDirB = true;
			break;
		}
		
		// Show cursor
		printfCoords(0, j, "*");
		if(lastVal != j){
			printfCoords(0, lastVal, " ");	//clean old
		}
		lastVal = j;
	}
	
	//enter a dir
	if(reloadDirA == true){
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		enterDir((char*)newDir, Path);
		return true;
	}
	
	//leave a dir
	if(reloadDirB == true){
		//Free TGDS Dir API context
		//freeFileList(menuIteratorfileClassListCtx);	//can't because we keep the menuIteratorfileClassListCtx handle across folders
		
		//rewind to preceding dir in TGDSCurrentWorkingDirectory
		leaveDir(Path);
		return true;
	}
	
	strcpy((char*)outBuf, getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->fd_namefullPath);
	clrscr();
	printf("                                   ");
	if(getFileClassFromList(j+curjoffset, menuIteratorfileClassListCtx)->type == FT_DIR){
		//printf("you chose Dir:%s",outBuf);
	}
	else{
		*curFileIdx = (j+curjoffset);
		*pendingPlay = true;
	}
	
	//Free TGDS Dir API context
	//freeFileList(menuIteratorfileClassListCtx);
	return false;
}

#define ListSize (int)(6)
static char * TestList[ListSize] = {"c_partial_mock", "c_regression", "cpp_tests", "posix_filehandle_tests", "ndsDisplayListUtils_tests", "argv_chainload_test"};

static void returnMsgHandler(int bytes, void* user_data);

//Should ARM7 reply a message, then, it'll be received to test the Libnds FIFO implementation.
static void returnMsgHandler(int bytes, void* user_data)
{
	returnMsg msg;
	fifoGetDatamsg(FIFO_RETURN, bytes, (u8*) &msg);
	printf("ARM7 reply size: %d returnMsgHandler: ", bytes);
	printf("%s >%d", (char*)&msg.data[0], TGDSPrintfColor_Green);
}

static void InstallSoundSys()
{
	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_RETURN, returnMsgHandler, 0);
}

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
int TGDSProjectReturnFromLinkedModule() {
	return -1;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	//xmalloc init removes args, so save them
	int i = 0;
	for(i = 0; i < argc; i++){
		argvs[i] = argv[i];
	}

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	//argv destroyed here because of xmalloc init, thus restore them
	for(i = 0; i < argc; i++){
		argv[i] = argvs[i];
	}

	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else{
		printf("FS Init error: %d", ret);
	}
	
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	InstallSoundSys();
	
	//Init TGDS FS Directory Iterator Context(s). Mandatory to init them like this!! Otherwise several functions won't work correctly.
	menuIteratorfileClassListCtx = initFileList();
	cleanFileList(menuIteratorfileClassListCtx);
	
	memset(globalPath, 0, sizeof(globalPath));
	strcpy(globalPath,"/");
	
	menuShow();
	
	//Simple Triangle GL init
	float rotateX = 0.0;
	float rotateY = 0.0;
	{
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewport(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
	}
	
	ReSizeGLScene(255, 191);
	InitGL();

	while(1) {
		if(pendingPlay == true){
			internalCodecType = playSoundStream(curChosenBrowseFile, _FileHandleVideo, _FileHandleAudio);
			if(internalCodecType == SRC_NONE){
				stopSoundStreamUser();
			}
			pendingPlay = false;
			menuShow();
		}
		
		scanKeys();
		
		if (keysDown() & KEY_X){
			
			returnMsg msg;
			sprintf((char*)&msg.data[0], "%s", "Test message using libnds FIFO API!");
			if(fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg) == true){
				//printf("X: Channel: %d Message Sent! ", FIFO_RETURN);
			}
			else{
				//printf("X: Channel: %d Message FAIL! ", FIFO_RETURN);
			}
			
			while(keysDown() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_SELECT){
			menuShow();
			while(keysDown() & KEY_SELECT){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_START){
			while( ShowBrowserC((char *)globalPath, (char *)&curChosenBrowseFile[0], &pendingPlay, &curFileIndex) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				strcpy(curChosenBrowseFile, (const char *)getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx)->fd_namefullPath);
				pendingPlay = true;
			}
			
			scanKeys();
			while(keysDown() & KEY_START){
				scanKeys();
			}
			menuShow();
		}
		
		if (keysDown() & KEY_L){
			struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
			if(getCurrentDirectoryCount(menuIteratorfileClassListCtx) > 0){
				if(curFileIndex > 1){	//+1 stub FileClass
					curFileIndex--;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysDown() & KEY_L){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
			menuShow();
		}
		
		if (keysDown() & KEY_R){	
			//Play next song from current folder
			int lstSize = getCurrentDirectoryCount(menuIteratorfileClassListCtx);
			if(lstSize > 0){	
				if(curFileIndex < lstSize){
					curFileIndex++;
				}
				struct FileClass * fileClassInst = getFileClassFromList(curFileIndex, menuIteratorfileClassListCtx);
				if(fileClassInst->type == FT_FILE){
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
					pendingPlay = true;
				}
				else{
					strcpy(curChosenBrowseFile, (const char *)fileClassInst->fd_namefullPath);
				}
			}
			
			scanKeys();
			while(keysDown() & KEY_R){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
			menuShow();
		}
		
		if (keysDown() & KEY_UP){
			struct touchPosition touchPos;
			XYReadScrPosUser(&touchPos);
			volumeUp(touchPos.px, touchPos.py);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_UP){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		if (keysDown() & KEY_DOWN){
			struct touchPosition touchPos;
			XYReadScrPosUser(&touchPos);
			volumeDown(touchPos.px, touchPos.py);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_DOWN){
				scanKeys();
				IRQWait(0, IRQ_VBLANK);
			}
		}
		
		if (keysDown() & KEY_B){
			scanKeys();
			stopSoundStreamUser();
			menuShow();
			while(keysDown() & KEY_B){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_RIGHT){
			strcpy(curChosenBrowseFile, (const char *)"0:/rain.ima");
			pendingPlay = true;
			
			scanKeys();
			while(keysDown() & KEY_RIGHT){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_Y){
			
			//Choose the test
			clrscr();
			
			int testIdx = 0;
			while(1==1){
				scanKeys();
				
				while(keysDown() & KEY_UP){
					if(testIdx < (ListSize-1)){
						testIdx++;
					}
					scanKeys();
					while(!(keysUp() & KEY_UP)){
						scanKeys();
					}
					printfCoords(0, 10, "                                                    ");	//clean old
					printfCoords(0, 11, "                                                    ");
				}
				
				while(keysDown() & KEY_DOWN){
					if(testIdx > 0){
						testIdx--;
					}
					scanKeys();
					while(!(keysUp() & KEY_DOWN)){
						scanKeys();
					}
					printfCoords(0, 10, "                                                    ");	//clean old
					printfCoords(0, 11, "                                                    ");
				}
				
				if(keysDown() & KEY_A){
					break;
				}
				
				printfCoords(0, 10, "Which Test? [%s] >%d", TestList[testIdx], TGDSPrintfColor_Yellow);
				printfCoords(0, 11, "Press (A) to start test");
			}
			
			switch(testIdx){
				case (0):{ //c_partial_mock
					opmock_test_suite_reset();
					opmock_register_test(test_fizzbuzz_with_15, "test_fizzbuzz_with_15");
					opmock_register_test(test_fizzbuzz_many_3, "test_fizzbuzz_many_3");
					opmock_register_test(test_fizzbuzz_many_5, "test_fizzbuzz_many_5");
					opmock_test_suite_run();
				}
				break;
				case (1):{ //c_regression
					opmock_test_suite_reset();
					opmock_register_test(test_push_pop_stack, "test_push_pop_stack");
					opmock_register_test(test_push_pop_stack2, "test_push_pop_stack2");
					opmock_register_test(test_push_pop_stack3, "test_push_pop_stack3");
					opmock_register_test(test_push_pop_stack4, "test_push_pop_stack4");
					opmock_register_test(test_verify, "test_verify");
					opmock_register_test(test_verify_with_matcher_cstr, "test_verify_with_matcher_cstr");
					opmock_register_test(test_verify_with_matcher_int, "test_verify_with_matcher_int");
					opmock_register_test(test_verify_with_matcher_float, "test_verify_with_matcher_float");
					opmock_register_test(test_verify_with_matcher_custom, "test_verify_with_matcher_custom");
					opmock_register_test(test_cmp_ptr_with_typedef, "test_cmp_ptr_with_typedef");
					opmock_register_test(test_cmp_ptr_with_typedef_fail, "test_cmp_ptr_with_typedef_fail");	
					opmock_test_suite_run();
				}
				break;
				case (2):{ //cpp_tests
					doCppTests(argc, argv);
				}
				break;
				case (3):{ //posix_filehandle_tests
					clrscr();
					printf(" - - ");
					printf(" - - ");
					printf(" - - ");
					
					if(testPosixFilehandle_fopen_fclose_method() == 0){
						printf("testPosixFilehandle_fopen_fclose_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fopen_fclose_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_sprintf_fputs_fscanf_method() == 0){
						printf("testPosixFilehandle_sprintf_fputs_fscanf_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_sprintf_fputs_fscanf_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fread_fwrite_method() == 0){
						printf("testPosixFilehandle_fread_fwrite_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fread_fwrite_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fgetc_feof_method() == 0){
						printf("testPosixFilehandle_fgetc_feof_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fgetc_feof_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fgets_method() == 0){
						printf("testPosixFilehandle_fgets_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fgets_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
					if(testPosixFilehandle_fseek_rewind_method() == 0){
						printf("testPosixFilehandle_fseek_rewind_method() OK >%d", TGDSPrintfColor_Green);
					}
					else{
						printf("testPosixFilehandle_fseek_rewind_method() ERROR >%d", TGDSPrintfColor_Red);
					}
					
				}
				break;
				case (4):{ 
					//Unit Test: #0
					//ndsDisplayListUtils_tests: Nintendo DS reads embedded and compiled Cube.bin binary (https://bitbucket.org/Coto88/blender-nds-exporter/src) from filesystem
					//which generates a NDS GX Display List object, then it gets compiled again into a binary DisplayList.
					bool res = ndsDisplayListUtilsTestCaseARM9("0:/Cube_test.bin", "0:/Cube_compiled.bin");
					clrscr();
					printf(" - - ");
					printf(" - - ");
					printf(" - - ");
					
					if(res != true){
						printf("ndsDisplayListUtilsTestCaseARM9 ERROR >%d", TGDSPrintfColor_Red);
						printf("Cube_test.bin missing from SD root path >%d", TGDSPrintfColor_Red);
					}
					else{
						printf("ndsDisplayListUtilsTestCaseARM9 OK >%d", TGDSPrintfColor_Green);
					}
					
					//Unit Test #1: Tests OpenGL DisplayLists components functionality then emitting proper GX displaylists, unpacked format.
					GLInitExt();
					int list = glGenLists(10);
					if(list){
						glListBase(list);
						bool ret = glIsList(list); //should return false (DL generated, but no displaylist-name was generated)
						glNewList(list, GL_COMPILE);
						ret = glIsList(list); //should return true (DL generated, and displaylist-name was generated)
						if(ret == true){
							for (int i = 0; i <10; i ++){ //Draw 10 cubes
								glPushMatrix();
								glRotatef(36*i,0.0,0.0,1.0);
								glTranslatef((float)10.0, (float)0.0, (float)0.0);
								glPopMatrix(1);
							}
						}
						glEndList();
						
						glListBase(list + 1);
						glNewList (list + 1, GL_COMPILE);//Create a second display list and execute it
						ret = glIsList(list + 1); //should return true (DL generated, and displaylist-name was generated)
						if(ret == true){
							for (int i = 0; i <20; i ++){ //Draw 20 triangles
								glPushMatrix();
								glRotatef(18*i,0.0,0.0,1.0);
								glTranslatef((float)15.0, (float)0.0, (float)0.0);
								glPopMatrix(1);
							}
						}
						glEndList();//The second display list is created
					}
					
					u32 * CompiledDisplayListsBuffer = getInternalDisplayListBuffer(); //Lists called earlier are written to this buffer, using the unpacked GX command format.
					//Unit Test #2:
					//Takes an unpacked format display list, gets converted into packed format then exported as C Header file source code
					char cwdPath[256];
					char outPath[256];
					strcpy(cwdPath, "0:/");
					sprintf(outPath, "%s%s", cwdPath, "PackedDisplayList.h");
					bool result = packAndExportSourceCodeFromRawUnpackedDisplayListFormat(outPath, CompiledDisplayListsBuffer);
					if(result == true){
						printf("OK:Unpacked DisplayList PACKED and exported as C source: >%d", TGDSPrintfColor_Green);
						printf("%s", outPath);
					}
					else{
						printf("Unpacked Display List generation failure >%d", TGDSPrintfColor_Red);
					}
					
					//Unit Test #3: Using rawUnpackedToRawPackedDisplayListFormat() in target platform builds a packed Display List from an unpacked one IN MEMORY.
					//Resembles the same behaviour as if C source file generated in Unit Test #2 was then built through ToolchainGenericDS and embedded into the project.
					u32 Packed_DL_Binary[2048/4];
					memset(Packed_DL_Binary, 0, sizeof(Packed_DL_Binary));
					bool result2 = rawUnpackedToRawPackedDisplayListFormat(CompiledDisplayListsBuffer, (u32*)&Packed_DL_Binary[0]);
					if(result2 == true){
						//Save to file
						sprintf(outPath, "%s%s", cwdPath, "PackedDisplayListCompiled.bin");
						FILE * fout = fopen(outPath, "w+");
						if(fout != NULL){
							int packedSize = Packed_DL_Binary[0];
							int written = fwrite((u8*)&Packed_DL_Binary[0], 1, packedSize, fout);
							printf("OK:Unpacked DisplayList PACKED and exported as GX binary: >%d", TGDSPrintfColor_Green);
							printf("Written: %d bytes", packedSize);
							fclose(fout);
						}
					}
					else{
						printf("Unpacked Display List generation failure >%d", TGDSPrintfColor_Red);
					}
					
					//Unit Test #4: glCallLists test
					GLuint index = glGenLists(10);  // create 10 display lists
					GLubyte lists[10];              // allow maximum 10 lists to be rendered
					
					//Init glCallLists
					int DLOffset = 0;
					for(DLOffset = 0; DLOffset < 10; DLOffset++){
						lists[DLOffset] = (GLubyte)-1;
					}
					//Compile 5 display lists
					for(DLOffset = 0; DLOffset < 5; DLOffset++){
						glNewList(index + DLOffset, GL_COMPILE);   // compile each one until the 10th
						glEndList();
					}
					
					// draw odd placed display lists names only (1st, 3rd, 5th, 7th, 9th)
					lists[0]=0; lists[1]=2; lists[2]=4; lists[3]=6; lists[4]=8;
					
					glListBase(index);              // set base offset
					glCallLists(10, GL_UNSIGNED_BYTE, lists); //only OpenGL Display List names set earlier will run!

					//Unit Test #5: glDeleteLists test
					glDeleteLists(index, 5); //remove 5 of them
				}
				break;
				
				case(5):{
					//argv_chainload_test: Chainloads through toolchaingenericds-multiboot into toolchaingenericds-argvtest while passing an argument to it.
					//Case use: Booting TGDS homebrew through TGDS-Multiboot in external loaders, and passing arguments to said TGDS homebrew
					
					//Default case use
					char * TGDS_MB = NULL;
					char * TGDS_ARGVTEST = NULL;
					char * TGDS_UNITTEST = NULL;
					if(__dsimode == true){
						TGDS_UNITTEST = "0:/ToolchainGenericDS-UnitTest.srl";
						TGDS_MB = "0:/ToolchainGenericDS-multiboot.srl";
						TGDS_ARGVTEST = "0:/ToolchainGenericDS-argvtest.srl";
					}
					else{
						TGDS_UNITTEST = "0:/ToolchainGenericDS-UnitTest.nds";
						TGDS_MB = "0:/ToolchainGenericDS-multiboot.nds";
						TGDS_ARGVTEST = "0:/ToolchainGenericDS-argvtest.nds";
					}
					char thisArgv[4][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], TGDS_UNITTEST);	//Arg0:	This Binary loaded
					strcpy(&thisArgv[1][0], TGDS_MB);	//Arg1:	NDS Binary to chainload through TGDS-MB
					strcpy(&thisArgv[2][0], TGDS_ARGVTEST);	//Arg2: NDS Binary loaded from TGDS-MB	
					strcpy(&thisArgv[3][0], "thisArgumentShouldBeSeenInTGDS-argvtest.bin");					//Arg3: NDS Binary loaded from TGDS-MB's     ARG0
					addARGV(4, (char*)&thisArgv);
					strcpy(curChosenBrowseFile, TGDS_MB);
					
					
					//Snemulds chainloading implementation
					/*
					char * TGDS_MB = NULL;
					char * TGDS_ARGVTEST = NULL;
					char * TGDS_UNITTEST = NULL;
					if(__dsimode == true){
						TGDS_UNITTEST = "0:/ToolchainGenericDS-UnitTest.srl";
						TGDS_MB = "0:/ToolchainGenericDS-multiboot.srl";
						TGDS_ARGVTEST = "0:/SNEmulDS.srl";
					}
					else{
						TGDS_UNITTEST = "0:/ToolchainGenericDS-UnitTest.nds";
						TGDS_MB = "0:/ToolchainGenericDS-multiboot.nds";
						TGDS_ARGVTEST = "0:/SNEmulDS.nds";
					}
					
					char thisArgv[4][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], TGDS_UNITTEST);	//Arg0:	This Binary loaded
					strcpy(&thisArgv[1][0], TGDS_MB);	//Arg1:	NDS Binary to chainload through TGDS-MB
					strcpy(&thisArgv[2][0], TGDS_ARGVTEST);	//Arg2: NDS Binary loaded from TGDS-MB	
					strcpy(&thisArgv[3][0], "0:/snes/filename.sfc");					//Arg3: NDS Binary loaded from TGDS-MB's     ARG0
					addARGV(4, (char*)&thisArgv);
					strcpy(curChosenBrowseFile, TGDS_MB);
					*/
					
					if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
						printf("Invalid NDS/TWL Binary >%d", TGDSPrintfColor_Yellow);
						printf("or you are in NTR mode trying to load a TWL binary. >%d", TGDSPrintfColor_Yellow);
						printf("or you are missing the TGDS-multiboot payload in root path. >%d", TGDSPrintfColor_Yellow);
						printf("Press (A) to continue. >%d", TGDSPrintfColor_Yellow);
						while(1==1){
							scanKeys();
							if(keysDown()&KEY_A){
								scanKeys();
								while(keysDown() & KEY_A){
									scanKeys();
								}
								break;
							}
						}
						menuShow();
					}
				}
				break;
			}
			
			
			printf("Tests done. Press (A) to exit.");
			while(1==1){
				scanKeys();	
				if(keysDown() & KEY_A){
					break;
				}	
			}
			menuShow();
		}
		
		/*
		int pen_delta[2];
		bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
		rotateY -= pen_delta[0];
		rotateX -= pen_delta[1];
		
		if( isTSCActive == false ){
			printfCoords(0, 16, " No TSC Activity ----");
		}
		else{
			
			printfCoords(0, 16, "TSC Activity ----");
			
			glReset();
	
			//any floating point gl call is being converted to fixed prior to being implemented
			gluPerspective(35, 256.0 / 192.0, 0.1, 40);

			gluLookAt(	0.0, 0.0, 1.0,		//camera possition 
						0.0, 0.0, 0.0,		//look at
						0.0, 1.0, 0.0);		//up

			glPushMatrix();

			//move it away from the camera
			glTranslate3f32(0, 0, floattof32(-1));
					
			glRotateX(rotateX);
			glRotateY(rotateY);			
			glMatrixMode(GL_MODELVIEW);
			
			//not a real gl function and will likely change
			glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);
			
			//glShadeModel(GL_FLAT); //forces the fill color to be the first glColor3b call
			
			//draw the obj
			glBegin(GL_TRIANGLE);
				
				glColor3b(31,0,0);
				glVertex3v16(inttov16(-1),inttov16(-1),0);

				glColor3b(0,31,0);
				glVertex3v16(inttov16(1), inttov16(-1), 0);

				glColor3b(0,0,31);
				glVertex3v16(inttov16(0), inttov16(1), 0);
				
			glEnd();
			glPopMatrix(1);
			glFlush();
		}
		*/

		DrawGLScene();
		if (keysDown() & KEY_LEFT)
		{
			yrot-=0.2f;
		}
		if (keysDown() & KEY_RIGHT)
		{
			yrot+=0.2f;
		}
		if (keysDown() & KEY_UP)
		{
			xrot-=0.2f;
		}
		if (keysDown() & KEY_DOWN)
		{
			xrot+=0.2f;
		}

		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}

	return 0;
}


//GL Display Lists Unit Test: Cube Demo
// Build Cube Display Lists
GLvoid BuildLists(){
	box=glGenLists(2);									// Generate 2 Different Lists
	glNewList(box,GL_COMPILE);							// Start With The Box List
		glBegin(GL_QUADS);
			// Bottom Face
			glNormal3f( 0.0f,-1.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			// Front Face
			glNormal3f( 0.0f, 0.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			// Back Face
			glNormal3f( 0.0f, 0.0f,-1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			// Right face
			glNormal3f( 1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			// Left Face
			glNormal3f(-1.0f, 0.0f, 0.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();
	top=box+1;											// Storage For "Top" Is "Box" Plus One
	glNewList(top,GL_COMPILE);							// Now The "Top" Display List
		glBegin(GL_QUADS);
			// Top Face
			glNormal3f( 0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

int InitGL()										// All Setup For OpenGL Goes Here
{
	BuildLists();										// Jump To The Code That Creates Our Display Lists

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glClearColor(0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	return true;										// Initialization Went OK
}

int DrawGLScene()									// Here's Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer

	for (yloop=1;yloop<6;yloop++)
	{
		for (xloop=0;xloop<yloop;xloop++)
		{
			glLoadIdentity();							// Reset The View
			glTranslatef(1.4f+(float(xloop)*2.8f)-(float(yloop)*1.4f),((6.0f-float(yloop))*2.4f)-7.0f,-20.0f);
			glRotatef(45.0f-(2.0f*yloop)+xrot,1.0f,0.0f,0.0f);
			glRotatef(45.0f+yrot,0.0f,1.0f,0.0f);
			glColor3fv(boxcol[yloop-1]);
			glCallList(box);
			glColor3fv(topcol[yloop-1]);
			glCallList(top);
		}
	}
	return true;										// Keep Going
}