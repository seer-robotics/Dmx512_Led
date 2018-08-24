#include "Dmx512_Led.h"

#include <robokit/chasis/model.h>
#include <robokit/core/error.h>

using namespace std;
RBK_INHERIT_SOURCE(Dmx512_Led)

namespace {
	const int CHANEL_RED = 1;
	const int CHANEL_WHITE = 2;
	const int CHANEL_GREEN = 3;
	const int CHANEL_BLUE = 4;
}

Dmx512_Led::Dmx512_Led() {
	_hx = new Clight();
    _modelJson = rbk::chasis::Model::Instance()->getJson();
}

Dmx512_Led::~Dmx512_Led() {
	delete _hx;
}

void Dmx512_Led::loadFromConfigFile() {
	loadParam(_com, "DmxCom", "COM4", rbk::ParamGroup::Chassis, "The com of Dmx512_Led");
	LogInfo("The com of Dmx512 is " << _com  << " !");
	loadParam(_color_r, "DmxLedR", 0, 0, 255, rbk::ParamGroup::Chassis, "The R of Dmx512_Led");
	loadParam(_color_g, "DmxLedG", 255, 0, 255, rbk::ParamGroup::Chassis, "The G of Dmx512_Led");
	loadParam(_color_b, "DmxLedB", 0, 0, 255, rbk::ParamGroup::Chassis, "The B of Dmx512_Led");
	loadParam(_color_w, "DmxLedW", 0, 0, 255, rbk::ParamGroup::Chassis, "The W of Dmx512_Led");
	loadParam(isShowCharging, "isDmxLedShowCharging", true, rbk::ParamGroup::Chassis, "ShowCharging or not");
	loadParam(isShowBattery, "isDmxLedShowBattery", true, rbk::ParamGroup::Chassis, "ShowBattery or not");
}

void Dmx512_Led::setSubscriberCallBack() {
	setTopicCallBack<rbk::protocol::Message_Odometer>(&Dmx512_Led::messageDmx512_Led_OdometerCallBack, this);
	setTopicCallBack<rbk::protocol::Message_Battery>(&Dmx512_Led::messageDmx512_Led_BatteryCallBack, this);

    rbk::chasis::Model::Instance()->connectChangedSignal(std::bind(&Dmx512_Led::modelChangedSubscriber, this));
}

void Dmx512_Led::messageDmx512_Led_OdometerCallBack(google::protobuf::Message* msg) {
	m_Odometer.CopyFrom(*msg);
	return;
}

void Dmx512_Led::messageDmx512_Led_BatteryCallBack(google::protobuf::Message* msg) {
	m_Battery.CopyFrom(*msg);
	return;
}

void Dmx512_Led::modelChangedSubscriber() {
    rbk::ThreadPool::Instance()->schedule([this]() {
        rbk::writeLock l(this->_modelMutex);
        this->_modelJson = rbk::chasis::Model::Instance()->getJson();
        LogInfo(formatLog("Text", "model updated"));
    });
}

