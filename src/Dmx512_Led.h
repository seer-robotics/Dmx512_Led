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
		void messageDmx512_Led_OdometerCallBack(google::protobuf::Message* msg);
		void messageDmx512_Led_BatteryCallBack(google::protobuf::Message* msg);
		rbk::MutableParam<int> _comNum;
		rbk::MutableParam<int> _color_r;
		rbk::MutableParam<int> _color_g;
		rbk::MutableParam<int> _color_b;
		rbk::MutableParam<int> _color_w;
		rbk::MutableParam<bool> isShowCharging;
		rbk::MutableParam<bool> isShowBattery;

    private:
		rbk::protocol::Message_Odometer m_Odometer;
		rbk::protocol::Message_Battery m_Battery;
		CSerialport * _pCSerialport;
		Clight * _hx;
		wchar_t * _pcom;
		std::string _com;
		std::string _comRaw;
		rbk::utils::json _modelJson;
		int is_stop_counts;
		double bttery_percetage;
		int8_t color_r;
		int8_t color_g;
		int8_t color_b;
		int8_t color_w;
};

class ILightDataCalcu;
class CSerialport;
class Clight
{
	public:
		enum EType { EErrofatal, EBattery, EMutableBreath, ECharging, EConstantLight};
		Clight();
		~Clight();
		void update(EType type, double param, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
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
		virtual void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0) = 0;
};

class ErroFatal : public ILightDataCalcu
{
	public:
		ErroFatal() {}
		~ErroFatal() {}
		void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
	private:
		int8_t _increment = 0;
};

class BatteryCalcu : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
};

class MutableBreath : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
	private:
		int8_t _increment = 0;
};

class ConstantLight : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
};

class Charging : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int8_t R = 0, int8_t G = 0, int8_t B = 0, int8_t W = 0);
	private:
		int8_t _increment = 0;
};

RBK_INHERIT_PROVIDER(Dmx512_Led, NPluginInterface, "1.0.0");
#endif