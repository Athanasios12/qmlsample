#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQmlContext>
#include "SpiHandler.h"

int main(int argc, char *argv[])
{
#if defined(Q_OS_WIN)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    SpiHandler spiHandler;
    const uint8_t SPI_BUFF_SIZE = 10U;

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }
    auto mainWindow = engine.rootObjects().at(0);
    QQuickItem *logWindow = nullptr;
    if (mainWindow)
    {
        logWindow = mainWindow->findChild<QQuickItem*>("commLogWindow");
        if (logWindow)
        {
            engine.rootContext()->setContextProperty("_SpiHandlerApi", &spiHandler);
            spiHandler.setLogWindowRef(logWindow);
            QQmlContext *context = engine.rootContext();
            context->setContextProperty("backend", &spiHandler);

            SpiHandler::SpiConfig spiConfig;

            spiConfig.deviceName = "/dev/spidev0.0";
            spiConfig.bufferSize = SPI_BUFF_SIZE;
            spiConfig.bits = 8U;
            spiConfig.delay = 0U;
            spiConfig.speed = 1500000; //1.5Mhz
            spiHandler.setConfig(spiConfig);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }

    return app.exec();
}