void Dmx512_Led::run() {
    while (true) {

        while (true) {
            SLEEP(2000);
            if (_hx->_pCSerialport->init(_com.get())) {
                break;
            }
        }

        while (true) {
            SLEEP(20);
            getSubscriberData(m_Battery);
            getSubscriberData(m_Odometer);
            Clight::EType et = Clight::EConstantLight;
            color_r = _color_r;
            color_g = _color_g;
            color_b = _color_b;
            color_w = _color_w;

            rbk::readLock l(_modelMutex);
            try {
                /* if there exist erro or fatal ,it has the first priority*/
                if (rbk::ErrorCodes::Instance()->errorNum() > 0 || rbk::ErrorCodes::Instance()->fatalNum() > 0) {
                    et = Clight::EErrofatal;
                }

                /* run without erro or fatal*/
                else if (!m_Odometer.is_stop()) {
                    if (is_stop_counts >= 10) {
                        et = Clight::EMutableBreath;
                    }
                    else {
                        is_stop_counts++;    //for avoiding the misoperation
                    }
                }

                /* battery logic*/
                else if (_modelJson["chassis"]["batteryInfo"].get<uint32_t>() > 0) {
                    if (m_Battery.is_charging() && isShowCharging) {
                        et = Clight::ECharging;
                    }
                    else if (m_Battery.percetage() < 0.3) {
                        et = Clight::EConstantLight;
                        color_r = 255;
                        color_g = 0;
                        color_b = 0;
                        color_w = 0;
                    }
                    else if (m_Odometer.is_stop() && isShowBattery) {
                        is_stop_counts = 0;
                        bttery_percetage = m_Battery.percetage();
                        et = Clight::EBattery;
                    }
                    else {
                        et = Clight::EConstantLight;
                    }
                }
            }
            catch (const std::exception& e) {
                LogError(e.what());
            }

            if (!_hx->update(et, bttery_percetage, color_r, color_g, color_b, color_w)) {
                break;
            }
        }
    }
}

Clight::Clight() {
	_pILightDataCalcu[0] = new ErroFatal();
	_pILightDataCalcu[1] = new BatteryCalcu();
	_pILightDataCalcu[2] = new MutableBreath();
	_pILightDataCalcu[3] = new Charging();
	_pILightDataCalcu[4] = new ConstantLight();
	_pCSerialport = new CSerialport();
}

Clight::~Clight() {
	if (NULL != _pILightDataCalcu) {
		delete[] _pILightDataCalcu;
	}
	if (NULL != _pCSerialport) {
		delete _pCSerialport;
        _pCSerialport = NULL;
	}
}

bool Clight::update(EType type, double param , int R, int G, int B, int W) {
    memset(_data, 0x00, sizeof(_data));
    _pILightDataCalcu[type]->calc(_data, param, R, G, B, W);
    return _pCSerialport->send(_data, 512);
}

CSerialport::CSerialport() { }

CSerialport::~CSerialport() {
    _hcom.flush();
    _hcom.close();
}

bool CSerialport::init(const std::string& com) {
    boost::system::error_code ec;
    _hcom.open(com, 250000, boost::asio::serial_port_base::parity(
        boost::asio::serial_port_base::parity::none), boost::asio::serial_port_base::character_size(8), boost::asio::serial_port_base::flow_control(
        boost::asio::serial_port_base::flow_control::none), boost::asio::serial_port_base::stop_bits(
        boost::asio::serial_port_base::stop_bits::two), ec);
    _hcom.setTimeout(std::chrono::microseconds(2000));
	if (ec) {
		LogError("The init of Dmx512 failed: " << ec.message());
        return false;
	} 
	else {
		LogInfo("The init of Dmx512 success!");
        return true;
	}
}

bool CSerialport::send(const char *data, size_t size) {
	DWORD dwWrittenLen = 0;
	SetCommBreak(_hcom.port().native_handle());   SLEEP(10);
	ClearCommBreak(_hcom.port().native_handle()); SLEEP(0.1);
    boost::system::error_code ec;
    _hcom.write(data, size, ec);
    if (ec) {
        LogError("Send command error: " << ec.message());
        return false;
    }
    return true;
}

void ErroFatal::calc(char * data, double param, int R, int G, int B, int W) {
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

void BatteryCalcu::calc(char * data, double param, int R, int G, int B, int W) {
	int red = 0xFF * (1 - param);
	int green = 0xFF * param;
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = red;
		data[i + 2] = green;
	}
}

void MutableBreath::calc(char * data, double param, int R, int G, int B, int W) {
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

void ConstantLight::calc(char * data, double param, int R, int G, int B, int W) {
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = R;
		data[i + 2] =G;
		data[i + 3] = B;
		data[i + 1] = W;
	}
}

void Charging::calc(char * data, double param, int R, int G, int B, int W) {
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
