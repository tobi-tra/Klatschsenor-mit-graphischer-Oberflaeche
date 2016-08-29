#ifndef SERIALPORTLISTENER_H
#define SERIALPORTLISTENER_H

#include <QThread>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QString>
#include <QtDebug>
#include <QMainWindow>
#include <QMutex>

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
    void clearStack();
    void fixProcessed();
private:
    void decodeSerialData();
    void push(QString str);
    QString pop();
    QSerialPort * serialPort;
    ulong speed;
    QString queue[400]; // Stack
    int count_elem;
    QString data;
    QByteArray bytes;
    bool waitingForAnswer;
signals:
    void dataReceived(QString) /** Sendet empfangene Daten an GUI. */;
    void sendBackAvailablePorts(QList<QSerialPortInfo>, int, QString) /** Sendet Portliste an GUI. */;
    void backToConnect(int, QString) /** Sendet Verbingsversuchsstatus an die GUI. */;
    void sendDataToGuiToArduino(QByteArray) /** Sendet Daten an GUI, dort wird sekundärer Thread erstellt und sendet Daetn an Arduino. */;
    void numberInStack(int) /** Gibt die Anzahl an ausstehnden Befehlen zurück an die GUI. */;
};

#endif // SERIALPORTLISTENER_H
