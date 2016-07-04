#include "klatschui.h"
#include "ui_klatschui.h"
#include "serialportlistener.h"

QMutex mutex;

SerialPortListener::SerialPortListener(QSerialPort* serialPort, ulong speed) {
    this->serialPort = serialPort;
    this->speed = speed;
    this->count_elem = 0;
    this->waitingForAnswer = true;
    qDebug() << "SPL created\n";
}

SerialPortListener::~SerialPortListener() {

}

void SerialPortListener::Connect(QString checkedName) {
    serialPort->setPortName(checkedName);
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if(serialPort->open(QIODevice::ReadWrite)){
        qDebug() << "success connection";
        emit backToConnect(1, serialPort->portName());
    } else{
        qDebug() << "fail connection\n";
        emit backToConnect(0, serialPort->portName());
    }
}

void SerialPortListener::Close() {
    qDebug() << "Connection closed.";
    clearStack();
    serialPort->close();
}

void SerialPortListener::AvailablePorts() {
    QList<QSerialPortInfo> portInfoList = QSerialPortInfo::availablePorts();
    emit sendBackAvailablePorts(portInfoList, serialPort->isOpen(), serialPort->portName());
}

void SerialPortListener::run() {
    if(serialPort == nullptr || !serialPort->isOpen()) {
        return;
    }
    serialPort->setDataTerminalReady(true);
    while(isRunning() && serialPort->isOpen()) {
        if (!waitingForAnswer) {
            data = pop();
            if (data != "-1" ) {
                bytes = data.toUtf8();
                qDebug() << "data in bytes: " << bytes  ;
                emit sendDataToGuiToArduino(bytes);
                //lastSend = QTime::currentTime();
                waitingForAnswer = true;
            }
        }
        decodeSerialData();
        msleep(speed);
        data = "";
    }
}

void SerialPortListener::decodeSerialData() {
    if(!serialPort->isDataTerminalReady()) {
        return;
    }
    QByteArray bytes = serialPort->readAll();
    QString text = QString::fromUtf8(bytes).trimmed();
    //QString text = QString::fromUtf8(bytes);
    if(text.isEmpty()) {
        return;
    }
    qDebug() << "text from Arduino is" << text << "\n";
    if (text.contains("processed")) {
        waitingForAnswer = false;
        qDebug() << "processed\n";
    }
    emit dataReceived(text);
}

void SerialPortListener::writeToQueue(QString text)
{
    qDebug() << "in queue";
    push(text);
}

QString SerialPortListener::pop()
{
    mutex.lock();
    if (count_elem-1 < 0) {                     // Underflow verhindern
        mutex.unlock();
        return "-1";                            // Fehlermeldung ausgeben
    }
    QString strbuffer = queue[0];
    for (int i = 0; i < count_elem; i++) {
        queue[i] = queue[i+1];
    }
    --count_elem;				// Position des Kopfes um einen verringern
    mutex.unlock();
    return (strbuffer);                         // Obersten Wert des Stacks zurückgeben
}

void SerialPortListener::push(QString str)
{
    mutex.lock();
    qDebug() << "got in Stack";
    if (count_elem >= 200) {                    // Overflow verhindern
        qDebug() << "Overflow\n";
        mutex.unlock();
        return;
    }
    queue[count_elem] = str;                     // Integer speichern
    ++count_elem;				// Position des Kopfes um einen erhöhen
    mutex.unlock();
    for(int i = 0; i < count_elem; i++) {
        qDebug() << queue[i] << "\n";
    }
}

void SerialPortListener::clearStack()
{
  mutex.lock();
  if (count_elem-1 < 0) {                    // Underflow verhindern
      mutex.unlock();
      return;                            // Fehlermeldung ausgeben
  }
  for (int i = 0; i < count_elem; i++) {
      queue[i] = "";
  }
  count_elem = 0;
  mutex.unlock();
  qDebug() << "Stack cleared";
  return;
}
