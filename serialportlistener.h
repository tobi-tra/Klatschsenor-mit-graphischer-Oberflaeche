#ifndef SERIALPORTLISTENER_H
#define SERIALPORTLISTENER_H

#include <QThread>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QString>
#include <QtDebug>
#include <QMainWindow>
#include <QTime>

#include <QMutex>

//#include "klatschui.h"

class SerialPortListener : public QThread {
    Q_OBJECT
public:
    SerialPortListener(QSerialPort* serialPort, ulong speed);
    ~SerialPortListener();
protected:
    virtual void run();
private slots:
    void writeToQueue(QString);
    void AvailablePorts();
    void Connect(QString);
    void Close();
private:
    void decodeSerialData();
    void push(QString str);
    void clearStack();
    QString pop();
    QSerialPort * serialPort;
    ulong speed;
    QString queue[200]; // Stack
    int count_elem;
    QString data;
    QByteArray bytes;
    bool waitingForAnswer;
    QTime lastSend;
signals:
    void dataReceived(QString);
    void sendBackAvailablePorts(QList<QSerialPortInfo>, int, QString);
    void backToConnect(int, QString);
    void sendDataToGuiToArduino(QByteArray);
};

#endif // SERIALPORTLISTENER_H
