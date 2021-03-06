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

    void on_configIntAufnahme_valueChanged();
    void on_configIntSchwelle_valueChanged();
    void on_configIntStille_valueChanged();
    void on_configIntToleranz_valueChanged();
    void on_configIntAllDef_clicked();
    void on_configIntAufnahmeDef_clicked();
    void on_configIntSchwelleDef_clicked();
    void on_configIntStilleDef_clicked();
    void on_configIntToleranzDef_clicked();
    void on_configPinAktualisierenBtn_clicked();
    void on_configPinVerbinden_clicked();
    void on_configPinDisconnect_clicked();
    void on_configPinSound_currentTextChanged(const QString &arg1);
    void on_configPinPieper_currentTextChanged(const QString &arg1);
    void on_GerSave_clicked();
    void on_musSave_0_clicked();
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
    void on_musClear_clicked();
    void on_mus0m1_currentTextChanged(const QString &arg1);
    void on_mus0m2_currentTextChanged(const QString &arg1);
    void on_mus0m3_currentTextChanged(const QString &arg1);
    void on_mus0m4_currentTextChanged(const QString &arg1);
    void on_mus1m1_currentTextChanged(const QString &arg1);
    void on_mus1m2_currentTextChanged(const QString &arg1);
    void on_mus1m3_currentTextChanged(const QString &arg1);
    void on_mus1m4_currentTextChanged(const QString &arg1);
    void on_mus2m1_currentTextChanged(const QString &arg1);
    void on_mus2m2_currentTextChanged(const QString &arg1);
    void on_mus2m3_currentTextChanged(const QString &arg1);
    void on_mus2m4_currentTextChanged(const QString &arg1);
    void on_mus3m1_currentTextChanged(const QString &arg1);
    void on_mus3m2_currentTextChanged(const QString &arg1);
    void on_mus3m3_currentTextChanged(const QString &arg1);
    void on_mus3m4_currentTextChanged(const QString &arg1);
    void on_mus4m1_currentTextChanged(const QString &arg1);
    void on_mus4m2_currentTextChanged(const QString &arg1);
    void on_mus4m3_currentTextChanged(const QString &arg1);
    void on_mus4m4_currentTextChanged(const QString &arg1);
    void on_mus5m1_currentTextChanged(const QString &arg1);
    void on_mus5m2_currentTextChanged(const QString &arg1);
    void on_mus5m3_currentTextChanged(const QString &arg1);
    void on_mus5m4_currentTextChanged(const QString &arg1);
    void on_mus6m1_currentTextChanged(const QString &arg1);
    void on_mus6m2_currentTextChanged(const QString &arg1);
    void on_mus6m3_currentTextChanged(const QString &arg1);
    void on_mus6m4_currentTextChanged(const QString &arg1);
    void on_mus7m1_currentTextChanged(const QString &arg1);
    void on_mus7m2_currentTextChanged(const QString &arg1);
    void on_mus7m3_currentTextChanged(const QString &arg1);
    void on_mus7m4_currentTextChanged(const QString &arg1);
    void on_mus8m1_currentTextChanged(const QString &arg1);
    void on_mus8m2_currentTextChanged(const QString &arg1);
    void on_mus8m3_currentTextChanged(const QString &arg1);
    void on_mus8m4_currentTextChanged(const QString &arg1);
    void on_mus9m1_currentTextChanged(const QString &arg1);
    void on_mus9m2_currentTextChanged(const QString &arg1);
    void on_mus9m3_currentTextChanged(const QString &arg1);
    void on_mus9m4_currentTextChanged(const QString &arg1);
    void on_mus10m1_currentTextChanged(const QString &arg1);
    void on_mus10m2_currentTextChanged(const QString &arg1);
    void on_mus10m3_currentTextChanged(const QString &arg1);
    void on_mus10m4_currentTextChanged(const QString &arg1);
    void on_steuGerEin_0_clicked();
    void on_steuGerEin_2_clicked();
    void on_steuGerEin_4_clicked();
    void on_steuGerEin_5_clicked();
    void on_steuGerEin_6_clicked();
    void on_steuGerEin_7_clicked();
    void on_steuGerEin_8_clicked();
    void on_steuGerEin_9_clicked();
    void on_steuGerEin_10_clicked();
    void on_steuGerEin_20_clicked();
    void on_steuGerEin_21_clicked();
    void on_steuGerAus_0_clicked();
    void on_steuGerAus_2_clicked();
    void on_steuGerAus_4_clicked();
    void on_steuGerAus_5_clicked();
    void on_steuGerAus_6_clicked();
    void on_steuGerAus_7_clicked();
    void on_steuGerAus_8_clicked();
    void on_steuGerAus_9_clicked();
    void on_steuGerAus_10_clicked();
    void on_steuGerAus_20_clicked();
    void on_steuGerAus_21_clicked();
    void readArduinoData(QString text);
    void PortListeAktualisieren(QList<QSerialPortInfo>, int, QString);
    void WhenHandledConnected(int, QString);
    void Send(QByteArray data);
    void on_pushButton_pressed();
    void on_pushButton_released();
    void numberInStackToGUI(int);
    void on_pushButton_2_clicked();

private:
    Ui::klatschui *ui;
    QSerialPort *SerialPort;
    SerialPortListener *SPL;
    bool disconnected = false;

    void changeLampe0 (int LampenZustand);
    void changeLampe1 (int LampenZustand);
    void changeLampe2 (int LampenZustand);
    void changeLampe3 (int LampenZustand);
    void changeLampe4 (int LampenZustand);
    void changeLampe5 (int LampenZustand);
    void changeLampe6 (int LampenZustand);
    void changeLampe7 (int LampenZustand);
    void changeLampe8 (int LampenZustand);
    void changeLampe9 (int LampenZustand);
    void changeLampe10 (int LampenZustand);
    void changeLampenUI(int LampenId, int LampenZustand);
    int addMuster(QString titel, QString R1, QString R2, QString R3, QString R4, QString R5, QString Geraete, QString Action);
    void clearAllMuster();
    void DisplayPopup(QString);
    void configResetAll ();
    void sendCurrentValues();
    void changeAufnahme(int value, bool send = 1);
    void changeSchwelle(int value, bool send = 1);
    void changeToleranz (int value, bool send = 1);
    void changeStille(int value, bool send = 1);
    void changePieper(int value, bool send = 1);
    void changeSound(QString value, bool send = 1);
    void closeArduinoPort();
    void writeArduinoData(QString str);
    void setLampenMode(int, int);
    void gerSaveAll();
    void saveAllMuster(bool showInfo);

signals:
    void writeToArduino(QString);
    void AvailablePorts();
    void Connect(QString);
    void Close(); /** Getriggert durch GUI. Löst Trennen der Verbindung mit dem Arduino aus. */
    void clearStack(); /** Löscht die ganze Warteschlange */
    void fixProcessed(); /** Simuliert Signal von Arduin, dass Arduino empfangsbereit ist. */
};

#endif // KLATSCHUI_H
