#include <QtDebug>
#include "klatschui.h"
#include "ui_klatschui.h"
#include "serialportlistener.h"
//#include <QTextStream>
#include <QMessageBox>


klatschui::klatschui(QWidget *parent) : QMainWindow(parent), ui(new Ui::klatschui) {
    /** Konstruktor für Klasse, in der alle Oberflächensteueraktionen ausgeführt werden.*/
    ui->setupUi(this);

    SerialPort = new QSerialPort(this);

    SPL = new SerialPortListener(SerialPort, 500);

    connect(this, SIGNAL(AvailablePorts()),
            SPL,  SLOT(AvailablePorts()));
    connect(SPL,  SIGNAL(sendBackAvailablePorts(QList<QSerialPortInfo>, int, QString)),
            this, SLOT(PortListeAktualisieren(QList<QSerialPortInfo>, int, QString)));
    connect(this, SIGNAL(Connect(QString)),
            SPL,  SLOT(Connect(QString)));
    connect(SPL,  SIGNAL(backToConnect(int, QString)),
            this, SLOT(WhenHandledConnected(int, QString)));
    connect(this, SIGNAL(Close()),
            SPL,  SLOT(Close()));
    connect(SPL,  SIGNAL(dataReceived(QString)),
            this, SLOT(readArduinoData(QString)));
    connect(this, SIGNAL(writeToArduino(QString)),
            SPL,  SLOT(writeToQueue(QString)));
    connect(SPL,  SIGNAL(sendDataToGuiToArduino(QByteArray)),
            this, SLOT(Send(QByteArray)));
    connect(this, SIGNAL(clearStack()),
            SPL,  SLOT(clearStack()));
    connect(SPL,  SIGNAL(numberInStack(int)),
            this, SLOT(numberInStackToGUI(int)));
    connect(this, SIGNAL(fixProcessed()),
            SPL,  SLOT(fixProcessed()));

    //connect(SerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleErrors(QSerialPort::SerialPortError)));

    changeAufnahme(ui->configIntAufnahme->value(), 0);
    changeSchwelle(ui->configIntSchwelle->value(), 0);
    changeStille(ui->configIntStille->value(), 0);
    changeToleranz(ui->configIntToleranz->value(), 0);

    emit AvailablePorts();
    ui->configPinDisconnect->setDisabled(1);
}

klatschui::~klatschui() /** Löschen der Benutzeroberfläche beim Beenden des Threads */
{
    delete ui;
}

void klatschui::Send(QByteArray data) /** Daten direkt aus der Oberfläche an den Arduino senden, keine Queuing. Keine Sicherheit, dass der Arduino grade empangsbereit ist. */
{
    SerialPort->write(data);
}

void klatschui::on_configIntAllDef_clicked() /** Getriggert beim Klick auf "configAllDef". */
{
    configResetAll();
}

void klatschui::PortListeAktualisieren(QList<QSerialPortInfo> portInfoList, int isOpen, QString portName) /** Getriggert durch Signal "sendBackAvailablePorts" von SPL. Bekommt Daten von SPL und aktualisiert die GUI. */
{
    ui->configPinArduino->clear(); // aktuelle Liste löschen

    for (int i = 0; i < portInfoList.size(); i++) { // Liste durchgehen, jedes in neue Zeile einfügen
        QSerialPortInfo &info = portInfoList[i];

        ui->configPinArduino->addItem(info.portName(),info.portName());

        if (isOpen) { // bei geöffneter Verbindung: Verbundenes Gerät als ausgewählt markieren
            if (info.portName() == portName) {
                ui->configPinArduino->setCurrentIndex(i);
            }
        }
    }
}

void klatschui::on_configPinVerbinden_clicked() /** Getriggert beim Klick auf "configPinVerbinden". Simuliert Signal Connect mit Wert des aktuell ausgewählten Geräts. Löst "Connect" im SPL aus. */
{
    emit Connect(ui->configPinArduino->currentText()); // ausgewähltes Gerät an PL übergeben
}

void klatschui::WhenHandledConnected(int connectedTrueFalse, QString portName) { /** Getriggert durch Signal "backToConnect" von SPL. Prüft Verbindungsstatus. Bei erfolgreicher Verbindung: Aktualisiert GUI, startet Run-Funktion des SPL */
    if(connectedTrueFalse){
        emit clearStack();
            clearAllMuster();
        SPL->start();
        ui->configPinStatus1->setText("<table><tr><td><span style='color:green;'>◉  </span></td><td>Status: Verbunden mit <b>" + portName + "</b></td></tr><tr><td>&nbsp;</td><td>&nbsp;</td></tr></table>");
        ui->configPinDisconnect->setDisabled(0);
        ui->configPinVerbinden->setDisabled(1);
        sendCurrentValues();
    } else{
        ui->configPinStatus1->setText("<table><tr><td><span style='color:red;'>◉  </span></td><td>Status: Nicht Verbunden</td></tr><tr><td>&nbsp;</td><td>Verbindungsversuch mit <b>"+ ui->configPinArduino->currentText() + "</b> fehlgeschlagen.</td></tr></table>");
    }
}

void klatschui::on_configPinDisconnect_clicked() /** Getriggert beim Klick auf "configPinDisconnect". Führt Funktion zum Trennen der Verbindung aus. */
{
    closeArduinoPort();
}

void klatschui::closeArduinoPort() /** Aktualisiert GUI. Simuliert Signal "Close" an SPL und trennt dort die Verbindung zum Arduino. */
{
    QString last_port = ui->configPinArduino->currentText();
    ui->configPinStatus1->setText("<table><tr><td><span style='color:yellow;'>◉  </span></td><td>Status: Nicht Verbunden</td></tr><tr><td>&nbsp;</td><td>Verbindung mit "+ last_port + " geschlossen.</td></tr></table>");
    ui->configPinDisconnect->setDisabled(1);
    ui->configPinVerbinden->setDisabled(0);
    disconnected = true;
    emit Close();
    // hier quit SPL implementieren
}

void klatschui::readArduinoData(QString text) /** Getriggert durch Signal "dataReceived" vom SPL. Analysiert empangene Daten und löst Funktionen aus. */
{
    if (text.indexOf("statusLampe", 0) >= 0) {
        int pos = 0;

        while (1) {
            if (text.indexOf("statusLampe", pos) == -1) {
                break;
            }
            QString input = text.mid(text.indexOf("statusLampe", pos)+11);
            int Delim         = input.indexOf("~");
            int LampenId      = input.mid(0, Delim).toInt();
            int LampenZustand = input.mid(Delim+1, 1).toInt();
            // qDebug() << "delim:" << Delim << "id: " << LampenId << "zustan: " << LampenZustand;
            qDebug() << "Delim pos.: " + QString::number(Delim) + "; ID: " + QString::number(LampenId) + "; Zustand: " + QString::number(LampenZustand);

            changeLampenUI(LampenId, LampenZustand);

            pos = text.indexOf("statusLampe", pos)+11+Delim+2;
            qDebug() << pos;
        }

    } else if (text.indexOf("SoundWert") >= 0) {
        QString input = text.mid(text.indexOf("SoundWert")+9);
        ui->configSoundWert->setText(input.mid(0,4));
        text = "";
    } else if (text.contains("Notice:")) { // Prüfen ob immernoch versendet
      DisplayPopup(text.mid(7));
      text="";
    } else if (text.contains("Error10")) {
        text = "";
      DisplayPopup("Es ist ein Problem bei der Kommunikation aufgetreten. Der Befehl konnte nicht verarbeitet werden.");
    }/* else if (text.contains("Error")) {
        DisplayPopup(text);
        text = "";
    }*/
}

