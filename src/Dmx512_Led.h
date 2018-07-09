#ifndef _DMX512_LED_H_
#define _DMX512_LED_H_
#include <robokit/core/rbk_core.h>
#include "message_odometer.pb.h"
#include "message_battery.pb.h"
#include <iostream>

#define MAX_CALC_TYPE_NUM 10
using namespace rbk;

class Clight;
class CSerialport;
class Dmx512_Led:public NPluginInterface
{
    public: 
        Dmx512_Led();
        ~Dmx512_Led();
        void run();
        void loadFromConfigFile();
	    void setSubscriberCallBack();
		void messageDmx512_Led_Is_StopCallBack(google::protobuf::Message* msg);
		void messageDmx512_Led_BatteryCallBack(google::protobuf::Message* msg);
		rbk::MutableParam<int> _comNum;
    private:
		rbk::protocol::Message_Odometer m_Odometer;
		rbk::protocol::Message_Battery m_Battery;
		CSerialport * _pCSerialport;
		Clight * _hx;
		wchar_t * _pcom;
		std::string _com;
		std::string _comRaw;
};

class ILightDataCalcu;
class CSerialport;
class Clight
{
public:
	enum EType { EFatalw = 0, ERun, EStop, EFlow };
	Clight();
	~Clight();
	void update(EType type, double param);
	CSerialport * _pCSerialport;
private:
	ILightDataCalcu * _pILightDataCalcu[MAX_CALC_TYPE_NUM];
	char _data[512];
};

class CSerialport
{
	public:
		CSerialport();
		~CSerialport();
		void init(CONST WCHAR *LPCWSTR);
		void send(const char *pdata);
	private:
		HANDLE _hcom;
};

class ILightDataCalcu
{
	public:
		virtual void calc(char * data, double param = 0) = 0;
};

class FatalWarrningCalcu : public ILightDataCalcu
{
	public:
		FatalWarrningCalcu() { count = 1; brightness = 0x00; }
		~FatalWarrningCalcu() {}
		void calc(char * data, double param = 0);
	private:
		int count;
		int brightness;
		int8_t _aa = 0;
};

class RunCalcu : public ILightDataCalcu
{
	public:
		RunCalcu(){ count = 1; brightness = 0x00; }
		~RunCalcu(){}
		void calc(char * data, double param = 0);
	private:
		int count;
		int brightness;
		int8_t _aa = 0;
};

class BatteryCalcu : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0);
};

RBK_INHERIT_PROVIDER(Dmx512_Led, NPluginInterface, "1.0.0");
#endif