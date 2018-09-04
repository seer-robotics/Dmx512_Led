#include "Dmx512_Led.h"
#include <robokit/chasis/model.h>
#include <robokit/core/error.h>

using namespace std;
RBK_INHERIT_SOURCE(Dmx512_Led)

/*The chanel of light always changes, define the chanel in correct position*/
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

/*By the roboshop ,can change the 7 params such as com, color_r, color_g, color_b, color_w, isShowCharging & isShowBattery*/
void Dmx512_Led::loadFromConfigFile() {
	loadParam(_com, "DmxCom", "COM4", rbk::ParamGroup::Chassis, "The com of Dmx512_Led");
	LogInfo("The com of Dmx512 is " << _com  << " !");
	loadParam(_color_r, "DmxLedR", 0, 0, 255, rbk::ParamGroup::Chassis, "The R of Dmx512_Led");
	loadParam(_color_g, "DmxLedG", 80, 0, 255, rbk::ParamGroup::Chassis, "The G of Dmx512_Led");
	loadParam(_color_b, "DmxLedB", 164, 0, 255, rbk::ParamGroup::Chassis, "The B of Dmx512_Led");
	loadParam(_color_w, "DmxLedW", 0, 0, 255, rbk::ParamGroup::Chassis, "The W of Dmx512_Led");
	loadParam(isShowCharging, "isDmxLedShowCharging", true, rbk::ParamGroup::Chassis, "ShowCharging or not");
	loadParam(isShowBattery, "isDmxLedShowBattery", true, rbk::ParamGroup::Chassis, "ShowBattery or not");
}

void Dmx512_Led::setSubscriberCallBack() {
	setTopicCallBack<rbk::protocol::Message_Odometer>();
	setTopicCallBack<rbk::protocol::Message_Battery>();
    setTopicCallBack<rbk::protocol::Message_Controller>();
    setTopicCallBack<rbk::protocol::Message_MoveStatus>();

    rbk::chasis::Model::Instance()->connectChangedSignal(std::bind(&Dmx512_Led::modelChangedSubscriber, this));
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
            getSubscriberData(m_moveStatus);
            getSubscriberData(m_controller);
            
            Clight::EType et = isShowBattery ? Clight::EBattery : Clight::EConstantLight;

			/*Defint a int arrary to add the color together,the param is set by user.
			If use it to fix a certain color ,can redefine it before @setColor_RGBW*/
			int RGBW[4] = { _color_r, _color_g, _color_b, _color_w };

            rbk::readLock l(_modelMutex);
            try {
				/*	ErrorFatal logic order:
				1.Red breath light when regard the errornum > 0 or fatalnum > 0 as occur fatal or error
				*/
                if (rbk::ErrorCodes::Instance()->errorNum() > 0 || rbk::ErrorCodes::Instance()->fatalNum() > 0) {
                    et = Clight::EErrofatal;
                }

				/*	Run logic order:
				1.The chassis of encoder change or not
				2.Regard the times of slight move >=10 as the chassis run  
				3.the times not reach 10
				*/
                else if (!m_Odometer.is_stop()) {
                    if (is_stop_counts >= 20) {
                        et = Clight::EMutableBreath;
						MutableBreathCalculator * pMutableBreath = dynamic_cast<MutableBreathCalculator * >(_hx->getPointOfICalculator(et)); 
						pMutableBreath->setColor_RGBW(RGBW);
                    }
                    else {
                        is_stop_counts++;
                    }
                }

                // ========================== for ABB ==========================
                // emc stop
                else if (m_controller.emc()) {
                    et = Clight::EConstantLight;
                    int RGBW[4] = { 255, 0, 0, 0 };
                    ConstantLightCalculator * pConstantLight = dynamic_cast<ConstantLightCalculator *>(_hx->getPointOfICalculator(et));
                    pConstantLight->setColor_RGBW(RGBW);
                }

                // blocked
                else if (m_moveStatus.blocked()) {
                    et = Clight::EErrofatal;
                }
                // =============================================================

                /*	Battery logic order:
					1.Orange breath light when charging && param @isShowChargine is true
					2.Red constant light when the percent of battery < 30% 
					3.Red-Green constant light when the chassis is stop && param @isShowBattery is true
					4.Constant light with color set by user 
				*/ 
                else if (_modelJson["chassis"]["batteryInfo"].get<uint32_t>() > 0) {
                    if (m_Battery.is_charging() && isShowCharging) {
                        et = Clight::ECharging;
                    }
                    else if (m_Battery.percetage() < 0.3) {
                        et = Clight::EConstantLight;
                        int RGBW[4] = { 255, 0, 0, 0 };
						ConstantLightCalculator * pConstantLight = dynamic_cast<ConstantLightCalculator *>(_hx->getPointOfICalculator(et));
						pConstantLight->setColor_RGBW(RGBW);
                    }
                    else if (m_Odometer.is_stop() && isShowBattery) {
                        et = Clight::EBattery;
						BatteryCalculator * pbattery = dynamic_cast<BatteryCalculator *>(_hx->getPointOfICalculator(et));
						pbattery->setBatteryPercent(m_Battery.percetage());
                    }
                    else {
                        et = Clight::EConstantLight;
                        ConstantLightCalculator * pConstantLight = dynamic_cast<ConstantLightCalculator *>(_hx->getPointOfICalculator(et));
                        pConstantLight->setColor_RGBW(RGBW);
                    }
                }
                else {
                    et = Clight::EConstantLight;
                    ConstantLightCalculator * pConstantLight = dynamic_cast<ConstantLightCalculator *>(_hx->getPointOfICalculator(et));
                    pConstantLight->setColor_RGBW(RGBW);
                }

                if (m_Odometer.is_stop()) {
                    is_stop_counts = 0;
                }
            }
            catch (const std::exception& e) {
                LogError(e.what());
            }
            if (!_hx->update(et)) {
                break;
            }
        }
    }
}