void klatschui::changeLampenUI(int LampenId, int LampenZustand) /** Ruft die richtige Funktion auf, um die GUI zu ändern.  */
{
    if (LampenId == 0) {
           changeLampe0(LampenZustand);
    } else if (LampenId == 1) {
           changeLampe1(LampenZustand);
    } else if (LampenId == 2) {
         changeLampe2(LampenZustand);
    } else if (LampenId == 3) {
         changeLampe3(LampenZustand);
    } else if (LampenId == 4) {
         changeLampe4(LampenZustand);
    } else if (LampenId == 5) {
         changeLampe5(LampenZustand);
    } else if (LampenId == 6) {
         changeLampe6(LampenZustand);
    } else if (LampenId == 7) {
         changeLampe7(LampenZustand);
    } else if (LampenId == 8) {
         changeLampe8(LampenZustand);
    } else if (LampenId == 9) {
         changeLampe9(LampenZustand);
    } else if (LampenId == 10) {
         changeLampe10(LampenZustand);
    }
}

void klatschui::writeArduinoData(QString str) /** Sendet Daten über Queuing im SPL an den Arduino. */
{
    qDebug() << "to SPL:" << str;
    QByteArray data = str.toUtf8();
    emit writeToArduino(data);
}

void klatschui::numberInStackToGUI(int elem) /** Getriggert durch "numberInStack" vom SPL. Gibt die Anzahl der ausstehenden Befehle an. */
{
    ui->BefehleInArbeitWert->setText(QString::number(elem));
}

/*void klatschui::handleErrors(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        ui->terminal->append(SerialPort->errorString());
        closeArduinoPort();
    }
}*/

void klatschui::sendCurrentValues() /** Sendet alle auf der GUI angezeigten Werte nach Verbindungsaufbau an den Arduino. */
{
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
    gerSaveAll();
    saveAllMuster(0);
    DisplayPopup("Alle Einstellungen wurden an das Gerät gesendet.");
}


void klatschui::changeAufnahme(int value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configIntAufnahmeWert->setText(QString::number(value));
    ui->configIntAufnahme->setValue(value);
    if (send) {
        QString send = "setIntAufnahme";
        send += QString::number(value);
        writeArduinoData(send);
    }
}
void klatschui::changeSchwelle(int value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configIntSchwelleWert->setText(QString::number(value));
    ui->configIntSchwelle->setValue(value);
    if (send) {
        QString send = "setIntSchwelle";
        send += QString::number(value);
        writeArduinoData(send);
    }
}
void klatschui::changeStille(int value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configIntStilleWert->setText(QString::number(value));
    ui->configIntStille->setValue(value);
    if (send) {
        QString send = "setIntStille";
        send += QString::number(value);
        writeArduinoData(send);
    }
}
void klatschui::changeToleranz(int value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configIntToleranzWert->setText(QString::number(value));
    ui->configIntToleranz->setValue(value);
        if (send) {
        QString send = "setIntToleranz";
        send += QString::number(value);
        writeArduinoData(send);
    }
}
void klatschui::changePieper(int value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configPinPieper->setCurrentText(QString::number(value));
    if (send) {
        QString send = "setPinModePieper";
        send += QString::number(value);
        writeArduinoData(send);
    }
}
void klatschui::changeSound(QString value, bool send) /** change*-Funktionen: Ändern einen Wert auf der GUI und im Arduino. "send" standardmäßig true. Bei send false wird nur die GUI aktualisiert; keine Daten werden an den Arduino gesendet. */
{
    ui->configPinSound->setCurrentText(value);
    if (send) {
        QString send = "setPinModeSound";
        send += value;
        writeArduinoData(send);
    }
}

