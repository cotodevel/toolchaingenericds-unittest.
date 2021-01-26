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

static inline void menuShow(){
	clrscr();
	printf("     ");
	printf("     ");
	printf("ToolchainGenericDS-CPPUnitTest: ");
	printf("Current file: %s ", curChosenBrowseFile);
	printf("(Select): This menu. ");
	printf("(Start): FileBrowser : (A) Play WAV/IMA-ADPCM (Intel) strm ");
	printf("(D-PAD:UP/DOWN): Volume + / - ");
	printf("(D-PAD:LEFT): GDB Debugging. >%d", TGDSPrintfColor_Green);
	printf("(D-PAD:RIGHT): Demo Sound. >%d", TGDSPrintfColor_Yellow);
	printf("(B): Stop WAV/IMA-ADPCM file. ");
	printf("Current Volume: %d", (int)getVolume());
	if(internalCodecType == SRC_WAVADPCM){
		printf("ADPCM Play: %s >%d", curChosenBrowseFile, TGDSPrintfColor_Red);
	}
	else if(internalCodecType == SRC_WAV){	
		printf("WAVPCM Play: %s >%d", curChosenBrowseFile, TGDSPrintfColor_Green);
	}
	else{
		printf("Player Inactive");
	}
	printf("(Y): Run CPPUTest. >%d", TGDSPrintfColor_Yellow);
	printf("Available heap memory: %d >%d", getMaxRam(), TGDSPrintfColor_Cyan);
	printarm7DebugBuffer();
}

static bool ShowBrowserC(char * Path, char * outBuf, bool * pendingPlay, int * curFileIdx){	//MUST be same as the template one at "fileBrowse.h" but added some custom code
	scanKeys();
	while((keysDown() & KEY_START) || (keysDown() & KEY_A) || (keysDown() & KEY_B)){
		scanKeys();
		IRQWait(IRQ_HBLANK);
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
	while(fileClassInst != NULL){
		//directory?
		if(fileClassInst->type == FT_DIR){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parseDirNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		//file?
		else if(fileClassInst->type  == FT_FILE){
			char tmpBuf[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(tmpBuf, fileClassInst->fd_namefullPath);
			parsefileNameTGDS(tmpBuf);
			strcpy(fileClassInst->fd_namefullPath, tmpBuf);
		}
		
		
		//more file/dir objects?
		fileClassInst = FAT_FindNextFile(curPath, menuIteratorfileClassListCtx);
	}
	
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
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
			}
		}
		
		else if (pressed&KEY_UP && (j > 1)) {
			j--;
			while(pressed&KEY_UP){
				scanKeys();
				pressed = keysDown();
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
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

#define ListSize (int)(3)
static char * TestList[ListSize] = {"c_partial_mock", "c_regression", "cpp_tests"};

int main(int argc, char **argv) {
	
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	printf("              ");
	printf("              ");
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Init TGDS FS Directory Iterator Context(s). Mandatory to init them like this!! Otherwise several functions won't work correctly.
	menuIteratorfileClassListCtx = initFileList();
	cleanFileList(menuIteratorfileClassListCtx);
	
	memset(globalPath, 0, sizeof(globalPath));
	strcpy(globalPath,"/");
	
	menuShow();
	
	static bool GDBEnabled = false;
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
			printf("X KEY PRESSED!");
			
			while(keysDown() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_TOUCH){
			u8 channel = SOUNDSTREAM_FREE_CHANNEL;
			startSoundSample(11025, (u32*)&click_raw[0], click_raw_size, channel, 40, 63, 1);	//PCM16 sample
			while(keysDown() & KEY_TOUCH){
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
				IRQWait(IRQ_HBLANK);
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
				IRQWait(IRQ_HBLANK);
			}
			menuShow();
		}
		
		if (keysDown() & KEY_UP){
			struct XYTscPos touchPos;
			XYReadScrPosUser(&touchPos);
			volumeUp(touchPos.touchXpx, touchPos.touchYpx);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_UP){
				scanKeys();
				IRQWait(IRQ_HBLANK);
			}
		}
		
		if (keysDown() & KEY_DOWN){
			struct XYTscPos touchPos;
			XYReadScrPosUser(&touchPos);
			volumeDown(touchPos.touchXpx, touchPos.touchYpx);
			menuShow();
			scanKeys();
			while(keysDown() & KEY_DOWN){
				scanKeys();
				IRQWait(IRQ_HBLANK);
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
		
		if (keysDown() & KEY_LEFT){
			if(GDBEnabled == false){
				GDBEnabled = true;
			}
			else{
				GDBEnabled = false;
			}
			
			while(keysDown() & KEY_LEFT){
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
		
		
		
		// TSC Test.
		struct XYTscPos touch;
		XYReadScrPosUser(&touch);
		printfCoords(0, 15, " x:%d y:%d", touch.touchXpx, touch.touchYpx);	//clean old
		
		//GDB Debug Start
		
		//GDB Stub Process must run here
		if(GDBEnabled == true){
			int retGDBVal = remoteStubMain();
			if(retGDBVal == remoteStubMainWIFINotConnected){
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Connect to GDB]: NDSMemory Mode!");
					char IP[16];
					printf("Port:%d GDB IP:%s",remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
				else{
					//GDB Client Reconnect:ERROR
				}
			}
			else if(retGDBVal == remoteStubMainWIFIConnectedGDBDisconnected){
				setWIFISetup(false);
				clrscr();
				printf("    ");
				printf("    ");
				printf("Remote GDB Client disconnected. ");
				printf("Press A to retry this GDB Session. ");
				printf("Press B to reboot NDS GDB Server ");
				
				int keys = 0;
				while(1){
					scanKeys();
					keys = keysDown();
					if (keys&KEY_A){
						break;
					}
					if (keys&KEY_B){
						break;
					}
					IRQWait(IRQ_HBLANK);
				}
				
				if (keys&KEY_B){
					main(argc, argv);
				}
				
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
					reconnectCount++;
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Re-Connect to GDB]: NDSMemory Mode!");
					char IP[16];
					printf("Retries: %d",reconnectCount);
					printf("Port:%d GDB IP:%s", remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
			}
		}
		//GDB Debug End
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQWait(IRQ_HBLANK);
	}

	return 0;
}