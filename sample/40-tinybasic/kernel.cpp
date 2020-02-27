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


#define DRIVE           "SD:"
//#define DRIVE         "USB:"
#define FILENAME        "/circle.txt"


/*
   struct TCHSAddress
   {
   unsigned char Head;
   unsigned char Sector       : 6,
CylinderHigh : 2;
unsigned char CylinderLow;
}
PACKED;

struct TPartitionEntry
{
unsigned char   Status;
TCHSAddress     FirstSector;
unsigned char   Type;
TCHSAddress     LastSector;
unsigned        LBAFirstSector;
unsigned        NumberOfSectors;
}
PACKED;

struct TMasterBootRecord
{
unsigned char   BootCode[0x1BE];
TPartitionEntry Partition[4];
unsigned short  BootSignature;
#define BOOT_SIGNATURE          0xAA55
}
PACKED;
*/


static const char FromKernel[] = "kernel";

void main_basic();
CKernel *g_kernel = 0;

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
	:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_USBHCI (&m_Interrupt, &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED),
	m_ShutdownMode (ShutdownNone)
{
	s_pThis = this;
	g_kernel = this;

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
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

int getchar()
{
	static char c;
	int n;
	do { 
		n = g_kernel->m_keyb->Read(&c, 1);
	} while (n ==0);
#if 0
	g_kernel->m_Screen.Write(&c, 1);
#endif
	return c;
}

void CKernel::init_sdcard()
{
	if(m_EMMC.Initialize())
		m_Logger.Write (FromKernel, LogNotice, "SD card initialised");
	else
		m_Logger.Write (FromKernel, LogError, "SD card initialisation FAILED");

	/*
	   m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	   CDevice *pUMSD1 = m_DeviceNameService.GetDevice ("umsd1", TRUE);
	   if (pUMSD1 == 0)
	   {
	   m_Logger.Write (FromKernel, LogError, "USB mass storage device not found");

	   return ShutdownHalt;
	   }

	   u64 ullOffset = 0 * UMSD_BLOCK_SIZE;
	   if (pUMSD1->Seek (ullOffset) != ullOffset)
	   {
	   m_Logger.Write (FromKernel, LogError, "Seek error");

	   return ShutdownHalt;
	   }

	   TMasterBootRecord MBR;
	   if (pUMSD1->Read (&MBR, sizeof MBR) != (int) sizeof MBR)
	   {
	   m_Logger.Write (FromKernel, LogError, "Read error");

	   return ShutdownHalt;
	   }

	   if (MBR.BootSignature != BOOT_SIGNATURE)
	   {
	   m_Logger.Write (FromKernel, LogError, "Boot signature not found");

	   return ShutdownHalt;
	   }

	   m_Logger.Write (FromKernel, LogNotice, "Dumping the partition table");
	   m_Logger.Write (FromKernel, LogNotice, "# Status Type  1stSector    Sectors");

	   for (unsigned nPartition = 0; nPartition < 4; nPartition++)
	   {
	   m_Logger.Write (FromKernel, LogNotice, "%u %02X     %02X   %10u %10u",
	   nPartition+1,
	   (unsigned) MBR.Partition[nPartition].Status,
	   (unsigned) MBR.Partition[nPartition].Type,
	   MBR.Partition[nPartition].LBAFirstSector,
	   MBR.Partition[nPartition].NumberOfSectors);
	   }

	   return ShutdownHalt;
	   */

}

void CKernel::cmd_type(unsigned char* filename)
{
	// Mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	}


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

	// Unmount file system
	if (f_mount (0, DRIVE, 0) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot unmount drive: %s", DRIVE);
	}

}
void CKernel::test_sdcard()
{
	// Mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	}

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

	// Unmount file system
	if (f_mount (0, DRIVE, 0) != FR_OK)
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot unmount drive: %s", DRIVE);
	}

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

	//run_sdcard();
	m_keyb = new CKeyboardBuffer(pKeyboard);


	//test_sdcard(); return m_ShutdownMode;


	main_basic();
	return m_ShutdownMode;



}

/*
   void CKernel::KeyPressedHandler (const char *pString)
   {
   assert (s_pThis != 0);
   s_pThis->m_Screen.Write (pString, strlen (pString));
   }
   */

void CKernel::ShutdownHandler (void)
{
	assert (s_pThis != 0);
	s_pThis->m_ShutdownMode = ShutdownReboot;
}

/*
   void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
   {
   assert (s_pThis != 0);

   CString Message;
   Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);

   for (unsigned i = 0; i < 6; i++)
   {
   if (RawKeys[i] != 0)
   {
   CString KeyCode;
   KeyCode.Format (" %02X", (unsigned) RawKeys[i]);

   Message.Append (KeyCode);
   }
   }

   s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
   }
   */

int putchar(int c)
{
	g_kernel->m_Screen.Write(&c, 1);
	return c;
}