void klatschui::gerSaveAll() /** Speichert neue Geräte. Bei erster unkorrekt ausgefüllter/leerer Zeile wird Speichervorgang beendet. GUI wird aktualisiert. */
{
  bool continueProcess = 1;
  QString send = "setGeraete";
  // überprüft, ob Zeilen mit neuem Content
  if (ui->gerPin_0->currentText() != "-" && ui->gerStart_0->currentText() != "-" && continueProcess) {
      send += ui->gerPin_0->currentText();
      send += ',';
      if (ui->gerStart_0->currentText() == "an") {
          send += "1";
          changeLampe0(1);
      } else if (ui->gerStart_0->currentText() == "aus") {
          send += "0";
          changeLampe0(0);
      }
      send += '~';

  } else {
      continueProcess = 0;
      ui->steuGerAus_0->setDisabled(1);
      ui->steuGerEin_0->setDisabled(1);
      ui->steuGerStatus_0->setText("");
  }
  if (ui->gerPin_1->currentText() != "-" && ui->gerStart_1->currentText() != "-" && continueProcess) {

      send += ui->gerPin_1->currentText();
      send += ',';
      if (ui->gerStart_1->currentText() == "an") {
          send += "1";
          changeLampe1(1);
      } else if (ui->gerStart_1->currentText() == "aus") {
          send += "0";
          changeLampe1(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_2->setDisabled(1);
      ui->steuGerEin_2->setDisabled(1);
      ui->steuGerStatus_2->setText("");
  }
  if (ui->gerPin_2->currentText() != "-" && ui->gerStart_2->currentText() != "-" && continueProcess) {
      send += ui->gerPin_2->currentText();
      send += ',';
      if (ui->gerStart_2->currentText() == "an") {
          send += "1";
          changeLampe2(1);
      } else if (ui->gerStart_2->currentText() == "aus") {
          send += "0";
          changeLampe2(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_4->setDisabled(1);
      ui->steuGerEin_4->setDisabled(1);
      ui->steuGerStatus_4->setText("");
  }
  if (ui->gerPin_3->currentText() != "-" && ui->gerStart_3->currentText() != "-" && continueProcess) {
      send += ui->gerPin_3->currentText();
      send += ',';
      if (ui->gerStart_3->currentText() == "an") {
          send += "1";
          changeLampe3(1);
      } else if (ui->gerStart_3->currentText() == "aus") {
          send += "0";
          changeLampe3(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_5->setDisabled(1);
      ui->steuGerEin_5->setDisabled(1);
      ui->steuGerStatus_5->setText("");
  }
  if (ui->gerPin_4->currentText() != "-" && ui->gerStart_4->currentText() != "-" && continueProcess) {
      send += ui->gerPin_4->currentText();
      send += ',';
      if (ui->gerStart_4->currentText() == "an") {
          send += "1";
          changeLampe4(1);
      } else if (ui->gerStart_4->currentText() == "aus") {
          send += "0";
          changeLampe4(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_6->setDisabled(1);
      ui->steuGerEin_6->setDisabled(1);
      ui->steuGerStatus_6->setText("");
  }
  if (ui->gerPin_5->currentText() != "-" && ui->gerStart_5->currentText() != "-" && continueProcess) {
      send += ui->gerPin_5->currentText();
      send += ',';
      if (ui->gerStart_5->currentText() == "an") {
          send += "1";
          changeLampe5(1);
      } else if (ui->gerStart_5->currentText() == "aus") {
          send += "0";
          changeLampe5(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_7->setDisabled(1);
      ui->steuGerEin_7->setDisabled(1);
      ui->steuGerStatus_7->setText("");
  }
  if (ui->gerPin_6->currentText() != "-" && ui->gerStart_6->currentText() != "-" && continueProcess) {
      send += ui->gerPin_6->currentText();
      send += ',';
      if (ui->gerStart_6->currentText() == "an") {
          send += "1";
          changeLampe6(1);
      } else if (ui->gerStart_6->currentText() == "aus") {
          send += "0";
          changeLampe6(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_8->setDisabled(1);
      ui->steuGerEin_8->setDisabled(1);
      ui->steuGerStatus_8->setText("");
  }
  if (ui->gerPin_7->currentText() != "-" && ui->gerStart_7->currentText() != "-" && continueProcess) {
      send += ui->gerPin_7->currentText();
      send += ',';
      if (ui->gerStart_7->currentText() == "an") {
          send += "1";
          changeLampe7(1);
      } else if (ui->gerStart_7->currentText() == "aus") {
          send += "0";
          changeLampe7(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_9->setDisabled(1);
      ui->steuGerEin_9->setDisabled(1);
      ui->steuGerStatus_9->setText("");
  }
  if (ui->gerPin_8->currentText() != "-" && ui->gerStart_8->currentText() != "-" && continueProcess) {
      send += ui->gerPin_8->currentText();
      send += ',';
      if (ui->gerStart_8->currentText() == "an") {
          send += "1";
          changeLampe8(1);
      } else if (ui->gerStart_8->currentText() == "aus") {
          send += "0";
          changeLampe8(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_10->setDisabled(1);
      ui->steuGerEin_10->setDisabled(1);
      ui->steuGerStatus_10->setText("");
  }
  if (ui->gerPin_9->currentText() != "-" && ui->gerStart_9->currentText() != "-" && continueProcess) {
      send += ui->gerPin_9->currentText();
      send += ',';
      if (ui->gerStart_9->currentText() == "an") {
          send += "1";
          changeLampe9(1);
      } else if (ui->gerStart_9->currentText() == "aus") {
          send += "0";
          changeLampe9(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_20->setDisabled(1);
      ui->steuGerEin_20->setDisabled(1);
      ui->steuGerStatus_20->setText("");
  }
  if (ui->gerPin_10->currentText() != "-" && ui->gerStart_10->currentText() != "-" && continueProcess) {
      send += ui->gerPin_10->currentText();
      send += ',';
      if (ui->gerStart_10->currentText() == "an") {
          send += "1";
          changeLampe10(1);
      } else if (ui->gerStart_10->currentText() == "aus") {
          send += "0";
          changeLampe10(0);
      }
      send += '~';
  } else {
      continueProcess = 0;
      ui->steuGerAus_21->setDisabled(1);
      ui->steuGerEin_21->setDisabled(1);
      ui->steuGerStatus_21->setText("");
  }
  writeArduinoData(send);
}

int klatschui::addMuster(QString titel, QString R1, QString R2, QString R3, QString R4, QString R5, QString Geraete, QString Action) /** Analysiert eine Muster-Zeile. Falls valide: Sendet Daten an Arduino. */
{
    int stopRythm = 0;
    QString send = "setLampe";

    if (titel == "") {
        qDebug() << "Titel darf nicht leer sein";
        DisplayPopup("Titel darf nicht leer sein");
        return 1;
    }
    send += titel;
    send += "~";

    if (R1 == "-") {
        qDebug() << "Muster darf nicht leer sein";
        DisplayPopup("Muster darf nicht leer sein");
        return 1;
    } else {
        send += R1;
        if (R2.contains("-")) {
            stopRythm = 1;
        } else {
            //send += ",";
            send += R2;
            stopRythm = 1;
            if (R3.contains("-") && stopRythm) {
                stopRythm = 1;
            } else {
                //send += ",";
                send += R3;
                if (R4.contains("-") && stopRythm) {
                    stopRythm = 1;
                } else {
                    //send += ",";
                    send += R4;
                    if (R5.contains("-") && stopRythm) {
                        stopRythm = 1;
                    } else {
                        //send += ",";
                        send += R5;
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

void klatschui::saveAllMuster(bool showInfo = 1) /** Speichert alle validen Muster. Ruft jedes mal "addMuster" auf. */
{
  int count = 0;
  if (ui->musLine_0->text() != "" && ui->mus0m1->currentText() != "-" && ui->musGer_0->text() != "" && ui->musLine_0->isEnabled()) {
      on_musSave_0_clicked();
      count++;
  }
  if (ui->musLine_1->text() != "" && ui->mus1m1->currentText() != "-" && ui->musGer_1->text() != ""&& ui->musLine_1->isEnabled()) {
      on_musSave_1_clicked();
      count++;
  }
  if (ui->musLine_2->text() != "" && ui->mus2m1->currentText() != "-" && ui->musGer_2->text() != ""&& ui->musLine_2->isEnabled()) {
      on_musSave_2_clicked();
      count++;
  }
  if (ui->musLine_3->text() != "" && ui->mus3m1->currentText() != "-" && ui->musGer_3->text() != ""&& ui->musLine_3->isEnabled()) {
      on_musSave_3_clicked();
      count++;
  }
  if (ui->musLine_4->text() != "" && ui->mus4m1->currentText() != "-" && ui->musGer_4->text() != ""&& ui->musLine_4->isEnabled()) {
      on_musSave_4_clicked();
      count++;
  }
  if (ui->musLine_5->text() != "" && ui->mus5m1->currentText() != "-" && ui->musGer_5->text() != ""&& ui->musLine_5->isEnabled()) {
      on_musSave_5_clicked();
      count++;
  }
  if (ui->musLine_6->text() != "" && ui->mus7m1->currentText() != "-" && ui->musGer_6->text() != ""&& ui->musLine_6->isEnabled()) {
      on_musSave_6_clicked();
      count++;
  }
  if (ui->musLine_7->text() != "" && ui->mus7m1->currentText() != "-" && ui->musGer_7->text() != ""&& ui->musLine_7->isEnabled()) {
      on_musSave_7_clicked();
      count++;
  }
  if (ui->musLine_8->text() != "" && ui->mus8m1->currentText() != "-" && ui->musGer_8->text() != ""&& ui->musLine_8->isEnabled()) {
      on_musSave_8_clicked();
      count++;
  }
  if (ui->musLine_9->text() != "" && ui->mus9m1->currentText() != "-" && ui->musGer_9->text() != ""&& ui->musLine_9->isEnabled()) {
      on_musSave_9_clicked();
      count++;
  }
  if (ui->musLine_10->text() != "" && ui->mus10m1->currentText() != "-" && ui->musGer_10->text() != ""&& ui->musLine_10->isEnabled()) {
      on_musSave_10_clicked();
      count++;
  }
  if (showInfo) {
    if (count < 1) {
        DisplayPopup("Es wurde nichts gespeichert. Für detailierte Fehlermeldungen Zeilen einzeln speichern");
    } else if (count == 1) {
        DisplayPopup("Es wurde 1 Änderung durchgeführt.");
    } else {
        DisplayPopup("Es wurden " + QString::number(count) + " Speicherungen durchgeführt.");
    }
  }
}

void klatschui::clearAllMuster() /** Löscht alle Muster auf dem Arduino, aktualisiert GUI. */
{
    QString send = "clearAllMuster";
    writeArduinoData(send);

    ui->musLine_0->setDisabled(0);
    ui->mus0m1->setDisabled(0);
    on_mus0m1_currentTextChanged(ui->mus0m1->currentText()); // Da nicht jedes Rythmusfeld wieder freigegeben werden soll, wird das Feld abhängig vom Inhalt klickbar gemacht
    ui->musGer_0->setDisabled(0);
    ui->musAction_0->setDisabled(0);
    ui->musSave_0->setDisabled(0);
    ui->musLine_1->setDisabled(0);
    ui->mus1m1->setDisabled(0);
    on_mus1m1_currentTextChanged(ui->mus1m1->currentText()); // s.o.
    ui->musGer_1->setDisabled(0);
    ui->musAction_1->setDisabled(0);
    ui->musSave_1->setDisabled(0);
    ui->musLine_2->setDisabled(0);
    ui->mus2m1->setDisabled(0);
    on_mus2m1_currentTextChanged(ui->mus2m1->currentText());
    ui->musGer_2->setDisabled(0);
    ui->musAction_2->setDisabled(0);
    ui->musSave_2->setDisabled(0);
    ui->musLine_3->setDisabled(0);
    ui->mus3m1->setDisabled(0);
    on_mus3m1_currentTextChanged(ui->mus3m1->currentText());
    ui->musGer_3->setDisabled(0);
    ui->musAction_3->setDisabled(0);
    ui->musSave_3->setDisabled(0);
    ui->musLine_4->setDisabled(0);
    ui->mus4m1->setDisabled(0);
    on_mus4m1_currentTextChanged(ui->mus4m1->currentText());
    ui->musGer_4->setDisabled(0);
    ui->musAction_4->setDisabled(0);
    ui->musSave_4->setDisabled(0);
    ui->musLine_5->setDisabled(0);
    ui->mus5m1->setDisabled(0);
    on_mus5m1_currentTextChanged(ui->mus5m1->currentText());
    ui->musGer_5->setDisabled(0);
    ui->musAction_5->setDisabled(0);
    ui->musSave_5->setDisabled(0);
    ui->musLine_6->setDisabled(0);
    ui->mus6m1->setDisabled(0);
    on_mus6m1_currentTextChanged(ui->mus6m1->currentText());
    ui->musGer_6->setDisabled(0);
    ui->musAction_6->setDisabled(0);
    ui->musSave_6->setDisabled(0);
    ui->musLine_7->setDisabled(0);
    ui->mus7m1->setDisabled(0);
    on_mus7m1_currentTextChanged(ui->mus7m1->currentText());
    ui->musGer_7->setDisabled(0);
    ui->musAction_7->setDisabled(0);
    ui->musSave_7->setDisabled(0);
    ui->musLine_8->setDisabled(0);
    ui->mus8m1->setDisabled(0);
    on_mus8m1_currentTextChanged(ui->mus8m1->currentText());
    ui->musGer_8->setDisabled(0);
    ui->musAction_8->setDisabled(0);
    ui->musSave_8->setDisabled(0);
    ui->musLine_9->setDisabled(0);
    ui->mus9m1->setDisabled(0);
    on_mus9m1_currentTextChanged(ui->mus9m1->currentText());
    ui->musGer_9->setDisabled(0);
    ui->musAction_9->setDisabled(0);
    ui->musSave_9->setDisabled(0);
    ui->musLine_10->setDisabled(0);
    ui->mus10m1->setDisabled(0);
    on_mus10m1_currentTextChanged(ui->mus10m1->currentText());
    ui->musGer_10->setDisabled(0);
    ui->musAction_10->setDisabled(0);
    ui->musSave_10->setDisabled(0);
}

void klatschui::DisplayPopup(QString text) /** Lässt Popup-Meldung erscheinen mit var text. */
{
    QMessageBox msgBox;
    msgBox.setText(text);
    msgBox.exec();
}

/* Muster */
void klatschui::on_musSave_0_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_0->text(),
              ui->mus0m1->currentText(),
              ui->mus0m2->currentText(),
              ui->mus0m3->currentText(),
              ui->mus0m4->currentText(),
              ui->mus0m5->currentText(),
              ui->musGer_0->text(),
              ui->musAction_0->currentText() ))
    {
        ui->musLine_0->setDisabled(1);
        ui->mus0m1->setDisabled(1);
        ui->mus0m2->setDisabled(1);
        ui->mus0m3->setDisabled(1);
        ui->mus0m4->setDisabled(1);
        ui->mus0m5->setDisabled(1);
        ui->musGer_0->setDisabled(1);
        ui->musAction_0->setDisabled(1);
        ui->musSave_0->setDisabled(1);
    }
}
void klatschui::on_musSave_1_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_1->text(),
              ui->mus1m1->currentText(),
              ui->mus1m2->currentText(),
              ui->mus1m3->currentText(),
              ui->mus1m4->currentText(),
              ui->mus1m5->currentText(),
              ui->musGer_1->text(),
              ui->musAction_1->currentText() ))
    {
        ui->musLine_1->setDisabled(1);
        ui->mus1m1->setDisabled(1);
        ui->mus1m2->setDisabled(1);
        ui->mus1m3->setDisabled(1);
        ui->mus1m4->setDisabled(1);
        ui->mus1m5->setDisabled(1);
        ui->musGer_1->setDisabled(1);
        ui->musAction_1->setDisabled(1);
        ui->musSave_1->setDisabled(1);
    }
}
void klatschui::on_musSave_2_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_2->text(),
              ui->mus2m1->currentText(),
              ui->mus2m2->currentText(),
              ui->mus2m3->currentText(),
              ui->mus2m4->currentText(),
              ui->mus2m5->currentText(),
              ui->musGer_2->text(),
              ui->musAction_2->currentText() ))
    {
        ui->musLine_2->setDisabled(1);
        ui->mus2m1->setDisabled(1);
        ui->mus2m2->setDisabled(1);
        ui->mus2m3->setDisabled(1);
        ui->mus2m4->setDisabled(1);
        ui->mus2m5->setDisabled(1);
        ui->musGer_2->setDisabled(1);
        ui->musAction_2->setDisabled(1);
        ui->musSave_2->setDisabled(1);
    }
}
void klatschui::on_musSave_3_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_3->text(),
              ui->mus3m1->currentText(),
              ui->mus3m2->currentText(),
              ui->mus3m3->currentText(),
              ui->mus3m4->currentText(),
              ui->mus3m5->currentText(),
              ui->musGer_3->text(),
              ui->musAction_3->currentText() ))
    {
        ui->musLine_3->setDisabled(1);
        ui->mus3m1->setDisabled(1);
        ui->mus3m2->setDisabled(1);
        ui->mus3m3->setDisabled(1);
        ui->mus3m4->setDisabled(1);
        ui->mus3m5->setDisabled(1);
        ui->musGer_3->setDisabled(1);
        ui->musAction_3->setDisabled(1);
        ui->musSave_3->setDisabled(1);
    }
}
void klatschui::on_musSave_4_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_4->text(),
              ui->mus4m1->currentText(),
              ui->mus4m2->currentText(),
              ui->mus4m3->currentText(),
              ui->mus4m4->currentText(),
              ui->mus4m5->currentText(),
              ui->musGer_4->text(),
              ui->musAction_4->currentText() ))
    {
        ui->musLine_4->setDisabled(1);
        ui->mus4m1->setDisabled(1);
        ui->mus4m2->setDisabled(1);
        ui->mus4m3->setDisabled(1);
        ui->mus4m4->setDisabled(1);
        ui->mus4m5->setDisabled(1);
        ui->musGer_4->setDisabled(1);
        ui->musAction_4->setDisabled(1);
        ui->musSave_4->setDisabled(1);
    }
}
void klatschui::on_musSave_5_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_5->text(),
              ui->mus5m1->currentText(),
              ui->mus5m2->currentText(),
              ui->mus5m3->currentText(),
              ui->mus5m4->currentText(),
              ui->mus5m5->currentText(),
              ui->musGer_5->text(),
              ui->musAction_5->currentText() ))
    {
        ui->musLine_5->setDisabled(1);
        ui->mus5m1->setDisabled(1);
        ui->mus5m2->setDisabled(1);
        ui->mus5m3->setDisabled(1);
        ui->mus5m4->setDisabled(1);
        ui->mus5m5->setDisabled(1);
        ui->musGer_5->setDisabled(1);
        ui->musAction_5->setDisabled(1);
        ui->musSave_5->setDisabled(1);
    }
}
void klatschui::on_musSave_6_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_6->text(),
              ui->mus6m1->currentText(),
              ui->mus6m2->currentText(),
              ui->mus6m3->currentText(),
              ui->mus6m4->currentText(),
              ui->mus6m5->currentText(),
              ui->musGer_6->text(),
              ui->musAction_6->currentText() ))
    {
        ui->musLine_6->setDisabled(1);
        ui->mus6m1->setDisabled(1);
        ui->mus6m2->setDisabled(1);
        ui->mus6m3->setDisabled(1);
        ui->mus6m4->setDisabled(1);
        ui->mus6m5->setDisabled(1);
        ui->musGer_6->setDisabled(1);
        ui->musAction_6->setDisabled(1);
        ui->musSave_6->setDisabled(1);
    }
}
void klatschui::on_musSave_7_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_7->text(),
              ui->mus7m1->currentText(),
              ui->mus7m2->currentText(),
              ui->mus7m3->currentText(),
              ui->mus7m4->currentText(),
              ui->mus7m5->currentText(),
              ui->musGer_7->text(),
              ui->musAction_7->currentText() ))
    {
        ui->musLine_7->setDisabled(1);
        ui->mus7m1->setDisabled(1);
        ui->mus7m2->setDisabled(1);
        ui->mus7m3->setDisabled(1);
        ui->mus7m4->setDisabled(1);
        ui->mus7m5->setDisabled(1);
        ui->musGer_7->setDisabled(1);
        ui->musAction_7->setDisabled(1);
        ui->musSave_7->setDisabled(1);
    }
}
void klatschui::on_musSave_8_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_8->text(),
              ui->mus8m1->currentText(),
              ui->mus8m2->currentText(),
              ui->mus8m3->currentText(),
              ui->mus8m4->currentText(),
              ui->mus8m5->currentText(),
              ui->musGer_8->text(),
              ui->musAction_8->currentText() ))
    {
        ui->musLine_8->setDisabled(1);
        ui->mus8m1->setDisabled(1);
        ui->mus8m2->setDisabled(1);
        ui->mus8m3->setDisabled(1);
        ui->mus8m4->setDisabled(1);
        ui->mus8m5->setDisabled(1);
        ui->musGer_8->setDisabled(1);
        ui->musAction_8->setDisabled(1);
        ui->musSave_8->setDisabled(1);
    }
}
void klatschui::on_musSave_9_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_9->text(),
              ui->mus9m1->currentText(),
              ui->mus9m2->currentText(),
              ui->mus9m3->currentText(),
              ui->mus9m4->currentText(),
              ui->mus9m5->currentText(),
              ui->musGer_9->text(),
              ui->musAction_9->currentText() ))
    {
        ui->musLine_9->setDisabled(1);
        ui->mus9m1->setDisabled(1);
        ui->mus9m2->setDisabled(1);
        ui->mus9m3->setDisabled(1);
        ui->mus9m4->setDisabled(1);
        ui->mus9m5->setDisabled(1);
        ui->musGer_9->setDisabled(1);
        ui->musAction_9->setDisabled(1);
        ui->musSave_9->setDisabled(1);
    }
}
void klatschui::on_musSave_10_clicked() /** Getriggert beim Klick auf "musSave_*". Aktualisiert GUI. Speichert auf Arduino.*/
{
    if (!addMuster(ui->musLine_10->text(),
              ui->mus10m1->currentText(),
              ui->mus10m2->currentText(),
              ui->mus10m3->currentText(),
              ui->mus10m4->currentText(),
              ui->mus10m5->currentText(),
              ui->musGer_10->text(),
              ui->musAction_10->currentText() ))
    {
        ui->musLine_10->setDisabled(1);
        ui->mus10m1->setDisabled(1);
        ui->mus10m2->setDisabled(1);
        ui->mus10m3->setDisabled(1);
        ui->mus10m4->setDisabled(1);
        ui->mus10m5->setDisabled(1);
        ui->musGer_10->setDisabled(1);
        ui->musAction_10->setDisabled(1);
        ui->musSave_10->setDisabled(1);
    }
}
void klatschui::on_musSave_clicked() /** Getriggert beim Klicken auf "musSave". Löst speicherung aller Muster aus. */
{
    saveAllMuster();
}
void klatschui::on_musClear_clicked() /** Getriggert beim Klicken auf "musClear". Löscht alle Muster vom Arduino. Enabled alle Muster-Felder, löscht jedoch keinen Inhalt. */
{
    clearAllMuster();
}
void klatschui::on_mus0m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus0m2->setEnabled(1);
        if (ui->mus0m2->currentText() != "-") {
            ui->mus0m3->setEnabled(1);
        }
        if (ui->mus0m3->currentText() != "-") {
            ui->mus0m4->setEnabled(1);
        }
        if (ui->mus0m4->currentText() != "-") {
            ui->mus0m5->setEnabled(1);
        }
      } else {
        ui->mus0m2->setEnabled(0);
        ui->mus0m3->setEnabled(0);
        ui->mus0m4->setEnabled(0);
        ui->mus0m5->setEnabled(0);
      }
}
void klatschui::on_mus0m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus0m3->setEnabled(1);
      if (ui->mus0m3->currentText() != "-") {
          ui->mus0m4->setEnabled(1);
      }
      if (ui->mus0m4->currentText() != "-") {
          ui->mus0m5->setEnabled(1);
      }
  } else {
      ui->mus0m3->setEnabled(0);
      ui->mus0m4->setEnabled(0);
      ui->mus0m5->setEnabled(0);
    }
}
void klatschui::on_mus0m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus0m4->setEnabled(1);
      if (ui->mus0m4->currentText() != "-") {
          ui->mus0m5->setEnabled(1);
      }
  } else {
      ui->mus0m4->setEnabled(0);
      ui->mus0m5->setEnabled(0);
    }
}
void klatschui::on_mus0m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus0m5->setEnabled(1);
  }else {
      ui->mus0m5->setEnabled(0);
    }
}
void klatschui::on_mus1m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus1m2->setEnabled(1);
        if (ui->mus1m2->currentText() != "-") {
            ui->mus1m3->setEnabled(1);
        }
        if (ui->mus1m3->currentText() != "-") {
            ui->mus1m4->setEnabled(1);
        }
        if (ui->mus1m4->currentText() != "-") {
            ui->mus1m5->setEnabled(1);
        }
      } else {
        ui->mus1m2->setEnabled(0);
        ui->mus1m3->setEnabled(0);
        ui->mus1m4->setEnabled(0);
        ui->mus1m5->setEnabled(0);
      }
}
void klatschui::on_mus1m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus1m3->setEnabled(1);
      if (ui->mus1m3->currentText() != "-") {
          ui->mus1m4->setEnabled(1);
      }
      if (ui->mus1m4->currentText() != "-") {
          ui->mus1m5->setEnabled(1);
      }
  } else {
      ui->mus1m3->setEnabled(0);
      ui->mus1m4->setEnabled(0);
      ui->mus1m5->setEnabled(0);
    }
}
void klatschui::on_mus1m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus1m4->setEnabled(1);
      if (ui->mus1m4->currentText() != "-") {
          ui->mus1m5->setEnabled(1);
      }
  } else {
      ui->mus1m4->setEnabled(0);
      ui->mus1m5->setEnabled(0);
    }
}
void klatschui::on_mus1m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus1m5->setEnabled(1);
  }else {
      ui->mus1m5->setEnabled(0);
    }
}
void klatschui::on_mus2m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus2m2->setEnabled(1);
        if (ui->mus2m2->currentText() != "-") {
            ui->mus2m3->setEnabled(1);
        }
        if (ui->mus2m3->currentText() != "-") {
            ui->mus2m4->setEnabled(1);
        }
        if (ui->mus2m4->currentText() != "-") {
            ui->mus2m5->setEnabled(1);
        }
      } else {
        ui->mus2m2->setEnabled(0);
        ui->mus2m3->setEnabled(0);
        ui->mus2m4->setEnabled(0);
        ui->mus2m5->setEnabled(0);
      }
}
void klatschui::on_mus2m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus2m3->setEnabled(1);
      if (ui->mus2m3->currentText() != "-") {
          ui->mus2m4->setEnabled(1);
      }
      if (ui->mus2m4->currentText() != "-") {
          ui->mus2m5->setEnabled(1);
      }
  } else {
      ui->mus2m3->setEnabled(0);
      ui->mus2m4->setEnabled(0);
      ui->mus2m5->setEnabled(0);
    }
}
void klatschui::on_mus2m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus2m4->setEnabled(1);
      if (ui->mus2m4->currentText() != "-") {
          ui->mus2m5->setEnabled(1);
      }
  } else {
      ui->mus2m4->setEnabled(0);
      ui->mus2m5->setEnabled(0);
    }
}
void klatschui::on_mus2m4_currentTextChanged(const QString &arg1)/** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus2m5->setEnabled(1);
  }else {
      ui->mus2m5->setEnabled(0);
    }
}
void klatschui::on_mus3m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus3m2->setEnabled(1);
        if (ui->mus3m2->currentText() != "-") {
            ui->mus3m3->setEnabled(1);
        }
        if (ui->mus3m3->currentText() != "-") {
            ui->mus3m4->setEnabled(1);
        }
        if (ui->mus3m4->currentText() != "-") {
            ui->mus3m5->setEnabled(1);
        }
      } else {
        ui->mus3m2->setEnabled(0);
        ui->mus3m3->setEnabled(0);
        ui->mus3m4->setEnabled(0);
        ui->mus3m5->setEnabled(0);
      }
}
void klatschui::on_mus3m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus3m3->setEnabled(1);
      if (ui->mus3m3->currentText() != "-") {
          ui->mus3m4->setEnabled(1);
      }
      if (ui->mus3m4->currentText() != "-") {
          ui->mus3m5->setEnabled(1);
      }
  } else {
      ui->mus3m3->setEnabled(0);
      ui->mus3m4->setEnabled(0);
      ui->mus3m5->setEnabled(0);
    }
}
void klatschui::on_mus3m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus3m4->setEnabled(1);
      if (ui->mus3m4->currentText() != "-") {
          ui->mus3m5->setEnabled(1);
      }
  } else {
      ui->mus3m4->setEnabled(0);
      ui->mus3m5->setEnabled(0);
    }
}
void klatschui::on_mus3m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus3m5->setEnabled(1);
  }else {
      ui->mus3m5->setEnabled(0);
    }
}
void klatschui::on_mus4m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus4m2->setEnabled(1);
        if (ui->mus4m2->currentText() != "-") {
            ui->mus4m3->setEnabled(1);
        }
        if (ui->mus4m3->currentText() != "-") {
            ui->mus4m4->setEnabled(1);
        }
        if (ui->mus4m4->currentText() != "-") {
            ui->mus4m5->setEnabled(1);
        }
      } else {
        ui->mus4m2->setEnabled(0);
        ui->mus4m3->setEnabled(0);
        ui->mus4m4->setEnabled(0);
        ui->mus4m5->setEnabled(0);
      }
}
void klatschui::on_mus4m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus4m3->setEnabled(1);
      if (ui->mus4m3->currentText() != "-") {
          ui->mus4m4->setEnabled(1);
      }
      if (ui->mus4m4->currentText() != "-") {
          ui->mus4m5->setEnabled(1);
      }
  } else {
      ui->mus4m3->setEnabled(0);
      ui->mus4m4->setEnabled(0);
      ui->mus4m5->setEnabled(0);
    }
}
void klatschui::on_mus4m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus4m4->setEnabled(1);
      if (ui->mus4m4->currentText() != "-") {
          ui->mus4m5->setEnabled(1);
      }
  } else {
      ui->mus4m4->setEnabled(0);
      ui->mus4m5->setEnabled(0);
    }
}
void klatschui::on_mus4m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus4m5->setEnabled(1);
  }else {
      ui->mus4m5->setEnabled(0);
    }
}
void klatschui::on_mus5m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus5m2->setEnabled(1);
        if (ui->mus5m2->currentText() != "-") {
            ui->mus5m3->setEnabled(1);
        }
        if (ui->mus5m3->currentText() != "-") {
            ui->mus5m4->setEnabled(1);
        }
        if (ui->mus5m4->currentText() != "-") {
            ui->mus5m5->setEnabled(1);
        }
      } else {
        ui->mus5m2->setEnabled(0);
        ui->mus5m3->setEnabled(0);
        ui->mus5m4->setEnabled(0);
        ui->mus5m5->setEnabled(0);
      }
}
void klatschui::on_mus5m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus5m3->setEnabled(1);
      if (ui->mus5m3->currentText() != "-") {
          ui->mus5m4->setEnabled(1);
      }
      if (ui->mus5m4->currentText() != "-") {
          ui->mus5m5->setEnabled(1);
      }
  } else {
      ui->mus5m3->setEnabled(0);
      ui->mus5m4->setEnabled(0);
      ui->mus5m5->setEnabled(0);
    }
}
void klatschui::on_mus5m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus5m4->setEnabled(1);
      if (ui->mus5m4->currentText() != "-") {
          ui->mus5m5->setEnabled(1);
      }
  } else {
      ui->mus5m4->setEnabled(0);
      ui->mus5m5->setEnabled(0);
    }
}
void klatschui::on_mus5m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus5m5->setEnabled(1);
  }else {
      ui->mus5m5->setEnabled(0);
    }
}
void klatschui::on_mus6m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus6m2->setEnabled(1);
        if (ui->mus6m2->currentText() != "-") {
            ui->mus6m3->setEnabled(1);
        }
        if (ui->mus6m3->currentText() != "-") {
            ui->mus6m4->setEnabled(1);
        }
        if (ui->mus6m4->currentText() != "-") {
            ui->mus6m5->setEnabled(1);
        }
      } else {
        ui->mus6m2->setEnabled(0);
        ui->mus6m3->setEnabled(0);
        ui->mus6m4->setEnabled(0);
        ui->mus6m5->setEnabled(0);
      }
}
void klatschui::on_mus6m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus6m3->setEnabled(1);
      if (ui->mus6m3->currentText() != "-") {
          ui->mus6m4->setEnabled(1);
      }
      if (ui->mus6m4->currentText() != "-") {
          ui->mus6m5->setEnabled(1);
      }
  } else {
      ui->mus6m3->setEnabled(0);
      ui->mus6m4->setEnabled(0);
      ui->mus6m5->setEnabled(0);
    }
}
void klatschui::on_mus6m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus6m4->setEnabled(1);
      if (ui->mus6m4->currentText() != "-") {
          ui->mus6m5->setEnabled(1);
      }
  } else {
      ui->mus6m4->setEnabled(0);
      ui->mus6m5->setEnabled(0);
    }
}
void klatschui::on_mus6m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus6m5->setEnabled(1);
  }else {
      ui->mus6m5->setEnabled(0);
    }
}
void klatschui::on_mus7m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus7m2->setEnabled(1);
        if (ui->mus7m2->currentText() != "-") {
            ui->mus7m3->setEnabled(1);
        }
        if (ui->mus7m3->currentText() != "-") {
            ui->mus7m4->setEnabled(1);
        }
        if (ui->mus7m4->currentText() != "-") {
            ui->mus7m5->setEnabled(1);
        }
      } else {
        ui->mus7m2->setEnabled(0);
        ui->mus7m3->setEnabled(0);
        ui->mus7m4->setEnabled(0);
        ui->mus7m5->setEnabled(0);
      }
}
void klatschui::on_mus7m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus7m3->setEnabled(1);
      if (ui->mus7m3->currentText() != "-") {
          ui->mus7m4->setEnabled(1);
      }
      if (ui->mus7m4->currentText() != "-") {
          ui->mus7m5->setEnabled(1);
      }
  } else {
      ui->mus7m3->setEnabled(0);
      ui->mus7m4->setEnabled(0);
      ui->mus7m5->setEnabled(0);
    }
}
void klatschui::on_mus7m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus7m4->setEnabled(1);
      if (ui->mus7m4->currentText() != "-") {
          ui->mus7m5->setEnabled(1);
      }
  } else {
      ui->mus7m4->setEnabled(0);
      ui->mus7m5->setEnabled(0);
    }
}
void klatschui::on_mus7m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus7m5->setEnabled(1);
  }else {
      ui->mus7m5->setEnabled(0);
    }
}
void klatschui::on_mus8m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus8m2->setEnabled(1);
        if (ui->mus8m2->currentText() != "-") {
            ui->mus8m3->setEnabled(1);
        }
        if (ui->mus8m3->currentText() != "-") {
            ui->mus8m4->setEnabled(1);
        }
        if (ui->mus8m4->currentText() != "-") {
            ui->mus8m5->setEnabled(1);
        }
      } else {
        ui->mus8m2->setEnabled(0);
        ui->mus8m3->setEnabled(0);
        ui->mus8m4->setEnabled(0);
        ui->mus8m5->setEnabled(0);
      }
}
void klatschui::on_mus8m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus8m3->setEnabled(1);
      if (ui->mus8m3->currentText() != "-") {
          ui->mus8m4->setEnabled(1);
      }
      if (ui->mus8m4->currentText() != "-") {
          ui->mus8m5->setEnabled(1);
      }
  } else {
      ui->mus8m3->setEnabled(0);
      ui->mus8m4->setEnabled(0);
      ui->mus8m5->setEnabled(0);
    }
}
void klatschui::on_mus8m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus8m4->setEnabled(1);
      if (ui->mus8m4->currentText() != "-") {
          ui->mus8m5->setEnabled(1);
      }
  } else {
      ui->mus8m4->setEnabled(0);
      ui->mus8m5->setEnabled(0);
    }
}
void klatschui::on_mus8m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus8m5->setEnabled(1);
  }else {
      ui->mus8m5->setEnabled(0);
    }
}
void klatschui::on_mus9m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus9m2->setEnabled(1);
        if (ui->mus9m2->currentText() != "-") {
            ui->mus9m3->setEnabled(1);
        }
        if (ui->mus9m3->currentText() != "-") {
            ui->mus9m4->setEnabled(1);
        }
        if (ui->mus9m4->currentText() != "-") {
            ui->mus9m5->setEnabled(1);
        }
      } else {
        ui->mus9m2->setEnabled(0);
        ui->mus9m3->setEnabled(0);
        ui->mus9m4->setEnabled(0);
        ui->mus9m5->setEnabled(0);
      }
}
void klatschui::on_mus9m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus9m3->setEnabled(1);
      if (ui->mus9m3->currentText() != "-") {
          ui->mus9m4->setEnabled(1);
      }
      if (ui->mus9m4->currentText() != "-") {
          ui->mus9m5->setEnabled(1);
      }
  } else {
      ui->mus9m3->setEnabled(0);
      ui->mus9m4->setEnabled(0);
      ui->mus9m5->setEnabled(0);
    }
}
void klatschui::on_mus9m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus9m4->setEnabled(1);
      if (ui->mus9m4->currentText() != "-") {
          ui->mus9m5->setEnabled(1);
      }
  } else {
      ui->mus9m4->setEnabled(0);
      ui->mus9m5->setEnabled(0);
    }
}
void klatschui::on_mus9m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus9m5->setEnabled(1);
  }else {
      ui->mus9m5->setEnabled(0);
    }
}
void klatschui::on_mus10m1_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
      if (arg1 != "-") {
        ui->mus10m2->setEnabled(1);
        if (ui->mus10m2->currentText() != "-") {
            ui->mus10m3->setEnabled(1);
        }
        if (ui->mus10m3->currentText() != "-") {
            ui->mus10m4->setEnabled(1);
        }
        if (ui->mus10m4->currentText() != "-") {
            ui->mus10m5->setEnabled(1);
        }
      } else {
        ui->mus10m2->setEnabled(0);
        ui->mus10m3->setEnabled(0);
        ui->mus10m4->setEnabled(0);
        ui->mus10m5->setEnabled(0);
      }
}
void klatschui::on_mus10m2_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus10m3->setEnabled(1);
      if (ui->mus10m3->currentText() != "-") {
          ui->mus10m4->setEnabled(1);
      }
      if (ui->mus10m4->currentText() != "-") {
          ui->mus10m5->setEnabled(1);
      }
  } else {
      ui->mus10m3->setEnabled(0);
      ui->mus10m4->setEnabled(0);
      ui->mus10m5->setEnabled(0);
    }
}
void klatschui::on_mus10m3_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus10m4->setEnabled(1);
      if (ui->mus10m4->currentText() != "-") {
          ui->mus10m5->setEnabled(1);
      }
  } else {
      ui->mus10m4->setEnabled(0);
      ui->mus10m5->setEnabled(0);
    }
}
void klatschui::on_mus10m4_currentTextChanged(const QString &arg1) /** Aktualisiert die GUI beim Reiter Muster. */
{
  if (arg1 != "-") {
      ui->mus10m5->setEnabled(1);
  }else {
      ui->mus10m5->setEnabled(0);
    }
}

