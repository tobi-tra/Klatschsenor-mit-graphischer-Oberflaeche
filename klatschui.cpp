#include <QtDebug>
#include "klatschui.h"
#include "ui_klatschui.h"
#include "serialportlistener.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

klatschui::klatschui(QWidget *parent) : QMainWindow(parent), ui(new Ui::klatschui) {

    ui->setupUi(this);

    SerialPort = new QSerialPort(this);

    SPL = new SerialPortListener(SerialPort, 200);

    connect(SPL,  SIGNAL(dataReceived(QString)),
            this, SLOT(readArduinoData(QString)));
    connect(this, SIGNAL(writeToArduino(QString)),
            SPL, SLOT(writeToQueue(QString)));
    connect(this, SIGNAL(AvailablePorts()),
            SPL, SLOT(AvailablePorts()));
    connect(SPL,  SIGNAL(sendBackAvailablePorts(QList<QSerialPortInfo>, int, QString)),
            this, SLOT(PortListeAktualisieren(QList<QSerialPortInfo>, int, QString)));
    connect(this, SIGNAL(Connect(QString)),
            SPL, SLOT(Connect(QString)));
    connect(SPL,  SIGNAL(backToConnect(int, QString)),
            this, SLOT(WhenHandledConnected(int, QString)));
    connect(this, SIGNAL(Close()),
            SPL, SLOT(Close()));
    connect(SPL,  SIGNAL(sendDataToGuiToArduino(QByteArray)),
            this, SLOT(fuckingSend(QByteArray)));

    //connect(this, SIGNAL(writeToArduino(QString)), qApp, SLOT(aboutQt())); // just for debugging
    // when detect error: delete queue, stop serial comm thread

    //connect(SerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleErrors(QSerialPort::SerialPortError)));
    //connect(SerialPort, SIGNAL(readyRead()), this, SLOT(readArduinoData()));

    changeAufnahme(ui->configIntAufnahme->value());
    changeSchwelle(ui->configIntSchwelle->value());
    changeStille(ui->configIntStille->value());
    changeToleranz(ui->configIntToleranz->value());
    //changePieper(3);
    //changeSound("A3");
    /*
    ui->configIntAufnahmeWert->setText(QString::number(ui->configIntAufnahme->value()));
    ui->configIntSchwelleWert->setText(QString::number(ui->configIntSchwelle->value()));
    ui->configIntStilleWert->setText(QString::number(ui->configIntStille->value()));
    ui->configIntToleranzWert->setText(QString::number(ui->configIntToleranz->value()));
*/
    on_configPinAktualisierenBtn_clicked();
    ui->configPinDisconnect->setDisabled(1);
}

klatschui::~klatschui()
{
    delete ui;
}

void klatschui::fuckingSend(QByteArray data) {
    SerialPort->write(data);
}

void klatschui::on_configIntAufnahme_valueChanged()
{
    changeAufnahme(ui->configIntAufnahme->value());
}

void klatschui::on_configIntSchwelle_valueChanged()
{
    changeSchwelle(ui->configIntSchwelle->value());
    /*ui->configIntSchwelleWert->setText(QString::number(ui->configIntSchwelle->value()));
    QString send = "setIntSchwelle";
    send += QString::number(ui->configIntSchwelle->value());
    writeArduinoData(send);*/
}

void klatschui::on_configIntStille_valueChanged()
{
    changeStille(ui->configIntStille->value());
    /*ui->configIntStilleWert->setText(QString::number(ui->configIntStille->value()));
    QString send = "setIntStille";
    send += QString::number(ui->configIntStille->value());
    writeArduinoData(send);*/
}

void klatschui::on_configIntToleranz_valueChanged()
{
    changeToleranz(ui->configIntToleranz->value());
    /*ui->configIntToleranzWert->setText(QString::number(ui->configIntToleranz->value()));
    QString send = "setIntToleranz";
    send += QString::number(ui->configIntToleranz->value());
    writeArduinoData(send);*/
}

