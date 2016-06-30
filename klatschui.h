#ifndef KLATSCHUI_H
#define KLATSCHUI_H

#include <QMainWindow>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QtDebug>
#include <QObject>

#include "serialportlistener.h"

#define STD_AUFNAHME 40
#define STD_SCHWELLE 6
#define STD_STILLE 2
#define STD_TOLERANZ 500

namespace Ui {
    class klatschui;
}

class klatschui : public QMainWindow
{
    Q_OBJECT

public:
    explicit klatschui(QWidget *parent = 0);
    ~klatschui();

private slots:
    void fuckingSend(QByteArray data);

    //void handleErrors(QSerialPort::SerialPortError);

    void readArduinoData(QString text);

    void closeArduinoPort();

    void writeArduinoData(QString str);

    void on_configIntAufnahme_valueChanged();

    void on_configIntSchwelle_valueChanged();

    void on_configIntStille_valueChanged();

    void on_configIntToleranz_valueChanged();

    void on_configIntAllDef_clicked();

    void on_configIntAufnahmeDef_clicked();

    void on_configIntSchwelleDef_clicked();

    void on_configIntStilleDef_clicked();

    void on_configIntToleranzDef_clicked();

    void on_configSaveBtn_clicked();

    void on_configPinAktualisierenBtn_clicked();

    void on_configPinVerbinden_clicked();

    void on_configPinDisconnect_clicked();

    void on_configPinSound_currentIndexChanged(const QString &arg1);

    void on_configPinSound_currentTextChanged(const QString &arg1);

    void on_configPinPieper_currentTextChanged(const QString &arg1);

    void sendCurrentValues();

    void changeAufnahme(int value);

    void changeSchwelle(int value);

    void changeToleranz (int value);

    void changeStille(int value);

    void changePieper(int value);

    void changeSound(QString value);

    void on_meldung_clicked();

    void on_neuesGeraetBtn_clicked();

    void WhenHandledConnected(int, QString);

    void PortListeAktualisieren(QList<QSerialPortInfo>, int, QString);

    void on_musSave_0_clicked();

    void clearAllMuster();

    int addMuster(QString titel, QString R1, QString R2, QString R3, QString R4, QString R5, QString R6, QString Geraete, QString Action);

    void DisplayPopup(QString);

    void on_musSave_1_clicked();

    void on_musSave_2_clicked();

    void on_musSave_3_clicked();

    void on_musSave_4_clicked();

    void on_musSave_5_clicked();

    void on_musSave_6_clicked();

    void on_musSave_7_clicked();

    void on_musSave_8_clicked();

    void on_musSave_9_clicked();

    void on_musSave_10_clicked();

    void on_musSave_clicked();

    void on_musSave_11_clicked();

private:
    Ui::klatschui *ui;
    QSerialPort *SerialPort;
    SerialPortListener *SPL;

signals:
    void writeToArduino(QString);
    void AvailablePorts();
    void Connect(QString);
    void Close();
};

#endif // KLATSCHUI_H