/* Geräte-Tab */
void klatschui::on_GerSave_clicked() /** Getriggert beim Klick auf "GerSave". Löst Speicherung aller Geräte aus. */
{
  gerSaveAll();
}

/* Steuerung-Tab */
void klatschui::on_steuGerEin_0_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe0(1);
    setLampenMode(0,1);
}
void klatschui::on_steuGerEin_2_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe1(1);
    setLampenMode(1,1);
}
void klatschui::on_steuGerEin_4_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe2(1);
    setLampenMode(2,1);
}
void klatschui::on_steuGerEin_5_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe3(1);
    setLampenMode(3,1);
}
void klatschui::on_steuGerEin_6_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe4(1);
    setLampenMode(4,1);
}
void klatschui::on_steuGerEin_7_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe5(1);
    setLampenMode(5,1);
}
void klatschui::on_steuGerEin_8_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe6(1);
    setLampenMode(6,1);
}
void klatschui::on_steuGerEin_9_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe7(1);
    setLampenMode(7,1);
}
void klatschui::on_steuGerEin_10_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe8(1);
    setLampenMode(8,1);
}
void klatschui::on_steuGerEin_20_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe9(1);
    setLampenMode(9,1);
}
void klatschui::on_steuGerEin_21_clicked() /** Getriggert beim Klicken auf "steuGerEin_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
    changeLampe10(1);
    setLampenMode(10,1);
}
void klatschui::on_steuGerAus_0_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe0(0);
  setLampenMode(0,0);
}
void klatschui::on_steuGerAus_2_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe1(0);
  setLampenMode(1,0);
}
void klatschui::on_steuGerAus_4_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe2(0);
  setLampenMode(2,0);
}
void klatschui::on_steuGerAus_5_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe3(0);
  setLampenMode(3,0);
}
void klatschui::on_steuGerAus_6_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe4(0);
  setLampenMode(4,0);
}
void klatschui::on_steuGerAus_7_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe5(0);
  setLampenMode(5,0);
}
void klatschui::on_steuGerAus_8_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe6(0);
  setLampenMode(6,0);
}
void klatschui::on_steuGerAus_9_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe7(0);
  setLampenMode(7,0);
}
void klatschui::on_steuGerAus_10_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe8(0);
  setLampenMode(8,0);
}
void klatschui::on_steuGerAus_20_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe9(0);
  setLampenMode(9,0);
}
void klatschui::on_steuGerAus_21_clicked() /** Getriggert beim Klicken auf "steuGerAus_*". Aktualisiert GUI bei Lampenliste. Sendet an Arduino. */
{
  changeLampe10(0);
  setLampenMode(10,0);
}
void klatschui::changeLampe0(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_0->setDisabled(0);
      ui->steuGerEin_0->setDisabled(1);
      ui->steuGerStatus_0->setText("🌕");
  } else {
      ui->steuGerAus_0->setDisabled(1);
      ui->steuGerEin_0->setDisabled(0);
      ui->steuGerStatus_0->setText("🌑");
  }
}
void klatschui::changeLampe1(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_2->setDisabled(0);
      ui->steuGerEin_2->setDisabled(1);
      ui->steuGerStatus_2->setText("🌕");
  } else {
      ui->steuGerAus_2->setDisabled(1);
      ui->steuGerEin_2->setDisabled(0);
      ui->steuGerStatus_2->setText("🌑");
  }
}
void klatschui::changeLampe2(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_4->setDisabled(0);
      ui->steuGerEin_4->setDisabled(1);
      ui->steuGerStatus_4->setText("🌕");
  } else {
      ui->steuGerAus_4->setDisabled(1);
      ui->steuGerEin_4->setDisabled(0);
      ui->steuGerStatus_4->setText("🌑");
  }
}
void klatschui::changeLampe3(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_5->setDisabled(0);
      ui->steuGerEin_5->setDisabled(1);
      ui->steuGerStatus_5->setText("🌕");
  } else {
      ui->steuGerAus_5->setDisabled(1);
      ui->steuGerEin_5->setDisabled(0);
      ui->steuGerStatus_5->setText("🌑");
  }
}
void klatschui::changeLampe4(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_6->setDisabled(0);
      ui->steuGerEin_6->setDisabled(1);
      ui->steuGerStatus_6->setText("🌕");
  } else {
      ui->steuGerAus_6->setDisabled(1);
      ui->steuGerEin_6->setDisabled(0);
      ui->steuGerStatus_6->setText("🌑");
  }
}
void klatschui::changeLampe5(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_7->setDisabled(0);
      ui->steuGerEin_7->setDisabled(1);
      ui->steuGerStatus_7->setText("🌕");
  } else {
      ui->steuGerAus_7->setDisabled(1);
      ui->steuGerEin_7->setDisabled(0);
      ui->steuGerStatus_7->setText("🌑");
  }
}
void klatschui::changeLampe6(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_8->setDisabled(0);
      ui->steuGerEin_8->setDisabled(1);
      ui->steuGerStatus_8->setText("🌕");
  } else {
      ui->steuGerAus_8->setDisabled(1);
      ui->steuGerEin_8->setDisabled(0);
      ui->steuGerStatus_8->setText("🌑");
  }
}
void klatschui::changeLampe7(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_9->setDisabled(0);
      ui->steuGerEin_9->setDisabled(1);
      ui->steuGerStatus_9->setText("🌕");
  } else {
      ui->steuGerAus_9->setDisabled(1);
      ui->steuGerEin_9->setDisabled(0);
      ui->steuGerStatus_9->setText("🌑");
  }
}
void klatschui::changeLampe8(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_10->setDisabled(0);
      ui->steuGerEin_10->setDisabled(1);
      ui->steuGerStatus_10->setText("🌕");
  } else {
      ui->steuGerAus_10->setDisabled(1);
      ui->steuGerEin_10->setDisabled(0);
      ui->steuGerStatus_10->setText("🌑");
  }
}
void klatschui::changeLampe9(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_20->setDisabled(0);
      ui->steuGerEin_20->setDisabled(1);
      ui->steuGerStatus_20->setText("🌕");
  } else {
      ui->steuGerAus_20->setDisabled(1);
      ui->steuGerEin_20->setDisabled(0);
      ui->steuGerStatus_20->setText("🌑");
  }
}
void klatschui::changeLampe10(int LampenZustand) /** Aktualisiert Lampenstatus auf der GUI. */
{
  if (LampenZustand) {
      ui->steuGerAus_21->setDisabled(0);
      ui->steuGerEin_21->setDisabled(1);
      ui->steuGerStatus_21->setText("🌕");
  } else {
      ui->steuGerAus_21->setDisabled(1);
      ui->steuGerEin_21->setDisabled(0);
      ui->steuGerStatus_21->setText("🌑");
  }
}
void klatschui::setLampenMode(int LampenId, int LampenZustand) /** Ändert Zustand der Lampe auf dem Arduino. */
{
  QString send = "updateLampe" + QString::number(LampenId) + "~" + QString::number(LampenZustand);
  writeArduinoData(send);
}