void klatschui::on_configIntAllDef_clicked()
{
    changeAufnahme(STD_AUFNAHME);
    changeSchwelle(STD_SCHWELLE);
    changeStille(STD_STILLE);
    changeToleranz(STD_TOLERANZ);
    //ui->configIntAufnahme->setValue(STD_AUFNAHME);
    //ui->configIntSchwelle->setValue(STD_SCHWELLE);
    //ui->configIntStille->setValue(STD_STILLE);
    //ui->configIntToleranz->setValue(STD_TOLERANZ);
}

void klatschui::on_configIntAufnahmeDef_clicked()
{
    changeAufnahme(STD_AUFNAHME);
}

void klatschui::on_configIntSchwelleDef_clicked()
{
    changeSchwelle(STD_SCHWELLE);
}

void klatschui::on_configIntStilleDef_clicked()
{
    changeStille(STD_STILLE);
}

void klatschui::on_configIntToleranzDef_clicked()
{
    changeToleranz(STD_TOLERANZ);
}

void klatschui::on_configSaveBtn_clicked() // not in use
{
    QString filename = "C:/Data.txt";
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << "something" << endl;
    }
}

void klatschui::on_configPinAktualisierenBtn_clicked()
{
    emit AvailablePorts();
}

void klatschui::PortListeAktualisieren(QList<QSerialPortInfo> portInfoList, int isOpen, QString portName) {
    ui->configPinArduino->clear();

    for (int i = 0; i < portInfoList.size(); i++) {
        QSerialPortInfo &info = portInfoList[i];

        ui->configPinArduino->addItem(info.portName(),info.portName());

        if (isOpen) {
            if (info.portName() == portName) {
                ui->configPinArduino->setCurrentIndex(i);
            }
        }
    }
}

void klatschui::on_configPinVerbinden_clicked()
{
    emit Connect(ui->configPinArduino->currentText());
}

void klatschui::WhenHandledConnected(int connectedTrueFalse, QString portName) {
    if(connectedTrueFalse){
        SPL->start();
        ui->configPinStatus1->setText("<table><tr><td><span style='color:green;'>◉  </span></td><td>Status: Verbunden mit <b>" + portName + "</b></td></tr><tr><td>&nbsp;</td><td>&nbsp;</td></tr></table>");
        ui->configPinDisconnect->setDisabled(0);
        ui->configPinVerbinden->setDisabled(1);

        //sendCurrentValues();
    } else{
        ui->configPinStatus1->setText("<table><tr><td><span style='color:red;'>◉  </span></td><td>Status: Nicht Verbunden</td></tr><tr><td>&nbsp;</td><td>Verbindungsversuch mit <b>"+ ui->configPinArduino->currentText() + "</b> fehlgeschlagen.</td></tr></table>");
    }
}

void klatschui::on_configPinDisconnect_clicked()
{
    closeArduinoPort();
}

void klatschui::on_configPinSound_currentIndexChanged(const QString &arg1)
{

}

void klatschui::readArduinoData(QString text)
{
    ui->terminal->append(text);
}

void klatschui::writeArduinoData(QString str)
{
    qDebug() << "write arduino data and wait: " << str;
    QByteArray data = str.toUtf8();
    emit writeToArduino(data);

    QString str2 = "Sent via wait " + str + " to the Arduino";
    ui->terminal->append(str2);
}

/*void klatschui::handleErrors(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        ui->terminal->append(SerialPort->errorString());
        closeArduinoPort();
    }
}*/

void klatschui::closeArduinoPort()
{
    QString last_port = ui->configPinArduino->currentText();
    emit Close();
    ui->configPinStatus1->setText("<table><tr><td><span style='color:yellow;'>◉  </span></td><td>Status: Nicht Verbunden</td></tr><tr><td>&nbsp;</td><td>Verbindung mit "+ last_port + " geschlossen.</td></tr></table>");
    ui->configPinDisconnect->setDisabled(1);
    ui->configPinVerbinden->setDisabled(0);
    QString str2 = "Disconnected";
    ui->terminal->append(str2);
}

void klatschui::on_configPinSound_currentTextChanged(const QString &arg1)
{
    changeSound(arg1);
}