Clight::Clight() {
	_pICalculator[0] = new ErroFatalCalculator();
	_pICalculator[1] = new BatteryCalculator();
	_pICalculator[2] = new MutableBreathCalculator();
	_pICalculator[3] = new ChargingCalculator();
	_pICalculator[4] = new ConstantLightCalculator();
	_pCSerialport = new CSerialport();
}

Clight::~Clight() {
	if (NULL != _pICalculator) {
		delete[] _pICalculator;
	}
	if (NULL != _pCSerialport) {
		delete _pCSerialport;
        _pCSerialport = NULL;
	}
}

/**
* @brief Calculate the data and send it
* @param[in] type
*		-EErrofatal
*		-EBattery
*		-EMutableBreath
*		-ECharging
*		-EConstantLight
* @note Reset the data before calculate the data to ensure its data won't be disorder.
* @retval -true Success -false Fail
*/
bool Clight::update(EType type) {
    memset(_data, 0x00, sizeof(_data));
    _pICalculator[type]->calc(_data);
    return _pCSerialport->send(_data, 512);
}

CSerialport::CSerialport() { }

CSerialport::~CSerialport() {
    _hcom.flush();
    _hcom.close();
}

/**
* @brief Initialize the serial port
* @param[in] com
*		-"COM4"
* @note Baud rate: 250000, stop bits:2, parity: none, character size:8
* @return If successful loginfo,else logerro
* @retval -true Success -false Fail
*/
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

/**
* @brief Send data
* @param data 
* @param size -512
* @note @SetCommBreak @ClearCommBreak
* @retval -true Success -false Fail
*/
bool CSerialport::send(const char *data, size_t size) {
	DWORD dwWrittenLen = 0;
	SetCommBreak(_hcom.port().native_handle());   SLEEP(10);  //depand on its protocol
	ClearCommBreak(_hcom.port().native_handle()); SLEEP(1);
    boost::system::error_code ec;
    _hcom.write(data, size, ec);
    if (ec) {
        LogError("Send command error: " << ec.message());
        return false;
    }
    return true;
}

/**
* @brief Error calculator
* @param data
* @note This is a breath light of red.
* @return None
*/
void ErroFatalCalculator::calc(char * data) {
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

/**
* @brief Battery calculator
* @param data
* @note "_param" depands on the percetage of battery .
* @return None
*/
void BatteryCalculator::calc(char * data) {
	int red = 0xFF * (1 - _param);
	int green = 0xFF * _param;
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = red;
		data[i + 2] = green;
	}
}

/**
* @brief Mutable breath calculator
* @param data
* @note The value of data is constant scale change ,also a breath light but can change the color.
* @return None
*/
void MutableBreathCalculator::calc(char * data) {
	_increment = _increment + 4;
	int final_value = std::abs(_increment) * 2;
	if (final_value > 255)
	{
		final_value = 255;
	}
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = ((_RGBW[0] * final_value) / 255);
		data[i + 2] = ((_RGBW[1] * final_value) / 255);
		data[i + 3] = ((_RGBW[2] * final_value) / 255);
		data[i + 1] = ((_RGBW[3] * final_value) / 255);
	}
}

/**
* @brief ConstantLight calculator
* @param data
* @note The value of data is a constant color.
* @return None
*/
void ConstantLightCalculator::calc(char * data) {
	for (int i = CHANEL_RED; i <= 50; i = i + 4)
	{
		data[i] = _RGBW[0];
		data[i + 2] = _RGBW[1];
		data[i + 3] = _RGBW[2];
		data[i + 1] = _RGBW[3];
	}
}

/**
* @brief Charging calculator
* @param data
* @note Constant breath light of orange color.
* @return None
*/
void ChargingCalculator::calc(char * data) {
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
