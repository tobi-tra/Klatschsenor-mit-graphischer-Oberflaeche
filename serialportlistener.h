#ifndef SERIALPORTLISTENER_H
#define SERIALPORTLISTENER_H

#include <QThread>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QString>
#include <QtDebug>
#include <QMainWindow>

#include <QMutex>

//#include "klatschui.h"

class SerialPortListener : public QThread {
    Q_OBJECT
public:
    SerialPortListener(QSerialPort* serialPort, ulong speed);
    //SerialPortListener(ulong speed);
    ~SerialPortListener();
protected:
    virtual void run();
private slots:
    void writeToQueue(QString);
    void AvailablePorts();
    void Connect(QString);
    void Close();

    //void writeToArduino(QString str);
private:
    void decodeSerialData();

    void Stack();
    QString pop();
    void push(QString str);

    QSerialPort * serialPort;
    ulong speed;

    QString buffer[200]; // Stack
    int count_elem;

    QString data;
    QByteArray bytes;

    bool waitingForAnswer;
signals:
    void dataReceived(QString);
    void sendBackAvailablePorts(QList<QSerialPortInfo>, int, QString);
    void backToConnect(int, QString);
    void sendDataToGuiToArduino(QByteArray);
};

#endif // SERIALPORTLISTENER_H