void klatschui::on_configPinPieper_currentTextChanged(const QString &arg1)
{

    changePieper(arg1.toInt());
}

void klatschui::sendCurrentValues() {
    QString send = "";
    if (ui->configPinSound->currentIndex() != 0) {
        changeSound(ui->configPinSound->currentText());
    }
    if (ui->configPinPieper->currentIndex() != 0) {
        changePieper(ui->configPinPieper->currentText().toInt());
    }

    changeAufnahme(ui->configIntAufnahme->value());
    changeSchwelle(ui->configIntSchwelle->value());
    changeStille(ui->configIntStille->value());
    changeToleranz(ui->configIntToleranz->value());
}

void klatschui::changeAufnahme(int value) {
    ui->configIntAufnahmeWert->setText(QString::number(value));
    ui->configIntAufnahme->setValue(value);
    QString send = "setIntAufnahme";
    send += QString::number(value);
    writeArduinoData(send);
}

void klatschui::changeSchwelle(int value) {
    ui->configIntSchwelleWert->setText(QString::number(value));
    ui->configIntSchwelle->setValue(value);
    QString send = "setIntSchwelle";
    send += QString::number(value);
    writeArduinoData(send);
}

void klatschui::changeStille(int value) {
    ui->configIntStilleWert->setText(QString::number(value));
    ui->configIntStille->setValue(value);
    QString send = "setIntStille";
    send += QString::number(value);
    writeArduinoData(send);
}

void klatschui::changeToleranz(int value) {
    ui->configIntToleranzWert->setText(QString::number(value));
    ui->configIntToleranz->setValue(value);
    QString send = "setIntToleranz";
    send += QString::number(value);
    writeArduinoData(send);
}

void klatschui::changePieper(int value) {
    QString send = "setPinModePieper";
    send += QString::number(value);
    writeArduinoData(send);
    ui->configPinPieper->setCurrentText(QString::number(value));
}

void klatschui::changeSound(QString value) {
    QString send = "setPinModeSound";
    send += value;
    writeArduinoData(send);
    ui->configPinSound->setCurrentText(value);
}

void klatschui::on_meldung_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("The document has been modified.");
    msgBox.exec();
}

