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
#include <atlconv.h>

using namespace std;
RBK_INHERIT_SOURCE(Dmx512_Led)

namespace 
{
	const int CHANEL_RED = 1;
	const int CHANEL_WHITE = 2;
	const int CHANEL_GREEN = 3;
	const int CHANEL_BLUE = 4;
}

Dmx512_Led::Dmx512_Led(){
	_hx = new Clight();
    _modelJson = rbk::chasis::Model::Instance()->getJson();
}

Dmx512_Led::~Dmx512_Led(){
	delete _hx;
}

void Dmx512_Led::loadFromConfigFile()
{
	loadParam(_comNum, "DmxComNum", 4, 1, 7, rbk::ParamGroup::Chassis, "The com of Dmx512_Led");
	/* turn the string to char to wchar :use the <atlconv.h> and its A2W */
	USES_CONVERSION;
	_comRaw = "COM";
	_com = _comRaw + to_string(_comNum);
	LogInfo("The com of Dmx512 is " << _com  << " !");
	_pcom = A2W(_com.c_str());
	_hx->_pCSerialport->init(_pcom);
	loadParam(_color_r, "DmxLedR", 0, 0, 255, rbk::ParamGroup::Chassis, "The R of Dmx512_Led");
	loadParam(_color_g, "DmxLedG", 255, 0, 255, rbk::ParamGroup::Chassis, "The G of Dmx512_Led");
	loadParam(_color_b, "DmxLedB", 0, 0, 255, rbk::ParamGroup::Chassis, "The B of Dmx512_Led");
	loadParam(_color_w, "DmxLedW", 0, 0, 255, rbk::ParamGroup::Chassis, "The W of Dmx512_Led");
	loadParam(isShowCharging, "isDmxLedShowCharging", true, rbk::ParamGroup::Chassis, "ShowCharging or not");
	loadParam(isShowBattery, "isDmxLedShowBattery", true, rbk::ParamGroup::Chassis, "ShowBattery or not");
}

void Dmx512_Led::setSubscriberCallBack()
{
	setTopicCallBack<rbk::protocol::Message_Odometer>(&Dmx512_Led::messageDmx512_Led_OdometerCallBack, this);
	setTopicCallBack<rbk::protocol::Message_Battery>(&Dmx512_Led::messageDmx512_Led_BatteryCallBack, this);
}

void Dmx512_Led::messageDmx512_Led_OdometerCallBack(google::protobuf::Message* msg)
{
	m_Odometer.CopyFrom(*msg);
	return;
}

void Dmx512_Led::messageDmx512_Led_BatteryCallBack(google::protobuf::Message* msg)
{
	m_Battery.CopyFrom(*msg);
	return;
}

void Dmx512_Led::run()
{
	//this->callService<void, uint16_t, bool>("DSPChassis", "setDO", 15, true);
	while (true)
	{
		SLEEP(20);
		Clight::EType et;
		color_r = _color_r;
		color_g = _color_g;
		color_b = _color_b;
		color_w = _color_w;

        try {
            /* if there exist erro or fatal ,it has the first priority*/
            if (rbk::ErrorCodes::Instance()->errorNum() || rbk::ErrorCodes::Instance()->fatalNum())
            {
                et = Clight::EErrofatal;
            }

            /* run without erro or fatal*/
            else if (!m_Odometer.is_stop())
            {
                is_stop_counts++;					//for avoiding the misoperation
                if (is_stop_counts >= 10)
                {
                    et = Clight::EMutableBreath;
                }
            }

            /* battery logic*/
            else if (_modelJson["chassis"]["batteryInfo"].get<uint32_t>() > 0)
            {
                if (m_Battery.is_charging() && isShowCharging)
                {
                    et = Clight::ECharging;
                }
                else if (m_Odometer.is_stop() && isShowBattery)
                {
                    is_stop_counts = 0;
                    bttery_percetage = m_Battery.percetage();
                    et = Clight::EBattery;
                }
            }
            else
            {
                et = Clight::EConstantLight;
            }
        }
        catch (const std::exception& e) {
            LogError(e.what());
        }
		
		_hx->update(et, bttery_percetage ,color_r, color_g, color_b, color_w);
	}
}


Clight::Clight(){
	_pILightDataCalcu[0] = new ErroFatal();
	_pILightDataCalcu[1] = new BatteryCalcu();
	_pILightDataCalcu[2] = new MutableBreath();
	_pILightDataCalcu[3] = new Charging();
	_pILightDataCalcu[4] = new ConstantLight();
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


void Clight::update(EType type, double param , int8_t R, int8_t G, int8_t B, int8_t W)
{
	memset(_data, 0x00, sizeof(_data));
	_pILightDataCalcu[type]->calc(_data, param, R, G, B, W);
	_pCSerialport->send(_data);
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
	WriteFile(_hcom, data, 512, &dwWrittenLen, NULL);
}

void ErroFatal::calc(char * data, double param, int8_t R, int8_t G, int8_t B, int8_t W)
{
	_increment = _increment + 4;
	int final_value = std::abs(_increment) * 2;
	if (final_value > 255)
	{
		final_value = 255;
	}
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = final_value;
	}
}

void BatteryCalcu::calc(char * data, double param, int8_t R, int8_t G, int8_t B, int8_t W)
{
	int red = 0xFF * (1 - param);
	int green = 0xFF * param;
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = red;
		data[i + 2] = green;
	}
}

void MutableBreath::calc(char * data, double param, int8_t R, int8_t G, int8_t B, int8_t W)
{
	_increment = _increment + 4;
	int final_value = std::abs(_increment) * 2;
	if (final_value > 255)
	{
		final_value = 255;
	}
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = ((R * final_value) / 255);
		data[i + 2] = ((G * final_value) / 255);
		data[i + 3] = ((B * final_value) / 255);
		data[i + 1] = ((W * final_value) / 255);
	}
}

void ConstantLight::calc(char * data, double param, int8_t R, int8_t G, int8_t B, int8_t W)
{
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = R;
		data[i + 2] =G;
		data[i + 3] = B;
		data[i + 1] = W;
	}
}

void Charging::calc(char * data, double param, int8_t R, int8_t G, int8_t B, int8_t W)
{
	/* Orange color is R:255 and B:165 */
	_increment = _increment + 4;
	int final_value = std::abs(_increment) * 2;
	if (final_value > 255)
	{
		final_value = 255;
	}
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = final_value;
		data[i + 2] = ((165 * final_value) / 255);
	}
}