/* Konfiguration-Tab */
void klatschui::on_configIntAufnahme_valueChanged() /** Getriggert durch Button. Aktualisiert GUI. */
{
    changeAufnahme(ui->configIntAufnahme->value());
}
void klatschui::on_configIntSchwelle_valueChanged()/** Getriggert durch Button. Aktualisiert GUI. */
{
    changeSchwelle(ui->configIntSchwelle->value());
}
void klatschui::on_configIntStille_valueChanged() /** Getriggert durch Button. Aktualisiert GUI. */
{
    changeStille(ui->configIntStille->value());
}
void klatschui::on_configIntToleranz_valueChanged() /** Getriggert durch Button. Aktualisiert GUI. */
{
    changeToleranz(ui->configIntToleranz->value());
}
void klatschui::on_configIntAufnahmeDef_clicked() /** Getriggert durch Button. Aktualisiert GUI. */ /** Default Aufnahmewert */
{
    changeAufnahme(STD_AUFNAHME);
}
void klatschui::on_configIntSchwelleDef_clicked() /** Getriggert durch Button. Aktualisiert GUI. */ /** Default Schwellenwert */
{
    changeSchwelle(STD_SCHWELLE);
}
void klatschui::on_configIntStilleDef_clicked() /** Getriggert durch Button. Aktualisiert GUI. */ /** Default Stille-Wert */
{
    changeStille(STD_STILLE);
}
void klatschui::on_configIntToleranzDef_clicked() /** Getriggert durch Button. Aktualisiert GUI. */ /** Default Toleranzwert */
{
    changeToleranz(STD_TOLERANZ);
}
void klatschui::configResetAll() /** Führt die Funktionen zum ändern der Config-Variablen aus. */
{
    changeAufnahme(STD_AUFNAHME);
    changeSchwelle(STD_SCHWELLE);
    changeStille(STD_STILLE);
    changeToleranz(STD_TOLERANZ);
}
void klatschui::on_configPinSound_currentTextChanged(const QString &arg1) /** Getriggert durch Feldänderung. Aktualisiert GUI, sendet an Aurduino. */
{
    changeSound(arg1);
}
void klatschui::on_configPinPieper_currentTextChanged(const QString &arg1) /** Getriggert durch Feldänderung. Aktualisiert GUI, sendet an Aurduino. */
{

    changePieper(arg1.toInt());
}
void klatschui::on_configPinAktualisierenBtn_clicked() /** Signal an SPL. PortListeAktualisieren() reagiert */
{
    emit AvailablePorts();
}

void klatschui::on_pushButton_pressed() /** Getriggert durch Button halten. Sendet Startbefehl an Arduino, Sensorwerte zu übermitteln. */
{
    writeToArduino("ConfigSoundStart");
}

void klatschui::on_pushButton_released() /** Getiggert durch Button loslassen. Sendet Stopbefehl an Arduino, Sensorwerte nicht mehr zu senden. */
{
    writeToArduino("ConfigSoundStop");
    ui->configSoundWert->setText("");
}

void klatschui::on_pushButton_2_clicked() /** Simuliert Nachricht vom Arduino, dass dieser Empfangsbereit ist. Manchmal wird dieses Signal des Arduinos vom SPL nicht korrekt erkannt. Daher würden keine weiteren Befehler an den Arduino geschickt werden. Dies überbrückt der Knop. */
{
    emit fixProcessed();
}