void klatschui::on_neuesGeraetBtn_clicked()
{

    bool continueProcess = 1;
    QString send = "setGeraete";

    if (ui->gerPin_0->currentText() != "-" && ui->gerStart_0->currentText() != "-" && continueProcess) {
        send += ui->gerPin_0->currentText();
        send += ',';
        if (ui->gerStart_0->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_0->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_1->currentText() != "-" && ui->gerStart_1->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_1->currentText();
        send += ',';
        if (ui->gerStart_1->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_1->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_2->currentText() != "-" && ui->gerStart_2->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_2->currentText();
        send += ',';
        if (ui->gerStart_2->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_2->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_3->currentText() != "-" && ui->gerStart_3->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_3->currentText();
        send += ',';
        if (ui->gerStart_3->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_3->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_4->currentText() != "-" && ui->gerStart_4->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_4->currentText();
        send += ',';
        if (ui->gerStart_4->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_4->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_5->currentText() != "-" && ui->gerStart_5->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_5->currentText();
        send += ',';
        if (ui->gerStart_5->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_5->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_6->currentText() != "-" && ui->gerStart_6->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_6->currentText();
        send += ',';
        if (ui->gerStart_6->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_6->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_7->currentText() != "-" && ui->gerStart_7->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_7->currentText();
        send += ',';
        if (ui->gerStart_7->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_7->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_8->currentText() != "-" && ui->gerStart_8->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_8->currentText();
        send += ',';
        if (ui->gerStart_8->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_8->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_9->currentText() != "-" && ui->gerStart_9->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_9->currentText();
        send += ',';
        if (ui->gerStart_9->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_9->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    if (ui->gerPin_10->currentText() != "-" && ui->gerStart_10->currentText() != "-" && continueProcess) {
        send += '~';
        send += ui->gerPin_10->currentText();
        send += ',';
        if (ui->gerStart_10->currentText() == "an") {
            send += "1";
        } else if (ui->gerStart_10->currentText() == "aus") {
            send += "0";
        }
    } else {
        continueProcess = 0;
    }
    writeArduinoData(send);
}

int klatschui::addMuster(QString titel, QString R1, QString R2, QString R3, QString R4, QString R5, QString R6, QString Geraete, QString Action) {
    int stopRythm = 0;
    QString send = "setMuster";

    if (titel == "") {
        qDebug() << "Titel darf nicht leer sein";
        DisplayPopup("Titel darf nicht leer sein");
        return 1;
    }
    send += titel;
    send += "~";

    if (R1 == "-") {
        qDebug() << "R1 darf nicht leer sein";
        DisplayPopup("R1 darf nicht leer sein");
        return 1;
    } else {
        send += R1;
        if (R2.contains("-")) {
            stopRythm = 1;
        } else {
            send += ",";
            send += R2;
            stopRythm = 1;
            if (R3.contains("-") && stopRythm) {
                stopRythm = 1;
            } else {
                send += ",";
                send += R3;
                if (R4.contains("-") && stopRythm) {
                    stopRythm = 1;
                } else {
                    send += ",";
                    send += R4;
                    if (R5.contains("-") && stopRythm) {
                        stopRythm = 1;
                    } else {
                        send += ",";
                        send += R5;
                        if (R6.contains("-") && stopRythm) {
                            stopRythm = 1;
                        } else {
                            send += ",";
                            send += R6;
                        }
                    }
                }
            }
        }
    }

    send += "~";

    if (Geraete == "") {
        qDebug() << "Geraete darf nicht leer sein.";
        DisplayPopup("Geraete darf nicht leer sein.");
        return 1;
    }
    send += Geraete;
    send += "~";
    if (Action == "-") {
        qDebug() << "Action darf nicht - sein";
        DisplayPopup("Action darf nicht - sein");
        return 1;
    } else if (Action == "an") {
        send += "1";
    } else if (Action == "aus") {
        send += "0";
    } else if (Action == "toggle") {
        send += "2";
    }

    writeArduinoData(send);
    return 0;
}

void klatschui::clearAllMuster() {
    QString send = "clearAllMuster";
    writeArduinoData(send);
    DisplayPopup("Befehl wurde gesendet.");

    ui->musLine_0->setDisabled(0);
    ui->mus0m1->setDisabled(0);
    ui->mus0m2->setDisabled(0);
    ui->mus0m3->setDisabled(0);
    ui->mus0m4->setDisabled(0);
    ui->mus0m5->setDisabled(0);
    ui->mus0m6->setDisabled(0);
    ui->musGer_0->setDisabled(0);
    ui->musAction_0->setDisabled(0);
    ui->musSave_0->setDisabled(0);
    ui->musLine_1->setDisabled(0);
    ui->mus1m1->setDisabled(0);
    ui->mus1m2->setDisabled(0);
    ui->mus1m3->setDisabled(0);
    ui->mus1m4->setDisabled(0);
    ui->mus1m5->setDisabled(0);
    ui->mus1m6->setDisabled(0);
    ui->musGer_1->setDisabled(0);
    ui->musAction_1->setDisabled(0);
    ui->musSave_1->setDisabled(0);
    ui->musLine_2->setDisabled(0);
    ui->mus2m1->setDisabled(0);
    ui->mus2m2->setDisabled(0);
    ui->mus2m3->setDisabled(0);
    ui->mus2m4->setDisabled(0);
    ui->mus2m5->setDisabled(0);
    ui->mus2m6->setDisabled(0);
    ui->musGer_2->setDisabled(0);
    ui->musAction_2->setDisabled(0);
    ui->musSave_2->setDisabled(0);
    ui->musLine_3->setDisabled(0);
    ui->mus3m1->setDisabled(0);
    ui->mus3m2->setDisabled(0);
    ui->mus3m3->setDisabled(0);
    ui->mus3m4->setDisabled(0);
    ui->mus3m5->setDisabled(0);
    ui->mus3m6->setDisabled(0);
    ui->musGer_3->setDisabled(0);
    ui->musAction_3->setDisabled(0);
    ui->musSave_3->setDisabled(0);
    ui->musLine_4->setDisabled(0);
    ui->mus4m1->setDisabled(0);
    ui->mus4m2->setDisabled(0);
    ui->mus4m3->setDisabled(0);
    ui->mus4m4->setDisabled(0);
    ui->mus4m5->setDisabled(0);
    ui->mus4m6->setDisabled(0);
    ui->musGer_4->setDisabled(0);
    ui->musAction_4->setDisabled(0);
    ui->musSave_4->setDisabled(0);
    ui->musLine_5->setDisabled(0);
    ui->mus5m1->setDisabled(0);
    ui->mus5m2->setDisabled(0);
    ui->mus5m3->setDisabled(0);
    ui->mus5m4->setDisabled(0);
    ui->mus5m5->setDisabled(0);
    ui->mus5m6->setDisabled(0);
    ui->musGer_5->setDisabled(0);
    ui->musAction_5->setDisabled(0);
    ui->musSave_5->setDisabled(0);
    ui->musLine_6->setDisabled(0);
    ui->mus6m1->setDisabled(0);
    ui->mus6m2->setDisabled(0);
    ui->mus6m3->setDisabled(0);
    ui->mus6m4->setDisabled(0);
    ui->mus6m5->setDisabled(0);
    ui->mus6m6->setDisabled(0);
    ui->musGer_6->setDisabled(0);
    ui->musAction_6->setDisabled(0);
    ui->musSave_6->setDisabled(0);
    ui->musLine_7->setDisabled(0);
    ui->mus7m1->setDisabled(0);
    ui->mus7m2->setDisabled(0);
    ui->mus7m3->setDisabled(0);
    ui->mus7m4->setDisabled(0);
    ui->mus7m5->setDisabled(0);
    ui->mus7m6->setDisabled(0);
    ui->musGer_7->setDisabled(0);
    ui->musAction_7->setDisabled(0);
    ui->musSave_7->setDisabled(0);
    ui->musLine_8->setDisabled(0);
    ui->mus8m1->setDisabled(0);
    ui->mus8m2->setDisabled(0);
    ui->mus8m3->setDisabled(0);
    ui->mus8m4->setDisabled(0);
    ui->mus8m5->setDisabled(0);
    ui->mus8m6->setDisabled(0);
    ui->musGer_8->setDisabled(0);
    ui->musAction_8->setDisabled(0);
    ui->musSave_8->setDisabled(0);
    ui->musLine_9->setDisabled(0);
    ui->mus9m1->setDisabled(0);
    ui->mus9m2->setDisabled(0);
    ui->mus9m3->setDisabled(0);
    ui->mus9m4->setDisabled(0);
    ui->mus9m5->setDisabled(0);
    ui->mus9m6->setDisabled(0);
    ui->musGer_9->setDisabled(0);
    ui->musAction_9->setDisabled(0);
    ui->musSave_9->setDisabled(0);
    ui->musLine_10->setDisabled(0);
    ui->mus10m1->setDisabled(0);
    ui->mus10m2->setDisabled(0);
    ui->mus10m3->setDisabled(0);
    ui->mus10m4->setDisabled(0);
    ui->mus10m5->setDisabled(0);
    ui->mus10m6->setDisabled(0);
    ui->musGer_10->setDisabled(0);
    ui->musAction_10->setDisabled(0);
    ui->musSave_10->setDisabled(0);
}

void klatschui::DisplayPopup(QString text) {
    QMessageBox msgBox;
    msgBox.setText(text);
    msgBox.exec();
}

void klatschui::on_musSave_0_clicked()
{
    if (!addMuster(ui->musLine_0->text(),
              ui->mus0m1->currentText(),
              ui->mus0m2->currentText(),
              ui->mus0m3->currentText(),
              ui->mus0m4->currentText(),
              ui->mus0m5->currentText(),
              ui->mus0m6->currentText(),
              ui->musGer_0->text(),
              ui->musAction_0->currentText() ))
    {
        ui->musLine_0->setDisabled(1);
        ui->mus0m1->setDisabled(1);
        ui->mus0m2->setDisabled(1);
        ui->mus0m3->setDisabled(1);
        ui->mus0m4->setDisabled(1);
        ui->mus0m5->setDisabled(1);
        ui->mus0m6->setDisabled(1);
        ui->musGer_0->setDisabled(1);
        ui->musAction_0->setDisabled(1);
        ui->musSave_0->setDisabled(1);
    }
}

void klatschui::on_musSave_1_clicked()
{
    if (!addMuster(ui->musLine_1->text(),
              ui->mus1m1->currentText(),
              ui->mus1m2->currentText(),
              ui->mus1m3->currentText(),
              ui->mus1m4->currentText(),
              ui->mus1m5->currentText(),
              ui->mus1m6->currentText(),
              ui->musGer_1->text(),
              ui->musAction_1->currentText() ))
    {
        ui->musLine_1->setDisabled(1);
        ui->mus1m1->setDisabled(1);
        ui->mus1m2->setDisabled(1);
        ui->mus1m3->setDisabled(1);
        ui->mus1m4->setDisabled(1);
        ui->mus1m5->setDisabled(1);
        ui->mus1m6->setDisabled(1);
        ui->musGer_1->setDisabled(1);
        ui->musAction_1->setDisabled(1);
        ui->musSave_1->setDisabled(1);
    }
}

void klatschui::on_musSave_2_clicked()
{
    if (!addMuster(ui->musLine_2->text(),
              ui->mus2m1->currentText(),
              ui->mus2m2->currentText(),
              ui->mus2m3->currentText(),
              ui->mus2m4->currentText(),
              ui->mus2m5->currentText(),
              ui->mus2m6->currentText(),
              ui->musGer_2->text(),
              ui->musAction_2->currentText() ))
    {
        ui->musLine_2->setDisabled(1);
        ui->mus2m1->setDisabled(1);
        ui->mus2m2->setDisabled(1);
        ui->mus2m3->setDisabled(1);
        ui->mus2m4->setDisabled(1);
        ui->mus2m5->setDisabled(1);
        ui->mus2m6->setDisabled(1);
        ui->musGer_2->setDisabled(1);
        ui->musAction_2->setDisabled(1);
        ui->musSave_2->setDisabled(1);
    }
}

void klatschui::on_musSave_3_clicked()
{
    if (!addMuster(ui->musLine_3->text(),
              ui->mus3m1->currentText(),
              ui->mus3m2->currentText(),
              ui->mus3m3->currentText(),
              ui->mus3m4->currentText(),
              ui->mus3m5->currentText(),
              ui->mus3m6->currentText(),
              ui->musGer_3->text(),
              ui->musAction_3->currentText() ))
    {
        ui->musLine_3->setDisabled(1);
        ui->mus3m1->setDisabled(1);
        ui->mus3m2->setDisabled(1);
        ui->mus3m3->setDisabled(1);
        ui->mus3m4->setDisabled(1);
        ui->mus3m5->setDisabled(1);
        ui->mus3m6->setDisabled(1);
        ui->musGer_3->setDisabled(1);
        ui->musAction_3->setDisabled(1);
        ui->musSave_3->setDisabled(1);
    }
}

void klatschui::on_musSave_4_clicked()
{
    if (!addMuster(ui->musLine_4->text(),
              ui->mus4m1->currentText(),
              ui->mus4m2->currentText(),
              ui->mus4m3->currentText(),
              ui->mus4m4->currentText(),
              ui->mus4m5->currentText(),
              ui->mus4m6->currentText(),
              ui->musGer_4->text(),
              ui->musAction_4->currentText() ))
    {
        ui->musLine_4->setDisabled(1);
        ui->mus4m1->setDisabled(1);
        ui->mus4m2->setDisabled(1);
        ui->mus4m3->setDisabled(1);
        ui->mus4m4->setDisabled(1);
        ui->mus4m5->setDisabled(1);
        ui->mus4m6->setDisabled(1);
        ui->musGer_4->setDisabled(1);
        ui->musAction_4->setDisabled(1);
        ui->musSave_4->setDisabled(1);
    }
}

void klatschui::on_musSave_5_clicked()
{
    if (!addMuster(ui->musLine_5->text(),
              ui->mus5m1->currentText(),
              ui->mus5m2->currentText(),
              ui->mus5m3->currentText(),
              ui->mus5m4->currentText(),
              ui->mus5m5->currentText(),
              ui->mus5m6->currentText(),
              ui->musGer_5->text(),
              ui->musAction_5->currentText() ))
    {
        ui->musLine_5->setDisabled(1);
        ui->mus5m1->setDisabled(1);
        ui->mus5m2->setDisabled(1);
        ui->mus5m3->setDisabled(1);
        ui->mus5m4->setDisabled(1);
        ui->mus5m5->setDisabled(1);
        ui->mus5m6->setDisabled(1);
        ui->musGer_5->setDisabled(1);
        ui->musAction_5->setDisabled(1);
        ui->musSave_5->setDisabled(1);
    }
}

void klatschui::on_musSave_6_clicked()
{
    if (!addMuster(ui->musLine_6->text(),
              ui->mus6m1->currentText(),
              ui->mus6m2->currentText(),
              ui->mus6m3->currentText(),
              ui->mus6m4->currentText(),
              ui->mus6m5->currentText(),
              ui->mus6m6->currentText(),
              ui->musGer_6->text(),
              ui->musAction_6->currentText() ))
    {
        ui->musLine_6->setDisabled(1);
        ui->mus6m1->setDisabled(1);
        ui->mus6m2->setDisabled(1);
        ui->mus6m3->setDisabled(1);
        ui->mus6m4->setDisabled(1);
        ui->mus6m5->setDisabled(1);
        ui->mus6m6->setDisabled(1);
        ui->musGer_6->setDisabled(1);
        ui->musAction_6->setDisabled(1);
        ui->musSave_6->setDisabled(1);
    }
}

void klatschui::on_musSave_7_clicked()
{
    if (!addMuster(ui->musLine_7->text(),
              ui->mus7m1->currentText(),
              ui->mus7m2->currentText(),
              ui->mus7m3->currentText(),
              ui->mus7m4->currentText(),
              ui->mus7m5->currentText(),
              ui->mus7m6->currentText(),
              ui->musGer_7->text(),
              ui->musAction_7->currentText() ))
    {
        ui->musLine_7->setDisabled(1);
        ui->mus7m1->setDisabled(1);
        ui->mus7m2->setDisabled(1);
        ui->mus7m3->setDisabled(1);
        ui->mus7m4->setDisabled(1);
        ui->mus7m5->setDisabled(1);
        ui->mus7m6->setDisabled(1);
        ui->musGer_7->setDisabled(1);
        ui->musAction_7->setDisabled(1);
        ui->musSave_7->setDisabled(1);
    }
}

void klatschui::on_musSave_8_clicked()
{
    if (!addMuster(ui->musLine_8->text(),
              ui->mus8m1->currentText(),
              ui->mus8m2->currentText(),
              ui->mus8m3->currentText(),
              ui->mus8m4->currentText(),
              ui->mus8m5->currentText(),
              ui->mus8m6->currentText(),
              ui->musGer_8->text(),
              ui->musAction_8->currentText() ))
    {
        ui->musLine_8->setDisabled(1);
        ui->mus8m1->setDisabled(1);
        ui->mus8m2->setDisabled(1);
        ui->mus8m3->setDisabled(1);
        ui->mus8m4->setDisabled(1);
        ui->mus8m5->setDisabled(1);
        ui->mus8m6->setDisabled(1);
        ui->musGer_8->setDisabled(1);
        ui->musAction_8->setDisabled(1);
        ui->musSave_8->setDisabled(1);
    }
}

void klatschui::on_musSave_9_clicked()
{
    if (!addMuster(ui->musLine_9->text(),
              ui->mus9m1->currentText(),
              ui->mus9m2->currentText(),
              ui->mus9m3->currentText(),
              ui->mus9m4->currentText(),
              ui->mus9m5->currentText(),
              ui->mus9m6->currentText(),
              ui->musGer_9->text(),
              ui->musAction_9->currentText() ))
    {
        ui->musLine_9->setDisabled(1);
        ui->mus9m1->setDisabled(1);
        ui->mus9m2->setDisabled(1);
        ui->mus9m3->setDisabled(1);
        ui->mus9m4->setDisabled(1);
        ui->mus9m5->setDisabled(1);
        ui->mus9m6->setDisabled(1);
        ui->musGer_9->setDisabled(1);
        ui->musAction_9->setDisabled(1);
        ui->musSave_9->setDisabled(1);
    }
}

void klatschui::on_musSave_10_clicked()
{
    if (!addMuster(ui->musLine_10->text(),
              ui->mus10m1->currentText(),
              ui->mus10m2->currentText(),
              ui->mus10m3->currentText(),
              ui->mus10m4->currentText(),
              ui->mus10m5->currentText(),
              ui->mus10m6->currentText(),
              ui->musGer_10->text(),
              ui->musAction_10->currentText() ))
    {
        ui->musLine_10->setDisabled(1);
        ui->mus10m1->setDisabled(1);
        ui->mus10m2->setDisabled(1);
        ui->mus10m3->setDisabled(1);
        ui->mus10m4->setDisabled(1);
        ui->mus10m5->setDisabled(1);
        ui->mus10m6->setDisabled(1);
        ui->musGer_10->setDisabled(1);
        ui->musAction_10->setDisabled(1);
        ui->musSave_10->setDisabled(1);
    }
}

void klatschui::on_musSave_clicked()
{
    int count = 0;
    if (ui->musLine_0->text() != "" && ui->mus0m1->currentText() != "-" && ui->musGer_0->text() != "" && ui->musLine_0->isEnabled()) {
        on_musSave_0_clicked();
        count++;
    }
    if (ui->musLine_1->text() != "" && ui->mus1m1->currentText() != "-" && ui->musGer_1->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_1_clicked();
        count++;
    }
    if (ui->musLine_2->text() != "" && ui->mus2m1->currentText() != "-" && ui->musGer_2->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_2_clicked();
        count++;
    }
    if (ui->musLine_3->text() != "" && ui->mus3m1->currentText() != "-" && ui->musGer_3->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_3_clicked();
        count++;
    }
    if (ui->musLine_4->text() != "" && ui->mus4m1->currentText() != "-" && ui->musGer_4->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_4_clicked();
        count++;
    }
    if (ui->musLine_5->text() != "" && ui->mus5m1->currentText() != "-" && ui->musGer_5->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_5_clicked();
        count++;
    }
    if (ui->musLine_6->text() != "" && ui->mus6m1->currentText() != "-" && ui->musGer_6->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_6_clicked();
        count++;
    }
    if (ui->musLine_7->text() != "" && ui->mus7m1->currentText() != "-" && ui->musGer_7->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_7_clicked();
        count++;
    }
    if (ui->musLine_8->text() != "" && ui->mus8m1->currentText() != "-" && ui->musGer_8->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_8_clicked();
        count++;
    }
    if (ui->musLine_9->text() != "" && ui->mus9m1->currentText() != "-" && ui->musGer_9->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_9_clicked();
        count++;
    }
    if (ui->musLine_10->text() != "" && ui->mus10m1->currentText() != "-" && ui->musGer_10->text() != ""&& ui->musLine_0->isEnabled()) {
        on_musSave_10_clicked();
        count++;
    }
    if (count < 1) {
        DisplayPopup("Es wurde nichts gespeichert. Für detailierte Fehlermeldungen Zeilen einzeln speichern");
    } else if (count == 1) {
        DisplayPopup("Es wurde 1 Änderung durchgeführt.");
    } else {
        DisplayPopup("Es wurden " + QString::number(count) + " Speicherungen durchgeführt.");
    }

}

void klatschui::on_musSave_11_clicked()
{
    clearAllMuster();
}
