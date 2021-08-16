#include "SpiHandler.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <QDateTime>
#include <sstream>

namespace
{
    enum SPI_Command
    {
        RPI_INIT = 0,
        STM_CONNECTED_ACK,
        RPI_GET_DATE_TIME,
        SEND_SENSOR_DATA
    };

    const char* spi_commands[] =
    {
        "RPI_INI",
        "STM_ACK",
        "STM_GET",
        "RPI_GET"
    };

    const uint8_t bufferSize = 10U;
    const uint8_t MSG_ID = 9U;

    struct SPI_TxDateTime
    {
        //Date and time data
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    struct SPI_TxSensorData
    {
        uint8_t humidity_int;
        uint8_t humidity_dec;
        uint8_t temp_int;
        uint8_t temp_dec;
        uint8_t adcVoltage : 3;
        uint8_t moveSensorState : 1;
    };

    SPI_TxSensorData m_sensorData;
    bool m_transferNewData{false};    
}

SpiHandler::SpiHandler()
{

}

SpiHandler::~SpiHandler()
{
    deinitDataTransfer();
}

void SpiHandler::run()
{
    if (m_initialized)
    {
        while (true)
        {
            if (m_transferNewData)
            {
                if (transferData())
                {
                    m_transferNewData = false;
                }
            }
            //check rx data
            processRxData();
            msleep(100);
        }
    }
}

bool SpiHandler::setConfig(const SpiConfig &spiConfig)
{
    m_spiConfig = spiConfig;

    m_txBuffer.resize(spiConfig.bufferSize);
    m_rxBuffer.resize(spiConfig.bufferSize);

    m_spiTransfer.tx_buf = reinterpret_cast<unsigned long>(m_txBuffer.data());
    m_spiTransfer.rx_buf = reinterpret_cast<unsigned long>(m_rxBuffer.data());
    m_spiTransfer.len = spiConfig.bufferSize;
    m_spiTransfer.delay_usecs = spiConfig.delay;
    m_spiTransfer.speed_hz = spiConfig.speed;
    m_spiTransfer.bits_per_word = spiConfig.bits;

    return true;
}

SpiHandler::SpiConfig SpiHandler::getSpiConfig() const
{
    return m_spiConfig;
}

/*
 * transferOptions:
 * SPI_LOOP
 * SPI_CPHA
 * SPI_CPOL
 * SPI_LSB_FIRST
 * SPI_CS_HIGH
 * SPI_3WIRE
 * SPI_NO_CS
 * SPI_READY
*/
bool SpiHandler::initDataTransfer(int transferOptions)
{
    bool initSuccess = false;
    int ret = 1;
    m_fileHandle = open(m_spiConfig.deviceName.c_str(), O_RDWR);
    if (m_fileHandle >= 0)
    {
        m_spiConfig.mode = transferOptions;        
        /*
         * spi mode
         */
        ret &= (ioctl(m_fileHandle, SPI_IOC_WR_MODE, &m_spiConfig.mode) == -1) ? 0 : 1;
        ret &= (ioctl(m_fileHandle, SPI_IOC_RD_MODE, &m_spiConfig.mode) == -1) ? 0 : 1;
        /*
         * bits per word
         */
        ret &= (ioctl(m_fileHandle, SPI_IOC_WR_BITS_PER_WORD, &m_spiConfig.bits) == -1) ? 0 : 1;
        ret &= (ioctl(m_fileHandle, SPI_IOC_RD_BITS_PER_WORD, &m_spiConfig.bits) == -1) ? 0 : 1;

        /*
         * max speed hz
         */
        ret &= (ioctl(m_fileHandle, SPI_IOC_WR_MAX_SPEED_HZ, &m_spiConfig.speed) == -1) ? 0 : 1;
        ret &= (ioctl(m_fileHandle, SPI_IOC_RD_MAX_SPEED_HZ, &m_spiConfig.speed) == -1) ? 0 : 1;

        if (ret)
        {
            initSuccess = true;
            m_initialized = true;
            this->start();
        }
    }
    return initSuccess;
}

bool SpiHandler::deinitDataTransfer()
{
    bool deinitialized = false;
    if (m_initialized)
    {
        close(m_fileHandle);
        deinitialized = true;
        m_initialized = false;
    }
    return deinitialized;
}

void SpiHandler::setLogWindowRef(QQuickItem *logWindow)
{
    if (logWindow && !m_logWindow)
    {
        m_logWindow = logWindow;
    }
}

inline bool SpiHandler::transferData()
{
    std::ostringstream newData;
    newData << "New tranfer msg : " << m_txBuffer.toStdString();
    emit updateLogWindow(QString{newData.str().c_str()});
    return (ioctl(m_fileHandle, SPI_IOC_MESSAGE(1), &m_spiTransfer) >= 1);
}

bool SpiHandler::newDataTransfer(const QByteArray &txData)
{
    bool dataTransferStarted = false;
    if ((txData.size() <= (m_txBuffer.size() - 1)) && !m_transferNewData)
    {
        uint8_t idx = 0U;
        for(auto && byte : txData)
        {
            m_txBuffer[idx] = byte;
            ++idx;
        }
        m_txBuffer[MSG_ID] = m_txMsgId++;
        m_transferNewData = true;
        dataTransferStarted = true;
    }
    return dataTransferStarted;
}

void SpiHandler::stopDataTransfer()
{

}

void SpiHandler::onUiStartComm()
{
    if(m_logWindow)
    {
        if(!m_initialized)
        {
            if (initDataTransfer())
            {
                newDataTransfer(spi_commands[RPI_INIT]);
                emit updateLogWindow("Start Communication");
            }
        }
        else
        {
            emit updateLogWindow("Comm already started");
        }

    }
}

void SpiHandler::onUiRequestSensorData()
{
//    if(m_logWindow && m_commEstablished)
//    {
//        emit updateLogWindow("Request Sensor Data");
//    }
    newDataTransfer(spi_commands[SEND_SENSOR_DATA]);
}

bool SpiHandler::processRxData()
{
    //process spi data frame
    bool rxDataProcessed = false;
    m_commEstablished = true; // temp, rmeove after tests
    if (m_commEstablished)
    {
        if ((m_oldRxMsgId != m_rxBuffer[MSG_ID]))
        {
            m_oldRxMsgId = m_rxBuffer[MSG_ID];
//            if (std::string::npos != m_rxBuffer.toStdString()
//                    .find(spi_commands[RPI_GET_DATE_TIME]))
//            {
//                //date and time stm request - prepare transfer time stamp
//                QByteArray timeStamp;
//                SPI_TxDateTime dateTimeMsg;
//                QDateTime current = QDateTime::currentDateTime();

//                dateTimeMsg.day = current.date().day();
//                dateTimeMsg.month = current.date().month();
//                dateTimeMsg.year = current.date().year();
//                dateTimeMsg.second = current.time().second();
//                dateTimeMsg.minute = current.time().minute();
//                dateTimeMsg.hour = current.time().hour();

//                timeStamp.append(reinterpret_cast<const char*>(&dateTimeMsg));
//                rxDataProcessed = newDataTransfer(timeStamp);

//            }
//            else
//            {
//                //otherwise rxData is sensor Data
//                m_sensorData.humidity_int = m_rxBuffer[0];
//                m_sensorData.humidity_dec = m_rxBuffer[1];
//                m_sensorData.temp_int = m_rxBuffer[2];
//                m_sensorData.temp_dec = m_rxBuffer[3];
//                m_sensorData.adcVoltage = m_rxBuffer[4];
//                m_sensorData.moveSensorState = m_rxBuffer[5];

//                rxDataProcessed = true;
//            }
            //log new Data received
            emit updateLogWindow("New Rx Data: ");
            emit updateLogWindow(m_rxBuffer);
        }
    }
    else
    {
        //log new Data received
        if (std::string::npos != m_rxBuffer.toStdString()
                .find(spi_commands[STM_CONNECTED_ACK]))
        {
            m_commEstablished = true;
            rxDataProcessed = true;
            emit updateLogWindow("Stm connected");
        }
    }
    return rxDataProcessed;
}



