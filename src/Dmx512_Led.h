/** @brief This is a pulgin of RBK
******************************************************************************
*  @file Dmx512_Led.h
*  @author @darboy @piaoxu93 @qunge12345
*  @version V1.0.0
*  @date  2018-08-24
*  @note  Data calculator class must inheritance ICalculator like @BatteryCalculator
******************************************************************************/
#ifndef _DMX512_LED_H_
#define _DMX512_LED_H_
#include <robokit/core/rbk_core.h>
#include <robokit/utils/serial_port.h>
#include "message_odometer.pb.h"
#include "message_battery.pb.h"
#include "message_controller.pb.h"
#include "message_movetask.pb.h"

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
		rbk::MutableParam<std::string> _com;
		rbk::MutableParam<int> _color_r;
		rbk::MutableParam<int> _color_g;
		rbk::MutableParam<int> _color_b;
		rbk::MutableParam<int> _color_w;
		rbk::MutableParam<bool> isShowCharging;
		rbk::MutableParam<bool> isShowBattery;
    private:
        void modelChangedSubscriber();
		rbk::protocol::Message_Odometer m_Odometer;
		rbk::protocol::Message_Battery m_Battery;
        rbk::protocol::Message_Controller m_controller;
        rbk::protocol::Message_MoveStatus m_moveStatus;
		Clight * _hx;
		rbk::utils::json _modelJson;
        rbk::rwMutex _modelMutex;
		int is_stop_counts;
};

class ICalculator;
class CSerialport;
class Clight
{
	public:
		enum EType { EErrofatal, EBattery, EMutableBreath, ECharging, EConstantLight};
		Clight();
		~Clight();
		bool update(EType type);
		CSerialport * _pCSerialport;
		ICalculator *  getPointOfICalculator(EType type) { return _pICalculator[type]; }
	private:
		ICalculator * _pICalculator[MAX_CALC_TYPE_NUM];
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

class ICalculator
{
	public:
		virtual void calc(char * data) = 0;
};

class ErroFatalCalculator : public ICalculator
{
	public:
		void calc(char * data);
	private:
		int8_t _increment = 0;
};

class BatteryCalculator : public ICalculator
{
	public:
		void calc(char * data);
		void setBatteryPercent(double param) { _param = param; }
	private:
		double _param;

};

class MutableBreathCalculator : public ICalculator
{
	public:
		void calc(char * data);
		void setColor_RGBW(int* RGBW) {memcpy(_RGBW, RGBW, 4 * sizeof(int));}
	private:
		int8_t _increment = 0;
		int _RGBW[4];
};

class ConstantLightCalculator : public ICalculator
{
	public:
		void calc(char * data);
		void setColor_RGBW(int* RGBW) { memcpy(_RGBW, RGBW, 4 * sizeof(int)); }
	private:
		int _RGBW[4];
};

class ChargingCalculator : public ICalculator
{
	public:
		void calc(char * data);
	private:
		int8_t _increment = 0;
};

RBK_INHERIT_PROVIDER(Dmx512_Led, NPluginInterface, "1.0.0");
#endif