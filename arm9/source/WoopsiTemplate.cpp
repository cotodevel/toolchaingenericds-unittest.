// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"
#include "posixHandleTGDS.h"
#include "keypadTGDS.h"
#include "biosTGDS.h"
#include "dswnifi_lib.h"
#include "clockTGDS.h"
#include "microphoneShared.h"
#include "lzss9.h"
#include "dldi.h"
#include "ipcfifoTGDSUser.h"
#include "libndsFIFO.h"
#include "TGDSLogoLZSSCompressed.h"
#include "opmock.h"
#include "c_partial_mock.h"
#include "c_partial_mock_test.h"
#include "c_regression.h"
#include "cpptests.h"
#include "posixFilehandleTest.h"
#include "VideoGL.h"
#include "ndsDisplayListUtils.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <_ansi.h>
#include <reent.h>
#include "wifi_arm9.h"
#include "fatfslayerTGDS.h"
#include "Texture_Cube.h"
#include "VideoGL.h"
#include "consoleTGDS.h"
#include "imagepcx.h"

//C++ part
using namespace std;
//#include <random>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <cmath>

char somebuf[frameDSsize];	//use frameDSsize as the sender buffer size, any other size won't be sent.

__attribute__((section(".dtcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int printfWoopsi(const char *fmt, ...){
	//Indentical Implementation as GUI_printf
	va_list args;
	va_start (args, fmt);
	vsnprintf ((sint8*)ConsolePrintfBuf, (int)sizeof(ConsolePrintfBuf), fmt, args);
	va_end (args);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText((char*)ConsolePrintfBuf);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
string ToStr( char c ) {
   return string( 1, c );
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
std::string OverrideFileExtension(const std::string& FileName, const std::string& newExt)
{
	std::string newString = string(FileName);
    if(newString.find_last_of(".") != std::string::npos)
        return newString.substr(0,newString.find_last_of('.'))+newExt;
    return "";
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::startup(int argc, char **argv) {
	
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	_controlsScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH);
	woopsiApplication->addGadget(_controlsScreen);
	_controlsScreen->setPermeable(true);

	//Nifi Controls
	{
		AmigaWindow* _controlWindow = new AmigaWindow(0, 13, 256, 33, "Nifi Tests", Gadget::GADGET_DECORATION, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
		_controlsScreen->addGadget(_controlWindow);
		_controlWindow->getClientRect(rect);
		_Idle = new Button(rect.x, rect.y, 62, 16, "Idle Nifi");	//_Idle->disable();
		_Idle->setRefcon(2);
		_controlWindow->addGadget(_Idle);
		_Idle->addGadgetEventHandler(this);
		_udpNifi = new Button(rect.x + 62, rect.y, 62, 16, "Udp Nifi");
		_udpNifi->setRefcon(3);
		_controlWindow->addGadget(_udpNifi);
		_udpNifi->addGadgetEventHandler(this);
		_localNifi = new Button(rect.x + 62 + 62, rect.y, 62, 16, "Local Nifi");
		_localNifi->setRefcon(4);
		_controlWindow->addGadget(_localNifi);
		_localNifi->addGadgetEventHandler(this);
		_sendNifiMsg = new Button(rect.x + 62 + 62 + 62, rect.y, 62, 16, "Send Msg");
		_sendNifiMsg->setRefcon(5);
		_controlWindow->addGadget(_sendNifiMsg);
		_sendNifiMsg->addGadgetEventHandler(this);
		
		//Create UDP Nifi IP textbox and get Button Handle
		_udpNifiMsgBox = new Alert(2, 2, 240, 160, "Enter Udp Nifi IP", "192.168.1.1");	//preset URL
		_controlsScreen->addGadget(_udpNifiMsgBox);
		Button * AlertOKButtonHandle = _udpNifiMsgBox->getButtonHandle();
		AlertOKButtonHandle->setRefcon(6);
		AlertOKButtonHandle->addGadgetEventHandler(this);
		_udpNifiMsgBox->hide();	
	}
	
	//SDK Controls
	{
		AmigaWindow* _controlWindow2 = new AmigaWindow(0, 48, 256, 33, "SDK Operations", Gadget::GADGET_DECORATION, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
		_controlsScreen->addGadget(_controlWindow2);
		_controlWindow2->getClientRect(rect);
		_lzssCompress = new Button(rect.x, rect.y, 100, 16, "LZSS Compress");
		_lzssCompress->setRefcon(7);
		_controlWindow2->addGadget(_lzssCompress);
		_lzssCompress->addGadgetEventHandler(this);
		
		_lzssDecompress = new Button(rect.x + 100, rect.y, 110, 16, "LZSS Decompress");
		_lzssDecompress->setRefcon(8);
		_controlWindow2->addGadget(_lzssDecompress);
		_lzssDecompress->addGadgetEventHandler(this);
		
		//file browser for various duties
		_fileReq = new FileRequester(7, 10, 250, 150, "Files", "/", GADGET_DRAGGABLE | GADGET_DOUBLE_CLICKABLE);
		_fileReq->setRefcon(1);
		_controlsScreen->addGadget(_fileReq);
		_fileReq->addGadgetEventHandler(this);
		_fileReq->hide();

		AmigaWindow* _controlWindow3 = new AmigaWindow(0, 48 + 35, 256, 110, "SDK Operations", Gadget::GADGET_DECORATION, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_micRecord = new Button(rect.x, rect.y, 100, 16, "Mic Stopped");
		_micRecord->setRefcon(9);
		_controlWindow3->addGadget(_micRecord);
		_micRecord->addGadgetEventHandler(this);
		_micRecording = false;
		_dumpNDSSections = new Button(rect.x + 100, rect.y, 130, 16, "Dump NDS Sections");
		_dumpNDSSections->setRefcon(10);
		_controlWindow3->addGadget(_dumpNDSSections);
		_dumpNDSSections->addGadgetEventHandler(this);
		
		_dumpARM7Memory = new Button(rect.x, rect.y, 100, 16, "Dump ARM7 Mem.");
		_dumpARM7Memory->setRefcon(11);
		_controlWindow3->addGadget(_dumpARM7Memory);
		_dumpARM7Memory->addGadgetEventHandler(this);
		
		_dumpDLDI = new Button(rect.x + 100, rect.y, 130, 16, "Dump DLDI");
		_dumpDLDI->setRefcon(12);
		_controlWindow3->addGadget(_dumpDLDI);
		_dumpDLDI->addGadgetEventHandler(this);

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_testLibNDSFifo = new Button(rect.x, rect.y + 16, 105, 16, "Test LibNDS FIFO");
		_testLibNDSFifo->setRefcon(13);
		_controlWindow3->addGadget(_testLibNDSFifo);
		_testLibNDSFifo->addGadgetEventHandler(this);

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_testcppFilesystem = new Button(rect.x + 105, rect.y + 16, 125, 16, "Test C++ Filesystem");
		_testcppFilesystem->setRefcon(14);
		_controlWindow3->addGadget(_testcppFilesystem);
		_testcppFilesystem->addGadgetEventHandler(this);

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_toggle2D3DMode = new Button(rect.x, rect.y + 16 + 16, 114, 16, "Toggle: 3D Mode");
		_toggle2D3DMode->setRefcon(15);
		_controlWindow3->addGadget(_toggle2D3DMode);
		_toggle2D3DMode->addGadgetEventHandler(this);
		_3DMode = 0;

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_reportPayloadMode = new Button(rect.x + 114, rect.y + 16 + 16, 132, 16, "Report Payload Mode");
		_reportPayloadMode->setRefcon(16);
		_controlWindow3->addGadget(_reportPayloadMode);
		_reportPayloadMode->addGadgetEventHandler(this);
		
		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_UnitTests = new Button(rect.x, rect.y + 16 + 16 + 16, 132, 16, "Run Unit Tests");
		_UnitTests->setRefcon(17);
		_controlWindow3->addGadget(_UnitTests);
		_UnitTests->addGadgetEventHandler(this);

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		__UnitTestList = new FileRequester(7, 10, 250, 150, "Unit Test List", "/", GADGET_DOUBLE_CLICKABLE);
		__UnitTestList->setRefcon(18);
		_controlsScreen->addGadget(__UnitTestList);
		__UnitTestList->addGadgetEventHandler(this);
		__UnitTestList->hide();
		

		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_RandomGen = new Button(rect.x + 116 + 16, rect.y + 16 + 16 + 16, 104, 16, "Random Key Gen.");
		_RandomGen->setRefcon(19);
		_controlWindow3->addGadget(_RandomGen);
		_RandomGen->addGadgetEventHandler(this);
		
		_controlsScreen->addGadget(_controlWindow3);
		_controlWindow3->getClientRect(rect);
		_RunToolchainGenericDSMB = new Button(rect.x, rect.y + 16 + 16 + 16 + 16, 120, 16, "Run TGDS-Multiboot");
		_RunToolchainGenericDSMB->setRefcon(20);
		_controlWindow3->addGadget(_RunToolchainGenericDSMB);
		_RunToolchainGenericDSMB->addGadgetEventHandler(this);

	}
	
	// Add Top Screen
	_topScreen = new AmigaScreen("Console Logger", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_topScreen);
	_topScreen->setPermeable(true);
	_topScreen->flipToTopScreen();
	// Add screen background
	_topScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	//Textbox 
	_topScreen->getClientRect(rect);
	_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, WoopsiString(TGDSPROJECTNAME), Gadget::GADGET_DRAGGABLE, 10);
	_topScreen->addGadget(_MultiLineTextBoxLogger);
	
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::ReportAvailableMem() {
	Rect rect;
	_controlsScreen->getClientRect(rect);
	_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y + 60, 200, 70, "DS Hardware status\n...", Gadget::GADGET_DRAGGABLE, 5);	// y + 60 px = move the rectangle vertically from parent obj
	_controlsScreen->addGadget(_MultiLineTextBoxLogger);
	
	_MultiLineTextBoxLogger->removeText(0);
	_MultiLineTextBoxLogger->moveCursorToPosition(0);
	_MultiLineTextBoxLogger->appendText("Memory Status: ");
	_MultiLineTextBoxLogger->appendText("\n");
	
	char arrBuild[256+1];
	sprintf(arrBuild, "Available heap memory: %d", TGDSARM9MallocFreeMemory());
	_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText) {
	thisLineTextBox->appendText(thisText);
	scanKeys();
	while((!(keysDown() & KEY_A)) && (!(keysDown() & KEY_TOUCH))){
		scanKeys();
	}
	scanKeys();
	while((keysDown() & KEY_A) && (keysDown() & KEY_TOUCH)){
		scanKeys();
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) {
	// Is the gadget the file requester? (OK pressed on _fileReq)
	if (e.getSource() != NULL) {
		if ((e.getSource()->getRefcon() == 18) && (((FileRequester*)e.getSource())->getSelectedOption() != NULL)) {
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			char debugBuf[256];
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			sprintf(debugBuf, "Handling test: %s", currentFileChosen);
			if(strncmp(currentFileChosen,"c_partial_mock", strlen(currentFileChosen)) == 0){
				//c_partial_mock
				opmock_test_suite_reset();
				opmock_register_test(test_fizzbuzz_with_15, "test_fizzbuzz_with_15");
				opmock_register_test(test_fizzbuzz_many_3, "test_fizzbuzz_many_3");
				opmock_register_test(test_fizzbuzz_many_5, "test_fizzbuzz_many_5");
				opmock_test_suite_run();
			}
			else if(strncmp(currentFileChosen,"c_regression", strlen(currentFileChosen)) == 0){
				//c_regression
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
			else if(strncmp(currentFileChosen,"cpp_tests", strlen(currentFileChosen)) == 0){
				//cpp_tests
				doCppTests();
			}
			else if(strncmp(currentFileChosen,"posix_filehandle_tests", strlen(currentFileChosen)) == 0){
				//posix_filehandle_tests
				if(testPosixFilehandle_fopen_fclose_method() == 0){
					printfWoopsi("testPosixFilehandle_fopen_fclose_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_fopen_fclose_method() ERROR");
				}
				
				if(testPosixFilehandle_sprintf_fputs_fscanf_method() == 0){
					printfWoopsi("testPosixFilehandle_sprintf_fputs_fscanf_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_sprintf_fputs_fscanf_method() ERROR");
				}
				
				if(testPosixFilehandle_fread_fwrite_method() == 0){
					printfWoopsi("testPosixFilehandle_fread_fwrite_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_fread_fwrite_method() ERROR");
				}
				
				if(testPosixFilehandle_fgetc_feof_method() == 0){
					printfWoopsi("testPosixFilehandle_fgetc_feof_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_fgetc_feof_method() ERROR");
				}
				
				if(testPosixFilehandle_fgets_method() == 0){
					printfWoopsi("testPosixFilehandle_fgets_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_fgets_method() ERROR");
				}
				
				if(testPosixFilehandle_fseek_rewind_method() == 0){
					printfWoopsi("testPosixFilehandle_fseek_rewind_method() OK");
				}
				else{
					printfWoopsi("testPosixFilehandle_fseek_rewind_method() ERROR");
				}
			}
			else if(strncmp(currentFileChosen,"ndsDisplayListUtils_tests", strlen(currentFileChosen)) == 0){
				/*
				//Unit Test: #0
				//ndsDisplayListUtils_tests: Nintendo DS reads embedded and compiled Cube.bin binary (https://bitbucket.org/Coto88/blender-nds-exporter/src) from filesystem
				//which generates a NDS GX Display List object, then it gets compiled again into a binary DisplayList.
				bool res = ndsDisplayListUtilsTestCaseARM9("0:/Cube_test.bin", "0:/Cube_compiled.bin");
				if(res != true){
					printfWoopsi("ndsDisplayListUtilsTestCaseARM9 ERROR");
					printfWoopsi("Cube_test.bin missing from SD root path");
				}
				else{
					printfWoopsi("ndsDisplayListUtilsTestCaseARM9 OK");
				}
				*/
			}
			else if(strncmp(currentFileChosen,"argv_chainload_test", strlen(currentFileChosen)) == 0){
				//argv_chainload_test: Chainloads through toolchaingenericds-multiboot into toolchaingenericds-argvtest while passing an argument to it.
				//Case use: Booting TGDS homebrew through TGDS-Multiboot in external loaders, and passing arguments to said TGDS homebrew
				
				//Default case use
				char * TGDS_CHAINLOADEXEC = NULL;
				char * TGDS_CHAINLOADTARGET = NULL;
				char * TGDS_CHAINLOADCALLER = NULL;
				if(__dsimode == true){
					TGDS_CHAINLOADCALLER = "0:/ToolchainGenericDS-UnitTest.srl";
					TGDS_CHAINLOADEXEC = "0:/ToolchainGenericDS-multiboot.srl";
					TGDS_CHAINLOADTARGET = "0:/ToolchainGenericDS-argvtest.srl";
				}
				else{
					TGDS_CHAINLOADCALLER = "0:/ToolchainGenericDS-UnitTest.nds";
					TGDS_CHAINLOADEXEC = "0:/ToolchainGenericDS-multiboot.nds";
					TGDS_CHAINLOADTARGET = "0:/ToolchainGenericDS-argvtest.nds";
				}
				char thisArgv[4][MAX_TGDSFILENAME_LENGTH];
				memset(thisArgv, 0, sizeof(thisArgv));
				strcpy(&thisArgv[0][0], TGDS_CHAINLOADCALLER);	//Arg0:	This Binary loaded
				strcpy(&thisArgv[1][0], TGDS_CHAINLOADEXEC);	//Arg1:	NDS Binary to chainload through TGDS-MB
				strcpy(&thisArgv[2][0], TGDS_CHAINLOADTARGET);	//Arg2: NDS Binary loaded from TGDS-MB	
				strcpy(&thisArgv[3][0], "0:/snes/smw.smc");					//Arg3: NDS Binary loaded from TGDS-MB's     ARG0
				addARGV(4, (char*)&thisArgv);
				strcpy(currentFileChosen, TGDS_CHAINLOADEXEC);
				if(TGDSMultibootRunNDSPayload(currentFileChosen) == false){ //should never reach here, nor even return true. Should fail it returns false
					printfWoopsi("Invalid NDS/TWL Binary");
					printfWoopsi("or you are in NTR mode trying to load a TWL binary.");
					printfWoopsi("or you are missing the TGDS-multiboot payload in root path.");
					printfWoopsi("Press (A) to continue.");
				}
			}
			else{
				_MultiLineTextBoxLogger->appendText("Unhandled test");
			}
			__UnitTestList->hide();
		}

		else if ((e.getSource()->getRefcon() == 1) && (((FileRequester*)e.getSource())->getSelectedOption() != NULL)) {
			//Handle task depending on _parentRefcon
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			switch(_parentRefcon){
				//_lzssCompress "Press OK" Event
				case 7:{
					string filenameChosen = string(currentFileChosen);
					string targetLZSSFilenameOut = OverrideFileExtension(filenameChosen, ".lzss");
					string targetLZSSFilenameIn = OverrideFileExtension(filenameChosen, ".bin");
					LZS_Encode(filenameChosen.c_str(), targetLZSSFilenameOut.c_str());
					_MultiLineTextBoxLogger->removeText(0);
					_MultiLineTextBoxLogger->moveCursorToPosition(0);
					char debugBuf[256];
					sprintf(debugBuf, "LZSS Compress OK: %s\n", targetLZSSFilenameOut.c_str());
					_MultiLineTextBoxLogger->appendText(debugBuf);
				}
				break;

				//_lzssDecompress "Press OK" Event
				case 8:{
					_MultiLineTextBoxLogger->removeText(0);
					_MultiLineTextBoxLogger->moveCursorToPosition(0);	
					//ext here
					char tmpName[256];
					char ext[256];
					char debugBuf[256];
					strcpy(tmpName, currentFileChosen);
					separateExtension(tmpName, ext);
					strlwr(ext);
					if(strncmp(ext,".lzss", 5) != 0){
						sprintf(debugBuf, "LZSS Decompress ERROR: %s\n", currentFileChosen);
						_MultiLineTextBoxLogger->appendText(debugBuf);
					}
					else{
						string filenameChosen = string(currentFileChosen);
						string targetLZSSFilenameOut = OverrideFileExtension(filenameChosen, ".lzss");
						string targetLZSSFilenameIn = OverrideFileExtension(filenameChosen, ".bin");
						bool res = LZS_Decode(filenameChosen.c_str(), targetLZSSFilenameIn.c_str());
						if(res == true){
							sprintf(debugBuf, "LZSS Decompress OK: %s\n", targetLZSSFilenameOut.c_str());
						}
						else{
							sprintf(debugBuf, "LZSS Decompress ERROR: %s\n", filenameChosen.c_str());
						}
						_MultiLineTextBoxLogger->appendText(debugBuf);
					}
				}
				break;

				//_dumpNDSSections "Press OK" Event
				case 10:{
					_MultiLineTextBoxLogger->removeText(0);
					_MultiLineTextBoxLogger->moveCursorToPosition(0);
					//Extract ARM7 and ARM9 bin
					if(dumpARM7ARM9Binary((char*)currentFileChosen) == true){
						_MultiLineTextBoxLogger->appendText("NDS/TWL Sections emitted correctly.");
					}
					else{
						_MultiLineTextBoxLogger->appendText("Invalid NDS/TWL Binary");
					}
				}
				break;
				default:{
					//Boot .NDS file! (homebrew only)
					char tmpName[256];
					char ext[256];
					strcpy(tmpName, currentFileChosen);
					separateExtension(tmpName, ext);
					strlwr(ext);
					{
						char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
						memset(thisArgv, 0, sizeof(thisArgv));
						strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
						strcpy(&thisArgv[1][0], currentFileChosen);	//Arg1:	NDS Binary reloaded
						strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
						addARGV(3, (char*)&thisArgv);
						if(TGDSMultibootRunNDSPayload(currentFileChosen) == false){  //Should fail it returns false. (Audio track)
							pendPlay = 1;
						}
					}
				}break;
			}
			_parentRefcon = -1;
			_fileReq->hide();
		}
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
std::string getDldiDefaultPath(){
	std::string dldiOut = string((char*)getfatfsPath( (sint8*)string(dldi_tryingInterface() + string(".dldi")).c_str() ));
	return dldiOut;
}


//Should ARM7 reply a message, then, it'll be received to test the Libnds FIFO implementation.
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void returnMsgHandler(int bytes, void* user_data)
{
	returnMsg msg;
	fifoGetDatamsg(FIFO_RETURN, bytes, (u8*) &msg);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->removeText(0);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->moveCursorToPosition(0);
	char debugBuf[256];
	sprintf(debugBuf, "ARM7: %s [Msg Size: %d]", (char*)&msg.data[0], bytes);
	WoopsiTemplateProc->_MultiLineTextBoxLogger->appendText(debugBuf);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void InstallSoundSys()
{
	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_RETURN, returnMsgHandler, 0);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleClickEvent(const GadgetEventArgs& e) {
	int currentRefCon = e.getSource()->getRefcon();
	switch (currentRefCon) {
		//_Idle Event
		case 2:{
			switch_dswnifi_mode(dswifi_idlemode);		//IDLE: (single player)
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("[Idle Nifi]:Disconnected\n");	
		}	
		break;
		
		//_udpNifi Event
		case 3:{
			switch_dswnifi_mode(dswifi_idlemode);		//IDLE: (single player)
			switch_dswnifi_mode(dswifi_udpnifimode);	//UDP NIFI: Check readme
		}	
		break;
		
		//_localNifi Event
		case 4:{
			switch_dswnifi_mode(dswifi_idlemode);		//IDLE: (single player)
			switch_dswnifi_mode(dswifi_localnifimode);	//LOCAL NIFI:
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("[Local Nifi]:Connected\n");	
		}	
		break;
		
		//_sendNifiMsg Event
		case 5:{
			//Sender DS Time
			sprintf((char*)somebuf,"DSTime - %d - %d - %d ",getTime()->tm_hour,getTime()->tm_min,getTime()->tm_sec);
			FrameSenderUser = HandleSendUserspace((uint8*)somebuf, frameDSsize);
		}	
		break;
		
		//_udpNifiMsgBox "OK" click Event
		case 6:{
			//Get text string from textbox
			switch_dswnifi_mode(dswifi_localnifimode);	//LOCAL NIFI:
			Button * thisButton = _udpNifiMsgBox->getButtonHandle();
			MultiLineTextBox* thisTextBox = _udpNifiMsgBox->getTextBoxHandle();
			char ipAddrFromTextbox[256];
			memset(ipAddrFromTextbox, 0, sizeof(ipAddrFromTextbox));
			char outStr[256];
			memset(outStr, 0, sizeof(outStr));
			thisTextBox->getText()->copyToCharArray(ipAddrFromTextbox);
			//Validate IP here:
			bool validIP = isValidIpAddress(ipAddrFromTextbox);
			if(validIP != true){
				sprintf(outStr, "%s[%s]\nEnter a valid Remote Companion IP.", "[ERROR] Invalid IP: ",ipAddrFromTextbox);		
			}
			else{
				strcpy(outStr, "");
			}
			_udpNifiMsgBox->hide();
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText(outStr);		
			
			//Connect if valid
			if(validIP == true){
				//update new IP
				strcpy((char*)&server_ip[0], ipAddrFromTextbox);
				switch_dswnifi_mode(dswifi_udpnifimode);	//UDP NIFI: Check readme
			}
		}
		break;
		
		//_lzssCompress Event & _lzssDecompress Event & _dumpNDSSections Event
		case 7:
		case 8:
		case 10:
		{
			_parentRefcon = currentRefCon;
			_fileReq->show();
		}
		break;
		
		//_micRecord Event
		case 9:{
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_micRecording = !_micRecording;
			char * recFile = "0:/RECNDS.WAV";
			if(_micRecording == false){
				endRecording();
				_micRecord->setText("Mic Stopped");
				char debugBuf[256];
				sprintf(debugBuf, "Mic Recording ended: %s\n", recFile);
				_MultiLineTextBoxLogger->appendText(debugBuf);
			}
			else{
				startRecording(recFile);
				_micRecord->setText("Mic Recording");
				_MultiLineTextBoxLogger->appendText("Mic Recording started.\n");
			}
		}
		break;
		
		//_dumpARM7Memory Event
		case 11:{
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			u32 ARM7SrcAddress = 0x03800000;
			int ARM7ReadSize = 64*1024;
			u8* ARM7Mem = (u8*)TGDSARM9Malloc(ARM7ReadSize);
			ReadMemoryExt((u32*)ARM7SrcAddress, (u32 *)ARM7Mem, ARM7ReadSize); //todo: add new method
			char * fnameOut = "0:/arm7.bin";
			FILE * fout = fopen(fnameOut, "w+");
			if(fout != NULL){
				fwrite(ARM7Mem, 1, ARM7ReadSize, fout);
				fclose(fout);
			}
			_MultiLineTextBoxLogger->appendText("ARM7 Dump: Saved 0:/arm7.bin.\n");
			TGDSARM9Free(ARM7Mem);
		}	
		break;

		//_dumpDLDI Event
		case 12:{
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);

			DLDI_INTERFACE* dldiInterface = (struct DLDI_INTERFACE*)&_io_dldi_stub;
			uint8 * dldiStart = (uint8 *)dldiInterface;
			int dldiSize = (int)pow((double)2, (double)dldiInterface->driverSize);	// this is easily 2 << (dldiInterface->driverSize - 1), but I use pow() to test the math library in the ARM9 core
			FILE * fh = fopen(getDldiDefaultPath().c_str(),"w+");
			if(fh){
				fwrite(dldiStart, 1, dldiSize, fh);
				fclose(fh);
				char debugBuf[256];
				sprintf(debugBuf, "DLDI: %s exported.\nbytes:%d", getDldiDefaultPath().c_str(), dldiSize);
				_MultiLineTextBoxLogger->appendText(debugBuf);
			}
		}	
		break;

		//_testLibNDSFifo Event
		case 13:{
			returnMsg msg;
			sprintf((char*)&msg.data[0], "%s", "Test message using libnds FIFO API!");
			if(fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg) != true){
				_MultiLineTextBoxLogger->removeText(0);
				_MultiLineTextBoxLogger->moveCursorToPosition(0);
				_MultiLineTextBoxLogger->appendText("libnds FIFO API failure.");
			}
		}	
		break;

		//_testcppFilesystem Event
		case 14:{
			char debugBuf[256];
			bool testPass = false;
			//First write.........................................................................
			string fOut = string(getfatfsPath((char *)"filelist.txt"));
			std::ofstream outfile;
			outfile.open(fOut.c_str());
			char curPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(curPath, "/");
			
			//Create TGDS Dir API context
			struct FileClassList * fileClassListCtx = initFileList();
			cleanFileList(fileClassListCtx);
			
			//Use TGDS Dir API context
			int startFromIndex = 0;
			struct FileClass * fileClassInst = NULL;
			fileClassInst = FAT_FindFirstFile(curPath, fileClassListCtx, startFromIndex);			
			while(fileClassInst != NULL){
				std::string fnameOut = std::string("");
				//directory?
				if(fileClassInst->type == FT_DIR){
					fnameOut = string(fileClassInst->fd_namefullPath) + string("/[dir]");
				}
				//file?
				else if(fileClassInst->type == FT_FILE){
					fnameOut = string(fileClassInst->fd_namefullPath);
				}
				outfile << fnameOut << endl;
				
				//more file/dir objects?
				fileClassInst = FAT_FindNextFile(curPath, fileClassListCtx);
			}
			
			//Free TGDS Dir API context
			freeFileList(fileClassListCtx);
			outfile.close();
			
			//sprintf(debugBuf, "filelist %s saved.", fOut.c_str());
			//_MultiLineTextBoxLogger->appendText(debugBuf);

			//Then read...........................................................................
			char InFile[80];  // input file name
			char ch;
			
			ifstream InStream;
			std::string someString;
			
			ofstream OutStream;
			sprintf(InFile,"%s%s",getfatfsPath((char*)""),"filelist.txt");
			
			// Open file for input
			// in.open(fin); also works
			InStream.open(InFile, ios::in);

			// ensure file is opened successfully
			if(!InStream)
			{
				//sprintf(debugBuf, "Error open file %s",InFile);
				//_MultiLineTextBoxLogger->appendText(debugBuf);
			}
			else{
				// Read in each character until eof character is read.
				// Output it to screen.
				int somePosition = 0;
				while (!InStream.eof()) {
					//Read each character.
					InStream.get(ch);
					someString.insert(somePosition, ToStr(ch));
					somePosition++;
				}
				InStream.close();
				testPass = true;
			}

			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			if(testPass == true){
				sprintf(debugBuf, "C++ Filesystem Test Passed!\nFile emitted:%s", InFile);
				_MultiLineTextBoxLogger->appendText(debugBuf);
			}
			else{
				_MultiLineTextBoxLogger->appendText("C++ Filesystem Test Failure.");
			}
		}	
		break;

		//_toggle2D3DMode Event
		case 15:{
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_3DMode++;
			if(_3DMode > 2){
				_3DMode = 0;
			}
			if(_3DMode == 1){
				_toggle2D3DMode->setText("Toggle: 3D Mode: OpenGL DisplayList");
				_MultiLineTextBoxLogger->appendText("Set to 3D Mode: Simple Triangle .\n");
				//char debugBuf[256];
				//sprintf(debugBuf, "Mic Recording ended: %s\n", recFile);
				//_MultiLineTextBoxLogger->appendText(debugBuf);

				//Simple Triangle GL init
				glInit(); //NDSDLUtils: Initializes a new videoGL context
				rotateX = 0.0;
				rotateY = 0.0;
				{
					//set mode 0, enable BG0 and set it to 3D
					SETDISPCNT_MAIN(MODE_0_3D);
					
					//this should work the same as the normal gl call
					glViewport(0,0,255,191,USERSPACE_TGDS_OGL_DL_POINTER);
					
					glClearColor(0,0,0);
					glClearDepth(0x7FFF);
				}
			}
			else if(_3DMode == 2){ //OpenGL Display List mode
				_toggle2D3DMode->setText("Toggle: 2D Mode");
				_MultiLineTextBoxLogger->appendText("Set to 3D Mode: OpenGL DisplayList .\n");

				// OpenGL 1.1 Dynamic Display List 
				//bool isTGDSCustomConsole = true;	//set default console or custom console: 3D mode
				//GUI_init(isTGDSCustomConsole);
				
				//Update specific Engine A Bits
				VRAMBLOCK_SETBANK_A(VRAM_A_ENGINE_A_3DENGINE_TEXTURE);
				VRAMBLOCK_SETBANK_B(VRAM_B_ENGINE_A_3DENGINE_TEXTURE);

				InitGL();
				ReSizeGLScene(255, 191);			
			}
			else{
				bool isTGDSCustomConsole = false;	//set default console or custom console: default console
				GUI_init(isTGDSCustomConsole);

				woopsiFreeFrameBuffers(); //Free them first
				initWoopsiGfxMode();

				_toggle2D3DMode->setText("Toggle: 3D Mode");
				_MultiLineTextBoxLogger->appendText("Set to 2D Mode.\n");
			}
		}	
		break;

		//_reportPayloadMode Event
		case 16:{
			char debugBuf[256];
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			char ARM7OutLog[256];
			char ARM9OutLog[256];
			reportTGDSPayloadMode((u32)&bufModeARM7[0], (char*)&ARM7OutLog[0], (char*)&ARM9OutLog[0]);
			sprintf(debugBuf, "%s\n", ARM7OutLog);
			_MultiLineTextBoxLogger->appendText(debugBuf);
			sprintf(debugBuf, "%s\n", ARM9OutLog);
			_MultiLineTextBoxLogger->appendText(ARM9OutLog);
		}	
		break;

		//_UnitTests Event
		case 17:{
			char debugBuf[256];
			__UnitTestList->removeAllOptions();
			__UnitTestList->addOption("c_partial_mock", 0);
			__UnitTestList->addOption("c_regression", 1);
			__UnitTestList->addOption("cpp_tests", 2);
			__UnitTestList->addOption("posix_filehandle_tests", 3);
			__UnitTestList->addOption("ndsDisplayListUtils_tests", 4);
			__UnitTestList->addOption("argv_chainload_test", 5);
			__UnitTestList->redraw();
			__UnitTestList->show();
		}	
		break;

		//_RandomGen Event
		case 19:{
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			string fOut = string(getfatfsPath((char *)"keys.txt"));
			std::remove(fOut.c_str());
			std::ofstream outfile;
			outfile.open(fOut.c_str());

			int keyCount = 20;
			vector<string> keys = generateAndShuffleKeys(keyCount);
			char deb[256];
			for (int i = 0; i < keys.size(); i++){
				outfile << keys.at(i) << endl;
			}
			outfile.close();

			sprintf(deb, "%s emitted OK. (Count:%d)\n", fOut.c_str(),  keys.size());
			_MultiLineTextBoxLogger->appendText(deb);
		}
		break;

		//_RunToolchainGenericDSMB Event
		case 20:{
			
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("Running ToolchainGenericDS-Multiboot");
			//Default case use
			char * TGDS_CHAINLOADEXEC = NULL;
			char * TGDS_CHAINLOADTARGET = NULL;
			char * TGDS_CHAINLOADCALLER = NULL;
			if(__dsimode == true){
				TGDS_CHAINLOADCALLER = "0:/ToolchainGenericDS-UnitTest.srl";
				TGDS_CHAINLOADEXEC = "0:/ToolchainGenericDS-multiboot.srl";
				TGDS_CHAINLOADTARGET = "0:/ToolchainGenericDS-multiboot.srl";
			}
			else{
				TGDS_CHAINLOADCALLER = "0:/ToolchainGenericDS-UnitTest.nds";
				TGDS_CHAINLOADEXEC = "0:/ToolchainGenericDS-multiboot.nds";
				TGDS_CHAINLOADTARGET = "0:/ToolchainGenericDS-multiboot.nds";
			}
			char thisArgv[4][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], TGDS_CHAINLOADCALLER);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], TGDS_CHAINLOADEXEC);	//Arg1:	NDS Binary to chainload through TGDS-MB
			strcpy(&thisArgv[2][0], TGDS_CHAINLOADTARGET);	//Arg2: NDS Binary loaded from TGDS-MB	
			addARGV(3, (char*)&thisArgv);
			strcpy(currentFileChosen, TGDS_CHAINLOADEXEC);
			if(TGDSMultibootRunNDSPayload(currentFileChosen) == false){ //should never reach here, nor even return true. Should fail it returns false
				printfWoopsi("Invalid NDS/TWL Binary");
				printfWoopsi("or you are in NTR mode trying to load a TWL binary.");
				printfWoopsi("or you are missing the TGDS-multiboot payload in root path.");
				printfWoopsi("Press (A) to continue.");
			}
		}
		break;
	}
}

__attribute__((section(".dtcm")))
u32 pendPlay = 0;

char currentFileChosen[256+1];

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void Woopsi::ApplicationMainLoop() {
	//Earlier.. main from Woopsi SDK.
	
	//Handle TGDS stuff...
	if(WoopsiTemplateProc->_3DMode == 1){
		int pen_delta[2];
		bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
		if( isTSCActive == true ){
			WoopsiTemplateProc->rotateY -= pen_delta[0];
			WoopsiTemplateProc->rotateX -= pen_delta[1];
		}

		glReset(USERSPACE_TGDS_OGL_DL_POINTER);
	
		//any floating point gl call is being converted to fixed prior to being implemented
		gluPerspective(35, 256.0 / 192.0, 0.1, 40,USERSPACE_TGDS_OGL_DL_POINTER);

		gluLookAt(	0.0, 0.0, 1.0,		//camera possition 
					0.0, 0.0, 0.0,		//look at
					0.0, 1.0, 0.0,		//up
					USERSPACE_TGDS_OGL_DL_POINTER);		

		glPushMatrix(USERSPACE_TGDS_OGL_DL_POINTER);

		//move it away from the camera
		glTranslate3f32(0, 0, floattof32(-1),USERSPACE_TGDS_OGL_DL_POINTER);
				
		glRotateX(WoopsiTemplateProc->rotateX,USERSPACE_TGDS_OGL_DL_POINTER);
		glRotateY(WoopsiTemplateProc->rotateY,USERSPACE_TGDS_OGL_DL_POINTER);
		glMatrixMode(GL_MODELVIEW,USERSPACE_TGDS_OGL_DL_POINTER);
		
		//not a real gl function and will likely change
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE,USERSPACE_TGDS_OGL_DL_POINTER);
		
		//draw the obj
		glBegin(GL_TRIANGLE,USERSPACE_TGDS_OGL_DL_POINTER);
			
			glColor3b(31,0,0,USERSPACE_TGDS_OGL_DL_POINTER);
			glVertex3v16(inttov16(-1),inttov16(-1),0,USERSPACE_TGDS_OGL_DL_POINTER);

			glColor3b(0,31,0,USERSPACE_TGDS_OGL_DL_POINTER);
			glVertex3v16(inttov16(1), inttov16(-1), 0,USERSPACE_TGDS_OGL_DL_POINTER);

			glColor3b(0,0,31,USERSPACE_TGDS_OGL_DL_POINTER);
			glVertex3v16(inttov16(0), inttov16(1), 0,USERSPACE_TGDS_OGL_DL_POINTER);
			
		glEnd(USERSPACE_TGDS_OGL_DL_POINTER);
		glPopMatrix(1,USERSPACE_TGDS_OGL_DL_POINTER);
		glFlush(USERSPACE_TGDS_OGL_DL_POINTER);
		
	}
	else if(WoopsiTemplateProc->_3DMode == 2){
		DrawGLScene();
	}
	
	handleARM9SVC();	/* Do not remove, handles TGDS services */
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
static std::string to_hex_string( const unsigned int i ) {
    std::stringstream s;
    s /*<< "0x"*/ << std::hex << i;
    return s.str();
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
vector<string> generateAndShuffleKeys(int keyCount){
	//std::random_device dev;
    //std::mt19937 rng(dev());
    //std::uniform_int_distribution<std::mt19937::result_type> dist6(1,6); // distribution in range [1, 6]

	//Build random values
	int listCount = keyCount;
	int uniqueArr[listCount*4];
	int i=0;
	int indx=0;
	for(i=0; i < listCount*4; i++){ //each generated value is 4 x u32 random values
		uniqueArr[i] = -1;
	}
	for(;;){
		int randVal = rand() % (UINT32_MAX);
		if (contains(uniqueArr, listCount*4, randVal) == false){
			uniqueArr[indx] = randVal;
			indx++;
		}
		if(indx == (listCount*4)){
			break;
		}
	}
	
	vector<string> vect;
	i = 0;
	for(i = 0; i < keyCount; i++){
		u32 randomval0 = uniqueArr[(i*4)+0]; 
		u32 randomval1 = uniqueArr[(i*4)+1]; 
		u32 randomval2 = uniqueArr[(i*4)+2]; 
		u32 randomval3 = uniqueArr[(i*4)+3]; 		
		vect.push_back(to_hex_string(randomval0) + to_hex_string(randomval1) + to_hex_string(randomval2) + to_hex_string(randomval3) + string("_0x" + to_hex_string(i)));
	}
	return vect;
}

//GL Display Lists Unit Test: Cube Demo
// Build Cube Display Lists
GLvoid BuildLists(){
	box=glGenLists(2,USERSPACE_TGDS_OGL_DL_POINTER);	// Generate 2 Different Lists
	glNewList(box,GL_COMPILE,USERSPACE_TGDS_OGL_DL_POINTER);							// Start With The Box List
		glBegin(GL_QUADS,USERSPACE_TGDS_OGL_DL_POINTER);
			// Bottom Face
			glNormal3f(0.5f,-1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			// Front Face
			glNormal3f( 1.0f, 1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			// Back Face
			glNormal3f( 1.0f, 1.0f,-1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			// Right face
			glNormal3f( 1.0f, 0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			// Left Face
			glNormal3f(-1.0f, 0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f, -1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
		glEnd(USERSPACE_TGDS_OGL_DL_POINTER);
	glEndList(USERSPACE_TGDS_OGL_DL_POINTER);
	top=box+1;											// Storage For "Top" Is "Box" Plus One
	glNewList(top,GL_COMPILE,USERSPACE_TGDS_OGL_DL_POINTER); // Now The "Top" Display List
		glBegin(GL_QUADS,USERSPACE_TGDS_OGL_DL_POINTER);
			// Top Face
			glNormal3f( 1.0f, 1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(0.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f(-1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 0.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f,  1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glTexCoord2f(1.0f, 1.0f,USERSPACE_TGDS_OGL_DL_POINTER); glVertex3f( 1.0f,  1.0f, -1.0f,USERSPACE_TGDS_OGL_DL_POINTER);
		glEnd(USERSPACE_TGDS_OGL_DL_POINTER);
	glEndList(USERSPACE_TGDS_OGL_DL_POINTER);
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height,USERSPACE_TGDS_OGL_DL_POINTER);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION,USERSPACE_TGDS_OGL_DL_POINTER);						// Select The Projection Matrix
	glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f,USERSPACE_TGDS_OGL_DL_POINTER);

	glMatrixMode(GL_MODELVIEW,USERSPACE_TGDS_OGL_DL_POINTER); // Select The Modelview Matrix
	glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);									// Reset The Modelview Matrix
}

int textureArrayNDS[1]; //0 : Cube tex 
int InitGL()										// All Setup For OpenGL Goes Here
{
	glInit(); //NDSDLUtils: Initializes a new videoGL context	
	glClearColor(0,0,0);		// Black Background
	glClearDepth(0x7FFF);		// Depth Buffer Setup
	glEnable(GL_ANTIALIAS);
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping 
	glEnable(GL_BLEND);

	//#1: Load a texture and map each one to a texture slot
	u32 arrayOfTextures[1];
	arrayOfTextures[0] = (u32)&Texture_Cube;  
	int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (int*)&textureArrayNDS, 1); //Implements both glBindTexture and glTexImage2D 
	int i = 0;
	for(i = 0; i < texturesInSlot; i++){
		//printf("Texture loaded: %d:textID[%d] Size: %d", i, textureArrayNDS[i], getTextureBaseFromTextureSlot(activeTexture));
	}
	
	//#2: Emit Display Lists
	BuildLists();										// Jump To The Code That Creates Our Display Lists
	return true;				
}

int DrawGLScene(){									
	scanKeys();
	int pen_delta[2];
	bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
	if( isTSCActive == false ){
		printfCoords(0, 16, " No TSC Activity ----");
		rotateX = 0.0;
		rotateY = 0.0;
	}
	else{
		printfCoords(0, 16, "TSC Activity ----");
		rotateX = pen_delta[0];
		rotateY = pen_delta[1];
		if(yrot > 0){
			yrot-=rotateY;
		}
		else{
			yrot+=rotateY;
		}
		
		if(xrot > 0){
			xrot-=rotateX;
		}
		else{
			xrot+=rotateX;
		}
	}
	if (keysDown() & KEY_LEFT)
	{
		camMov-=2.8f;
	}
	if (keysDown() & KEY_RIGHT)
	{
		camMov+=2.8f;
	}

				//Clear The Screen And The Depth Buffer
	glReset(USERSPACE_TGDS_OGL_DL_POINTER); //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	todo: implement https://stackoverflow.com/questions/28137027/why-do-i-need-glcleargl-depth-buffer-bit through cuboid tests and rendering into OpenGL
	//Instead, we forcefully reset so always points to a first default projection matrix, then matrix node relative to a first default modelview 
	
	//Set initial view to look at
	gluPerspective(18, 256.0 / 192.0, 0.1, 40,USERSPACE_TGDS_OGL_DL_POINTER);

	//Camera perspective from there
	gluLookAt(	0.0, 0.0, 4.0,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0,		//up
	USERSPACE_TGDS_OGL_DL_POINTER);
	glMaterialShinnyness(USERSPACE_TGDS_OGL_DL_POINTER);
	
	GLfloat light_ambient[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, xrot, 1.0f };
    GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f };

    //GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
    //GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
    //GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
    //GLfloat high_shininess[] = { 120.0f };
	//Draw each cube through a set of Open GL Display List calls 
	for (yloop=1;yloop<6;yloop++){
		for (xloop=0;xloop<yloop;xloop++){
			glBindTexture( 0, textureArrayNDS[0],USERSPACE_TGDS_OGL_DL_POINTER);
			// Reset The View
			glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);
			glTranslatef(1.4f+(float(xloop)*2.8f)-(float(yloop)*1.4f),((6.0f-float(yloop))*2.4f)-7.3f,-20.0f + camMov,USERSPACE_TGDS_OGL_DL_POINTER);
			
			glRotatef(45.0f-(2.0f*yloop)+xrot,1.0f,0.0f,0.0f,USERSPACE_TGDS_OGL_DL_POINTER);
			glRotatef(45.0f+yrot,0.0f,1.0f,0.0f,USERSPACE_TGDS_OGL_DL_POINTER);

			//not a real gl function and will likely change
			#define GX_LIGHT0 (1 << 0)
			glPolyFmt(GX_LIGHT0 | POLY_ALPHA(31) | POLY_CULL_NONE,USERSPACE_TGDS_OGL_DL_POINTER);
			
			glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient,USERSPACE_TGDS_OGL_DL_POINTER);
			glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse,USERSPACE_TGDS_OGL_DL_POINTER);
			glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular,USERSPACE_TGDS_OGL_DL_POINTER);
			glLightfv(GL_LIGHT0, GL_POSITION, light_position,USERSPACE_TGDS_OGL_DL_POINTER);
			glMaterialf(GL_AMBIENT_AND_DIFFUSE, RGB15(31,31,31),USERSPACE_TGDS_OGL_DL_POINTER);	
			//Run the precompiled standard Open GL 1.1 Display List here
			glColor3fv(boxcol[yloop-1],USERSPACE_TGDS_OGL_DL_POINTER);
			glCallList(box,USERSPACE_TGDS_OGL_DL_POINTER);
			glColor3fv(topcol[yloop-1],USERSPACE_TGDS_OGL_DL_POINTER);
			glCallList(top,USERSPACE_TGDS_OGL_DL_POINTER);
		}
	}
	glFlush(USERSPACE_TGDS_OGL_DL_POINTER);
	
	return true;										// Keep Going
}