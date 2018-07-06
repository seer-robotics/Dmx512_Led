#include<Windows.h>
#include<iostream>
#include "Dmx512_Led.h"
#include "message_odometer.pb.h"
#include "message_battery.pb.h"
#include <bitset>
#include <strstream>
#include <robokit/foundation/utils/geo_utils.h>
#include <robokit/chasis/model.h>
#include <robokit/core/error.h>
#include <boost/lexical_cast.hpp>
#include <robokit/chasis/model.h>
#include <iostream>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
using namespace std;
#include <atlconv.h>
RBK_INHERIT_SOURCE(Dmx512_Led)

Dmx512_Led::Dmx512_Led(){
	_hx = new Clight();
}

Dmx512_Led::~Dmx512_Led(){
	delete _hx;
}

void Dmx512_Led::loadFromConfigFile()
{
	loadParam(_com,"Dmx512LedCom","COM4");
	//turn the string to char to wchar :use the <atlconv.h> and its A2W.
	USES_CONVERSION;
	_pcom = A2W(_com.c_str());
	_hx->_pCSerialport->init(_pcom);
}
//订阅里程、电池信息
void Dmx512_Led::setSubscriberCallBack()
{
	setTopicCallBack<rbk::protocol::Message_Odometer>(&Dmx512_Led::messageDmx512_Led_Is_StopCallBack, this);
	setTopicCallBack<rbk::protocol::Message_Battery>(&Dmx512_Led::messageDmx512_Led_BatteryCallBack, this);
}
//获取里程信息
void Dmx512_Led::messageDmx512_Led_Is_StopCallBack(google::protobuf::Message* msg)
{
	m_Odometer.CopyFrom(*msg);
	return;
}
//获取电池信息
void Dmx512_Led::messageDmx512_Led_BatteryCallBack(google::protobuf::Message* msg)
{
	m_Battery.CopyFrom(*msg);
	return;
}

void Dmx512_Led::run()
{
	while (true)
	{
		SLEEP(20);
		Clight::EType et;
		double param = 0;
		if (rbk::ErrorCodes::Instance()->errorNum() || rbk::ErrorCodes::Instance()->fatalNum())
		{
			et = Clight::EFatalw;
		}
		else if(!m_Odometer.is_stop())
		{
			et = Clight::ERun;
		}
		else if(m_Odometer.is_stop())
		{
			param = m_Battery.percetage();
			et = Clight::EStop;
		}
		_hx->update(et, param);
	}
}
Clight::Clight(){
	_pILightDataCalcu[0] = new FatalWarrningCalcu();
	_pILightDataCalcu[1] = new RunCalcu();
	_pILightDataCalcu[2] = new BatteryCalcu();
	_pCSerialport = new CSerialport();
}
Clight::~Clight() {
	if (NULL != _pILightDataCalcu)
	{
		delete _pILightDataCalcu;
	}
	if (NULL != _pCSerialport)
	{
		delete _pCSerialport;
	}
}


void Clight::update(EType type, double param)
{
	_pILightDataCalcu[type]->calc(_data, param);
	_pCSerialport->send(_data);
	memset(_data, 0x00, sizeof(_data));
}

CSerialport::CSerialport(){
	_hcom = NULL;
}

CSerialport::~CSerialport() {
	CloseHandle(_hcom);
}

void CSerialport::init(CONST WCHAR *LPCWSTR)
{
	_hcom = CreateFile(LPCWSTR, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (_hcom == INVALID_HANDLE_VALUE) 
	{ 
		LogError("The init of Dmx512 failed!");
	} 
	else
	{
		LogInfo("The init of Dmx512 success!");
	}
	SetupComm(_hcom, 1024, 1024);
	DCB dcb;
	GetCommState(_hcom, &dcb);
	dcb.BaudRate = 250000;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = 2;
	SetCommState(_hcom, &dcb);
	
}

void CSerialport::send(const char *data)
{
	
	DWORD dwWrittenLen = 0;
	SetCommBreak(_hcom);	Sleep(10);
	ClearCommBreak(_hcom);	Sleep(0.1);
	bool bSend =WriteFile(_hcom, data, 512, &dwWrittenLen, NULL);
}

void FatalWarrningCalcu::calc(char * data, double param)
{
	_aa = _aa + 4;
	int bb = std::abs(_aa) * 2;
	if (bb > 255)
	{
		bb = 255;
	}
	for (int i = 2; i <= 512; i = i + 4)
	{
		data[i] = bb; 
	}
}

void RunCalcu::calc(char * data, double param)
{
	_aa = _aa + 4;
	int bb = std::abs(_aa) * 2;
	if (bb > 255)
	{
		bb = 255;
	}
	for (int i = 3; i <= 512; i = i + 4)
	{
		data[i] = bb;
	}
}

void BatteryCalcu::calc(char * data, double param)
{
	int red = 0xFF * (1 - param);
	int green = 0xFF * param;
	for (int i = 1; i <= 512; i = i + 4)
	{
		data[i] = red;
		data[i + 1] = green;
	}
}




