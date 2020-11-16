//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include <circle/usb/usbkeyboard.h>
#include <circle/string.h>
#include <circle/util.h>
#include <assert.h>
//#include <circle/input/keyboardbuffer.h>
//#include <linux/sprintf.h>
#include <circle/usb/usbmassdevice.h>
#include <circle/macros.h>

#include <fatfs/ff.h>

#define DRIVE           "SD:"
#define FILENAME        "/circle.txt"


static const char FromKernel[] = "kernel";

void main_basic();


//CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
	:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_ShutdownMode (ShutdownNone)
{
	g_kernel = this;
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	g_kernel = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK) bOK = m_Screen.Initialize ();

	if (bOK) bOK = m_Serial.Initialize (115200);

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK) bOK = m_Interrupt.Initialize ();

	if (bOK) bOK = m_Timer.Initialize ();

	if (bOK) bOK = m_USBHCI.Initialize ();

	init_sdcard();

	return bOK;
}




void CKernel::cmd_type(unsigned char* filename)
{


	FIL File;
	FRESULT Result;
	CString full = CString(DRIVE);
	full.Append("/");
	full.Append((char*)filename);
	// Reopen file, read it and display its contents
	Result = f_open (&File, (const char*) full, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogError, "Cannot open file: %s", (const char*) full);
		return;
	}


	char Buffer[100];
	unsigned nBytesRead;
	while ((Result = f_read (&File, Buffer, sizeof Buffer, &nBytesRead)) == FR_OK)
	{
		if (nBytesRead > 0)
		{
			m_Screen.Write (Buffer, nBytesRead);
		}

		if (nBytesRead < sizeof Buffer)         // EOF?
		{
			break;
		}
	}

	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogError, "Read error");
	}

	if (f_close (&File) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}


}
void CKernel::test_sdcard()
{
	// Show contents of root directory
	DIR Directory;
	FILINFO FileInfo;
	FRESULT Result = f_findfirst (&Directory, &FileInfo, DRIVE "/", "*");
	for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
	{
		if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
		{
			CString FileName;
			FileName.Format ("%-19s", FileInfo.fname);

			m_Screen.Write ((const char *) FileName, FileName.GetLength ());

			if (i % 4 == 3)
			{
				m_Screen.Write ("\n", 1);
			}
		}

		Result = f_findnext (&Directory, &FileInfo);
	}
	m_Screen.Write ("\n", 1);


	// Create file and write to it
	FIL File;
	Result = f_open (&File, DRIVE FILENAME, FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot create file: %s", FILENAME);
	}

	for (unsigned nLine = 1; nLine <= 5; nLine++)
	{
		CString Msg;
		Msg.Format ("Hello File! (Line %u)\n", nLine);

		unsigned nBytesWritten;
		if (   f_write (&File, (const char *) Msg, Msg.GetLength (), &nBytesWritten) != FR_OK
				|| nBytesWritten != Msg.GetLength ())
		{
			m_Logger.Write (FromKernel, LogError, "Write error");
			break;
		}
	}

	if (f_close (&File) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}

	// Reopen file, read it and display its contents
	Result = f_open (&File, DRIVE FILENAME, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot open file: %s", FILENAME);
	}


	char Buffer[100];
	unsigned nBytesRead;
	while ((Result = f_read (&File, Buffer, sizeof Buffer, &nBytesRead)) == FR_OK)
	{
		if (nBytesRead > 0)
		{
			m_Screen.Write (Buffer, nBytesRead);
		}

		if (nBytesRead < sizeof Buffer)         // EOF?
		{
			break;
		}
	}

	if (Result != FR_OK)
	{
		m_Logger.Write (FromKernel, LogError, "Read error");
	}

	if (f_close (&File) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot close file");
	}


}

	
void CKernel::init_sdcard()
{
	if(m_EMMC.Initialize())
		m_Logger.Write (FromKernel, LogNotice, "SD card initialised");
	else
		m_Logger.Write (FromKernel, LogError, "SD card initialisation FAILED");

	if(FR_OK == f_mount (&m_FileSystem, DRIVE, 1)) 
		m_Logger.Write(FromKernel, LogNotice, "Mounted drive: %s", DRIVE);
	else
		m_Logger.Write (FromKernel, LogError, "Cannot mount drive: %s", DRIVE);

	/* You typically unmount it by calling:
	 * f_mount (0, DRIVE, 0);
	 */
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");
		return ShutdownHalt;
	}

	//init_sdcard();
	m_keyb = new CKeyboardBuffer(pKeyboard);


	//test_sdcard(); return m_ShutdownMode;


	main_routine();
	return m_ShutdownMode;



}


void CKernel::ShutdownHandler (void)
{
	assert (g_kernel != 0);
	g_kernel->m_ShutdownMode = ShutdownReboot;
}



