#include "klatschui.h"
#include "ui_klatschui.h"
#include "serialportlistener.h"

QMutex mutex;

SerialPortListener::SerialPortListener(QSerialPort* serialPort, ulong speed) {
//SerialPortListener::SerialPortListener(ulong speed) {


    //this->serialPort = new QSerialPort(this); ////
    //this->serialPort = new QSerialPort(this); ////


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
        //SPL->start();
        qDebug() << "success connection";

        emit backToConnect(1, serialPort->portName());
        /*ui->configPinStatus1->setText("<table><tr><td><span style='color:green;'>◉  </span></td><td>Status: Verbunden mit <b>" + SerialPort->portName() + "</b></td></tr><tr><td>&nbsp;</td><td>&nbsp;</td></tr></table>");
        ui->configPinDisconnect->setDisabled(0);
        ui->configPinVerbinden->setDisabled(1);*/

        //sendCurrentValues();
    } else{
        qDebug() << "fail connection\n";
        emit backToConnect(0, serialPort->portName());
        //ui->configPinStatus1->setText("<table><tr><td><span style='color:red;'>◉  </span></td><td>Status: Nicht Verbunden</td></tr><tr><td>&nbsp;</td><td>Verbindungsversuch mit <b>"+ ui->configPinArduino->currentText() + "</b> fehlgeschlagen.</td></tr></table>");
    }
}

void SerialPortListener::Close() {
    qDebug() << "Connection closed.";
    serialPort->close();
}

void SerialPortListener::AvailablePorts() {
    QList<QSerialPortInfo> portInfoList = QSerialPortInfo::availablePorts();
    emit sendBackAvailablePorts(portInfoList, serialPort->isOpen(), serialPort->portName());
}

void SerialPortListener::run() {
    qDebug() << "in run";
    /*
    if(serialPort == nullptr || !serialPort->isOpen()) {
        return;
    }
    */

    if(serialPort == nullptr || !serialPort->isOpen()) {
        qDebug() << "return\n";
        return;
    }
    qDebug() << "return passed\n";

    serialPort->setDataTerminalReady(true);
    //while(isRunning() && serialPort->isOpen()) {
    while(isRunning() && serialPort->isOpen()) {
        //qDebug() << "in loop\n";

        if (!waitingForAnswer) {
            data = pop();
            if (data != "-1" ) {
                bytes = data.toUtf8();
                qDebug() << "data in bytes: " << data;
                emit sendDataToGuiToArduino(bytes);
                waitingForAnswer = true;
                //serialPort->write(bytes);
            }
        }
        decodeSerialData();
        msleep(speed);
        data = "";
        //qDebug() << "in loop\n";
    }
    qDebug() << "end of while\n";
}

void SerialPortListener::decodeSerialData() {
    if(!serialPort->isDataTerminalReady()) {
        return;
    }
    QByteArray bytes = serialPort->readAll();
    QString text = QString::fromUtf8(bytes).trimmed();
    if(text.isEmpty()) {
        return;
    }
    qDebug() << "text is" << text << "\n";
    if (text.contains("processed")) {
        waitingForAnswer = false;
        qDebug() << "processed\n";
    }
    emit dataReceived(text);
}

void SerialPortListener::writeToQueue(QString text) {
    qDebug() << "text in to queue: " << text;
    push(text);
}

QString SerialPortListener::pop() {			// Funktion pop
    mutex.lock();
    if (count_elem-1 < 0) {	// Underflow verhindern
        //qDebug() << "Underflow\n";
        mutex.unlock(); // muss das dahin?
        return "-1";			// Fehlermeldung ausgeben; -1 führt nicht zu Fehlern, da nur positive Werte gespeichert werden
    }
    QString strbuffer = buffer[0];
    for (int i = 0; i < count_elem; i++) {
        buffer[i] = buffer[i+1];
    }
    --count_elem;				// Position des Kopfes um einen verringern
    mutex.unlock();
    return (strbuffer);	// Obersten Wert des Stacks zurückgeben

}

void SerialPortListener::push(QString i) {	// Funktion push
    mutex.lock();
    qDebug() << "got in Stack: " << i;
    if (count_elem >= 200) { // Overflow verhindern
        qDebug() << "Overflow\n";
        mutex.unlock(); // muss das dahin?
        return;
    }
    buffer[count_elem] = i;	// Integer speichern
    ++count_elem;				// Position des Kopfes um einen erhöhen
    mutex.unlock();
    for(int i = 0; i < count_elem; i++) {
        qDebug() << buffer[i] << "\n";
    }
}
