//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <circle/string.h>
#include <circle/debug.h>
#include <assert.h>
//#include "forth.h"
#include <circle/interrupt.h>
#include <circle/timer.h>
//#include <circle/util.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/devicenameservice.h>

CInterruptSystem irqs1;
CTimer mytimer(&irqs1);
//CTimer timer;

CUSBHCIDevice usbdev(&irqs1, &mytimer, 1);
//CUSBHCIDevice usbdev(&irqs, &timer, 1);


int strlen(const char* str)
{
	int n = 0;
	while(*str++) n++;
	return n;
}

int puts(const char* str) 
{
	//char* s = str;
	//const char* text = "hello world";
	//while(*s) scr.Write(*s++, 1);
	scr.Write(str, strlen(str));
	scr.Write("\n", 1);
	return 1;
}
	

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	
	//m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Logger (m_Options.GetLogLevel ())
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	//if (bOK)
	//{
	//	bOK = m_Screen.Initialize ();
	//}
	bOK = scr.Initialize();
	
	puts("screen initialised");
	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}
	puts("serial initialised");
	
	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &scr;
		}

		bOK = m_Logger.Initialize (pTarget);
	}
	puts("logger initialised");
	
	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	forth_main();

	// show the character set on screen
	for (char chChar = ' '; chChar <= '~'; chChar++)
	{
		if (chChar % 8 == 0)
		{
			scr.Write ("\n", 1);
		}

		CString Message;
		Message.Format ("%02X: \'%c\' ", (unsigned) chChar, chChar);
		
		scr.Write ((const char *) Message, Message.GetLength ());
	}
	scr.Write ("\n", 1);

#ifndef NDEBUG
	// some debugging features
	m_Logger.Write (FromKernel, LogDebug, "Dumping the start of the ATAGS");
	debug_hexdump ((void *) 0x100, 128, FromKernel);

	m_Logger.Write (FromKernel, LogNotice, "The following assertion will fail");
	assert (1 == 2);
#endif

	return ShutdownHalt;
}
