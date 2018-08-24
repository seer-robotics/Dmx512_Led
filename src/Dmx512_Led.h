#ifndef _DMX512_LED_H_
#define _DMX512_LED_H_
#include <robokit/core/rbk_core.h>
#include <robokit/utils/serial_port.h>
#include "message_odometer.pb.h"
#include "message_battery.pb.h"

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
		rbk::MutableParam<std::string> _com;
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
		rbk::utils::json _modelJson;
		int is_stop_counts;
		double bttery_percetage;
		int color_r;
		int color_g;
		int color_b;
		int color_w;
};

class ILightDataCalcu;
class CSerialport;
class Clight
{
	public:
		enum EType { EErrofatal, EBattery, EMutableBreath, ECharging, EConstantLight};
		Clight();
		~Clight();
		bool update(EType type, double param, int R = 0, int G = 0, int B = 0, int W = 0);
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
        bool init(const std::string& com);
		bool send(const char *pdata, size_t);
	private:
        rbk::utils::serialport::SyncSerial _hcom;
};

class ILightDataCalcu
{
	public:
		virtual void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0) = 0;
};

class ErroFatal : public ILightDataCalcu
{
	public:
		ErroFatal() {}
		~ErroFatal() {}
		void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0);
	private:
		int8_t _increment = 0;
};

class BatteryCalcu : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0);
};

class MutableBreath : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0);
	private:
		int8_t _increment = 0;
};

class ConstantLight : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0);
};

class Charging : public ILightDataCalcu
{
	public:
		void calc(char * data, double param = 0, int R = 0, int G = 0, int B = 0, int W = 0);
	private:
		int8_t _increment = 0;
};

RBK_INHERIT_PROVIDER(Dmx512_Led, NPluginInterface, "1.0.0");
#endif