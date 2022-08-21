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

//This file abstracts specific TGDS console code which allows for easy DS console setup.

#include "gui_console_connector.h"
#include "consoleTGDS.h"
#include "videoTGDS.h"

////////[For custom Console implementation]:////////
//You need to override :
	//vramSetup * getProjectSpecificVRAMSetup()
	//Which provides a proper custom 2D VRAM setup

//Then override :
	//bool InitProjectSpecificConsole()
	//Which provides the console init code, example not available here, checkout projects that support Custom console implementation.

//After that you can call :
	//bool project_specific_console = true;
	//GUI_init(project_specific_console);


	////////[Custom Console implementation]////////



//Definition that overrides the weaksymbol expected from toolchain to init console video subsystem
vramSetup * getProjectSpecificVRAMSetup(){
	return TGDSFOOBILLIARD_3DVRAM_SETUP();
}


//2) Uses subEngine: VRAM Layout -> Console Setup
bool InitProjectSpecificConsole(){
	//TGDS Console defaults
	DefaultSessionConsole = (ConsoleInstance *)(&DefaultConsole);
	
	//Set subEngine as TGDS Console
	GUI.consoleAtTopScreen = false;
	GUI.consoleBacklightOn = true;	//Backlight On for console
	SetEngineConsole(subEngine,DefaultSessionConsole);
	
	//Set subEngine properties
	DefaultSessionConsole->ConsoleEngineStatus.ENGINE_DISPCNT	=	(uint32)(MODE_5_2D | DISPLAY_BG3_ACTIVE );
	
	// BG3: FrameBuffer : 64(TILE:4) - 128 Kb
	DefaultSessionConsole->ConsoleEngineStatus.EngineBGS[3].BGNUM = 3;
	DefaultSessionConsole->ConsoleEngineStatus.EngineBGS[3].REGBGCNT = BG_BMP_BASE(4) | BG_BMP8_256x256 | BG_PRIORITY_1;
	
	GUI.DSFrameBuffer = (uint16 *)BG_BMP_RAM_SUB(4);
	
	GUI.Palette = &BG_PALETTE_SUB[0];
	GUI.Palette[0] = 	RGB8(0,0,0);			//Back-ground tile color / Black
	GUI.Palette[1] =	RGB8(255, 255, 255); 	//White
	GUI.Palette[2] =  	RGB8(150, 75, 0); 		//Brown
	GUI.Palette[3] =  	RGB8(255, 127, 0); 		//Orange
	GUI.Palette[4] = 	RGB8(255, 0, 255); 		//Magenta
	GUI.Palette[5] = 	RGB8(0, 255, 255); 		//Cyan
	GUI.Palette[6] = 	RGB8(255, 255, 0); 		//Yellow
	GUI.Palette[7] = 	RGB8(0, 0, 255); 		//Blue
	GUI.Palette[8] = 	RGB8(0, 255, 0); 		//Green
	GUI.Palette[9] = 	RGB8(255, 0, 0); 		//Red
	GUI.Palette[0xa] = 	RGB8(128, 128, 128); 	//Grey
	GUI.Palette[0xb] = 	RGB8(240, 240, 240);	//Light-Grey
	
	//Fill the Pallette
	int i = 0;
	for(i=0;i < (256 - 0xb); i++){
		GUI.Palette[i + 0xc] = GUI.Palette[TGDSPrintfColor_White];
	}
	
	InitializeConsole(DefaultSessionConsole);
	
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;
	REG_BG3PA_SUB = 1 << 8;
	REG_BG3PB_SUB = 0;
	REG_BG3PC_SUB = 0;
	REG_BG3PD_SUB = 1 << 8;
	
	
	bool mainEngine = true;
	setOrientation(ORIENTATION_0, mainEngine);
	mainEngine = false;
	setOrientation(ORIENTATION_0, mainEngine);
	
	return true;
}


//Definition that overrides the weaksymbol expected from toolchain to init console video subsystem
//1) VRAM Layout
vramSetup * TGDSFOOBILLIARD_3DVRAM_SETUP(){
	
	vramSetup * vramSetupInst = (vramSetup *)&vramSetupDefaultConsole;
	
	//Main Engine Setup: 
	
	//VRAM A and B reserved for 2D Textures to-be used with the 3D Engine
	vramSetupInst->vramBankSetupInst[VRAM_A_INDEX].vrambankCR = VRAM_A_ENGINE_A_3DENGINE_TEXTURE;
	vramSetupInst->vramBankSetupInst[VRAM_A_INDEX].enabled = true;																		
	vramSetupInst->vramBankSetupInst[VRAM_B_INDEX].vrambankCR = VRAM_B_ENGINE_A_3DENGINE_TEXTURE;
	vramSetupInst->vramBankSetupInst[VRAM_B_INDEX].enabled = true;
	
	//VRAM C: WoopsiTGDS Touchscreen UI
	vramSetupInst->vramBankSetupInst[VRAM_C_INDEX].vrambankCR = VRAM_C_0x06200000_ENGINE_B_BG;
	vramSetupInst->vramBankSetupInst[VRAM_C_INDEX].enabled = true;
	
	//VRAM D: ARM7 (128 Ko!)
	vramSetupInst->vramBankSetupInst[VRAM_D_INDEX].vrambankCR = VRAM_D_0x06000000_ARM7;
	vramSetupInst->vramBankSetupInst[VRAM_D_INDEX].enabled = true;
	
	//VRAM E,F,G,H,I: Unused and reserved
	vramSetupInst->vramBankSetupInst[VRAM_E_INDEX].vrambankCR = VRAM_E_LCDC_MODE;
	vramSetupInst->vramBankSetupInst[VRAM_E_INDEX].enabled = true;
	
	vramSetupInst->vramBankSetupInst[VRAM_F_INDEX].vrambankCR = VRAM_F_LCDC_MODE;
	vramSetupInst->vramBankSetupInst[VRAM_F_INDEX].enabled = true;
	
	vramSetupInst->vramBankSetupInst[VRAM_G_INDEX].vrambankCR = VRAM_G_LCDC_MODE;
	vramSetupInst->vramBankSetupInst[VRAM_G_INDEX].enabled = true;
	
	vramSetupInst->vramBankSetupInst[VRAM_H_INDEX].vrambankCR = VRAM_H_LCDC_MODE;
	vramSetupInst->vramBankSetupInst[VRAM_H_INDEX].enabled = true;
	
	vramSetupInst->vramBankSetupInst[VRAM_I_INDEX].vrambankCR = VRAM_I_LCDC_MODE;
	vramSetupInst->vramBankSetupInst[VRAM_I_INDEX].enabled = true;
	
	
	return vramSetupInst;
}

