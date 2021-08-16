#ifndef SPIHANDLER_H
#define SPIHANDLER_H
#include <linux/spi/spidev.h>
#include <QByteArray>
#include <QThread>
#include <QQuickItem>

//this should run in a separate thread
class SpiHandler : public QThread
{
    Q_OBJECT
public:
    struct SpiConfig
    {
        std::string deviceName;
        uint32_t speed;
        uint16_t delay;
        uint8_t bufferSize;
        uint8_t mode;
        uint8_t bits;

        bool operator ==(const SpiConfig &other)
        {
            return  (this->deviceName == other.deviceName) &&
                    (this->speed == other.speed) &&
                    (this->delay == other.delay) &&
                    (this->mode == other.mode) &&
                    (this->bits == other.bits);
        }
    };

    SpiHandler();
    ~SpiHandler();    

    bool setConfig(const SpiConfig &spiConfig);
    SpiConfig getSpiConfig() const;

    bool initDataTransfer(int transferOptions = 0);
    bool deinitDataTransfer();
    void setLogWindowRef(QQuickItem* logWindow);
protected:
    virtual void run() override;
public slots:
    void stopDataTransfer();
    void onUiStartComm();
    void onUiRequestSensorData();
signals:
    void rxDataReceived(const QByteArray &rxData);
    void updateLogWindow(QString text);
private:
    bool processRxData();
    inline bool transferData();
    bool newDataTransfer(const QByteArray &txData);

    QByteArray m_rxBuffer;
    QByteArray m_txBuffer;
    SpiConfig m_spiConfig;    
    spi_ioc_transfer m_spiTransfer;
    QQuickItem* m_logWindow{nullptr};
    int m_fileHandle;
    uint8_t m_oldRxMsgId{0U};
    uint8_t m_txMsgId{0U};
    bool m_initialized{false};
    bool m_commEstablished{false};

};

#endif // SPIHANDLER_H
