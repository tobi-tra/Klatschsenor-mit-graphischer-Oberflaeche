/****************************
 **                        **
 **    Klatschsensor       **
 **                        **
 **    Christian Mundt     **
 **    Louis Kniefs        **
 **    Tobias Trautmann    ** 
 **                        **
 ****************************/

#include <stdint.h>
//#include "einstellvariablen.h"  // In der h-Datei lassen sich die Anschlusspins sowie Schwell- und Toleranzwerte einstellen

/**   Funktionen   **/
int  check_config();
void schalten(uint8_t musterid);
void auswertung(uint8_t signale[6]);
void auswertung_vorbereiten();
void zeit(uint8_t zustand);
//vllt noch mehr funktionen vorhanden


/**   Variablen    **/
int16_t MAX_STILLE, TOLERANZ = -1; // Wert Max_Stille und Toleranz
int8_t        SOUNDSENSOR, PIEPER, AUFNAHMEZEIT, SCHWELLE    = -1;   // Anschlusspin des Soundsensors,Anschlusspin des Piepers, Wert Aufnahmezeit, Wert Schwelle
boolean       eingang, loaded_config, ende, just_shown = 0;
// Pegel-Startzeit, Stille-Startzeit, Stille-Endzeit, Stille-Differenz (=Stille-Zeit)
unsigned long p_s, s_s, s_e      = 0;    // Zeitmessungsvariablen auf 0 setzen; long ermöglich 23,1 Tage Laufzeit
unsigned int  toleranzbereich, s_z, checksum = 0;     // Stille-Zeit und Checksum auf 0 setzen
const uint8_t sechser_size = 6;         // Anzahl der Elemnte der folgenden zwei Arrays
unsigned int  zeiten[sechser_size];  // Zeiten-Array 
uint8_t       signale[sechser_size];      // Signale-Array
const uint8_t k = 2;                // kurze Pause zwischen Klatschern
const uint8_t l = 3;                // lange Pause zwischen Klatschern
const char *  meldung[3] = {"aus.","an.","invertiert."};
boolean       debug = 1; // war das mal =0?

struct Geraet {                     // Struct für eingespeicherte Geräte
  uint8_t pin;                // Anschlusspin
  boolean zustand;                  // An = 1 oder aus = 0
};

uint8_t muster_size, geraete_size, delimName, delimMuster, delimGeraete, delimAction, delimTilde, delimKomma, currentCount = 0;
String input, inputName, inputMuster, inputGeraete, inputAction, inputEinGeraet,inputExistierendeGeraete = "";

struct Geraet geraete[] = {};

struct Muster {                     // Struct für eingespeicherte Muster
  char* titel;                // Titel für die Ausgabe
  uint8_t rhythmus[6];        // Klatschrhythmus
  uint8_t geraete_ids[12];    // Angesteuerte Geräte, maximal 12, da mit dieser Arduino-Verion nicht mehr Pins vorhanden sind
  uint8_t action;             // 0 = aus, 1 = an, 2 = toggle;
  uint8_t geraete_count;            // Im Setup wird die Anzahl der angesteuerten Geräte gezählt und hier gespeichert
  uint8_t checksum;                     // Im Setup wird aus dem gespeicherten Rhythmus eine Checksum gebildet und gespeichert
};

struct Muster muster[10] = {};

void setup() {
  Serial.begin(9600);                                         // Serielle Verbindung initiieren
  //pinMode(SOUNDSENSOR, INPUT);                                // Soundsensor als Eingang definieren
  //pinMode(CONFIG, INPUT);                                     // Konfigurations-Taster als Eingang definieren
  //pinMode(PIEPER, OUTPUT);                                    // Pieper als Ausgang definieren
  //for (uint8_t i = 0; i < geraete_size; i++) {                // Vorhandene Geräte als Ausgänge definieren
  //  pinMode(geraete[i].pin, OUTPUT);
  //}

  /*for (uint8_t j = 0; j < muster_size; j++) {                 // Checksumme jedes einzelnen Musters bilden und im Muster-Struct speichern
    //muster[j].checksum = 1*muster[j].rhythmus[0]+8*muster[j].rhythmus[1]+27*muster[j].rhythmus[2]+125*muster[j].rhythmus[3]+343*muster[j].rhythmus[4]+1331*muster[j].rhythmus[5];
    uint8_t count = 1;                                 // Angesteuerte Geräte zählen
    for (uint8_t i = 1; i < geraete_size && (muster[j].geraete_ids[i] != muster[j].geraete_ids[i+1]); i++) { // Anzahl der Geräte pro Muster ermitteln. Annahmme: Mindestens ein Gerät vorhanden, Gerät "0" muss zuerst genannt werden
        count++;
    }
    muster[j].geraete_count = count; // Anzahl der Geräte pro Muster in Struct-Muster speichern
  }*/
   
  Serial.println(F("setGeraete3,1~91,1~5,1\nsetLampeHaakeBeck~kkk~0,9,3,11~2\nSollte der Sensor nicht richtig reagieren, druecke den\nKonfigurationstaster und veraendere die Stellschraube am Sensor.\nEin hoeherer Wert bedeutet eine hoehere Empfindlichkeitsstufe.\n\nEs sind folgende Muster gespeichert:\n\nID  Muster____________________________  Geraete_______  Aktion_____\n"));
  for (uint8_t i = 0; i < muster_size; i++) {
    
    Serial.print(i);Serial.print(F(")  "));
    for (uint8_t j = 0; j < sechser_size; j++) {
      const char * text[4] = {"      ","","kurz  ","lang  "};
      Serial.print(text[muster[i].rhythmus[j]]);
    }
    Serial.print(muster[i].geraete_ids[0]+1);
    if (muster[i].geraete_ids[1] != 0) {
      Serial.print(F(", "));
    } else {
      Serial.print(F("   "));
    }
    for (uint8_t j = 1; j < geraete_size; j++) {
      if (muster[i].geraete_ids[j] != 0) {
        Serial.print(muster[i].geraete_ids[j]+1);
      }
      if (muster[i].geraete_ids[j+1] != 0) {
        Serial.print(F(", "));
      } else {
        Serial.print(F("   "));
      }
    }
    const char * text[3] = {"ausschalten","einschalten","invertieren"};
    Serial.print(text[muster[i].action]);
    Serial.println(F("\n"));
  }
  Serial.println(F("\nPinbelegung: Geraet -> Pin\n"));
  for (uint8_t i = 0; i < geraete_size; i++) {
    Serial.print(i+1);
    Serial.print(F(" -> "));
    Serial.println(geraete[i].pin);
  }
  delay(500);
  Serial.println(F("\nBereit.\n\n\n"));

  
}

void loop() {
  if (Serial.available() > 0) {
      
    input = Serial.readStringUntil(10); // 10 = linefeed
    
    Serial.println(input);
    
    if (input.substring(0,16) == "setPinModePieper")
    { //Befehl zum Konfigurieren sieht dann so aus: setPinModePieper6 -> macht Pin 6 zum Output
          //Überprüfen, ob wirklich eine Nummer, bzw. lesen, was .toInt macht, falls es keine nummer ist
          Serial.println(input.substring(16,18));
          PIEPER = input.substring(16,18).toInt();
          pinMode(PIEPER, OUTPUT);
          Serial.println("processed");
    }
      else if (input.substring(0,15) == "setPinModeSound")
    {
          //Überprüfen, ob wirklich eine Nummer, bzw. lesen, was .toInt macht, falls es keine nummer ist: Antwort: .toInt macht garnichts!
          Serial.println(input.substring(15,17));
          SOUNDSENSOR = input.substring(15,17).toInt();
          pinMode(SOUNDSENSOR, OUTPUT);
          Serial.println("processed");
    }
      else if (input.substring(0,14) == "setIntAufnahme")
    {
          //
          Serial.println(input.substring(14,17));
          AUFNAHMEZEIT = input.substring(14,17).toInt();
          Serial.println("processed");
    }
      else if (input.substring(0,14) == "setIntSchwelle")
    {
          //
          Serial.println(input.substring(14,16));
          SCHWELLE = input.substring(14,16).toInt();
          Serial.println("processed");
    }
      else if (input.substring(0,12) == "setIntStille")
    {
          //
          Serial.println(input.substring(12,13));
          MAX_STILLE = input.substring(12,13).toInt()*1000;
          Serial.println("processed");
    }
      else if (input.substring(0,14) == "setIntToleranz")
    {
          //
          Serial.println(input.substring(14,17));
          TOLERANZ = input.substring(14,17).toInt();
          Serial.println("processed");
          
    }  
      else if (input.substring(0,8) == "setLampe") //.readString
    {
      // {"Haake Beck",     {k,k,k},    {0},           2,        0, 0},
      // HaakeBeck~kkk~0,9,3,11~2
      delimName        = input.indexOf('~', 0);
      delimMuster      = input.indexOf('~', delimName+1);
      delimGeraete     = input.indexOf('~', delimMuster+1);
      delimAction      = input.indexOf('~', delimGeraete+1);
      
      inputName     = input.substring(8, delimName);
      inputMuster   = input.substring(delimName+1, delimMuster);
      inputGeraete  = input.substring(delimMuster+1, delimGeraete);
      inputAction   = input.substring(delimGeraete+1, delimAction);

      muster[muster_size].titel = const_cast<char*>(inputName.c_str());
                   
      for (int i = 0; i < inputMuster.length(); i++) {
        if (inputMuster.charAt(i) == 'k') {
          muster[muster_size].rhythmus[i] = k;
        } else if (inputMuster.charAt(i) == 'l'){
          muster[muster_size].rhythmus[i] = l;
        } else {
          //Error meldung
        }
      }  
      
      currentCount = 0;
      uint8_t i = 0;
      delimKomma = 0;
      while (i < inputGeraete.length()) {
        delimKomma = inputGeraete.indexOf(',', i);
        muster[muster_size].geraete_ids[currentCount] = inputGeraete.substring(i, delimKomma).toInt();
        i = delimKomma+1;
        if (delimKomma == -1) {
          break;
        }
        currentCount++;
      }
      muster[muster_size].geraete_count = currentCount;
      currentCount = 0;
      i = 0;
      delimKomma = 0;
      
      muster[muster_size].action = inputAction.toInt();

      muster[muster_size].checksum = 1*muster[muster_size].rhythmus[0]+8*muster[muster_size].rhythmus[1]+27*muster[muster_size].rhythmus[2]+125*muster[muster_size].rhythmus[3]+343*muster[muster_size].rhythmus[4]+1331*muster[muster_size].rhythmus[5];
      
      /*
      Serial.println(muster[muster_size].titel);
      Serial.print(muster[muster_size].rhythmus[0]);
      Serial.print(muster[muster_size].rhythmus[1]);
      Serial.print(muster[muster_size].rhythmus[2]);
      Serial.print(muster[muster_size].rhythmus[3]);
      Serial.print(muster[muster_size].rhythmus[4]);
      Serial.print(muster[muster_size].rhythmus[5]);
      Serial.println("");
      Serial.print("CurrentCount: ");Serial.println(muster[muster_size].geraete_count);
      Serial.print(muster[muster_size].geraete_ids[0]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[1]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[2]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[3]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[4]);Serial.print(" ");
      Serial.println("");
      Serial.println(muster[muster_size].action);
      Serial.println(muster[muster_size].geraete_count);
      Serial.println(muster[muster_size].checksum);
      */
      muster_size++;
      Serial.println("processed");
      
    } else if (input.substring(0,10) == "setGeraete") {

      
      inputExistierendeGeraete = input.substring(10);

      for (int i = 0; i < geraete_size; i++) { // clear gerätespeicher
          geraete[i].pin = 0;
          geraete[i].zustand = 0;
      }
      
      geraete_size = 0;
      uint8_t i = 0;
      delimTilde = 0;
      delimKomma = 0;
      inputEinGeraet = "";
      while (i < inputExistierendeGeraete.length()) {
        //vllt hier daten nur zwischenspeichern, auf fehler überprüfen und erst am ende speichern
        delimTilde = inputExistierendeGeraete.indexOf('~', i);
        
        inputEinGeraet = inputExistierendeGeraete.substring(i, delimTilde);
          delimKomma = inputEinGeraet.indexOf(',', 0);

          geraete[geraete_size].pin = inputEinGeraet.substring(0, delimKomma).toInt();
          
          //Serial.print("pin ");Serial.println(geraete[geraete_size].pin);
          
          if (inputEinGeraet.substring(delimKomma+1) == "1") {
            geraete[geraete_size].zustand = 1;
          } else if (inputEinGeraet.substring(delimKomma+1) == "0") {
            geraete[geraete_size].zustand = 0;
          } else {
            // Send error  
          }
          pinMode(geraete[geraete_size].pin, OUTPUT);
         digitalWrite(geraete[geraete_size].pin, geraete[geraete_size].zustand); 
          
        i = delimTilde+1;
        
        geraete_size++;
        //Serial.println(geraete_size);
        if (delimTilde == -1) {
          break;
        }
      }
      //Serial.println(geraete_size);
      //for (int i = 0; i < geraete_size; i++) {
      //Serial.print("Pin: ");Serial.println(geraete[i].pin);
      //Serial.print("Zustand: ");Serial.println(geraete[i].zustand);
      //}
      Serial.println("processed");
    }
    
    check_config();
  }
 
  if (!loaded_config) { //Falls keine Konfiguration geladen wurde: Auf neue Eingabe warten.
    //Serial.println("Error01:Keine Konfiguration gefunden");
    if (!just_shown) {
      Serial.println(F("Noch fehlen Daten."));
      just_shown = 1;
    }
    return;
  } else {
    if (!just_shown) {
      Serial.println(F("Super, jetzt ist alles soweit!"));
      just_shown = 0;
    }
    
    return;
  }
  
  unsigned long aufnahme_start = millis();
  uint8_t differenz = 0;
  uint16_t max_sound = 0;
  uint16_t min_sound = 1024;
  
  while (millis()-aufnahme_start < AUFNAHMEZEIT) {  // Innerhalb einer Aufnahmezeit von 40ms wird der maximale und der minimale Sensorwert ermittelt 
    uint8_t sensorWert = analogRead(SOUNDSENSOR);
    if (sensorWert > max_sound) {
      max_sound = sensorWert;
    } else if (sensorWert < min_sound) {
      min_sound = sensorWert;
    }
  }
  differenz = max_sound - min_sound; // Aus Maximal- und Minimalwert wird die Differenz gebildet
  
  if (differenz > SCHWELLE) {        // Pegel -> Relative Lautstärke (Differenz) größer als angegebener Schwellwert
    if (debug == 1) {
      Serial.println(F("In Pegel"));
      debug = 0;
      //lcd.clear();                   // Als Erkennung dafür, dass das Programm einen Pegel erkennt, geht die Hintergrundbeleuchtung des LC-Displays aus
      //lcd.noBacklight();
    } 
    if (eingang == 0) {
      zeit(1);
      eingang = 1;
    }
    if (eingang && s_z > 0) {
       speichern(s_z);
       s_z = 0;
    }  
    ende = 0;
  }

  if (differenz <= SCHWELLE) {       // Stille -> Relative Lautstärke (Differenz) kleiner als angegebenr Schwellwert
    if (debug == 0) {
      debug = 1;
    }
 
    if (eingang == 1) {
      //lcd.clear();                   // Hintergrundbeleuchtung des LC-Displays geht nach einem Klatscher wieder an
      //lcd.backlight();
      zeit(0);
      eingang = 0;
    }
    if (!ende && p_s > 0 && (millis()-s_s) > MAX_STILLE) { // Abbruch/Auswertung (Maximale Stille von 2s wurde überschritten) 
      auswertung_vorbereiten();
    }
  }
  
}

void zeit(uint8_t zustand) {
  if (zustand) {
    p_s = s_e = millis();     // Zeitpunkt des Pegelstarts = Zeitpunkt Ende der Stille = aktuelle Zeit
    s_z = 0;                  // Stillezeit wird zurückgesetzt
    if (s_s != 0) {           // Bei einer Stille
      s_z = s_e - s_s;        // Stillezeit wird berechnet
    }
  }
  if (!zustand) {             // Wenn kein Pegel vorhanden ist
    s_s = millis();
  }
}

void speichern(int zeit) {    // Speichern in Zeiten-Array
  for (uint8_t i = 0; i < sechser_size; i++) {
    if (zeiten[i]==0) {
      zeiten[i] = zeit;       // Stillezeit wird in das Zeiten-Array geschrieben
      if ((i+1 ) == sechser_size) { // Falls letzer Klatscher gespeichert, beginne auszuwerten 
        Serial.print(F("Vorzeitiger Auswertungsbeginn, da maxiamale Anzahl an Klatschern erreicht. \n"));
        //lcd.clear();
        //lcd.backlight();      // Hintergrundbeleuchtung des LC-Displays geht nach letztem Klatscher wieder an
        auswertung_vorbereiten();
      }
      break;
    }
  }
}

void auswertung_vorbereiten() { // Auswertung der Signale (kurz oder lang)
  unsigned long kl = (MAX_STILLE*1,1);
  unsigned long gr = 0;
  
  for (uint8_t i = 0; i < sechser_size; i++) { 
    if (zeiten[i] == 0) { // Wenn das Zeiten-Array an der Stelle leer ist, springt die Funktion an der Stelle raus 
      break;
    }
    if (zeiten[i] < kl) { // Kleinste Zeit wird ermittelt
      kl = zeiten[i];
    }
    if (zeiten[i] > gr) { // Größte Zeit wird ermittelt
      gr = zeiten[i];
    }
  }
  
  if ((gr-kl) < TOLERANZ) { // direkt alles auf "kurz" setzen (-> Signale gleich lang)
    for (uint8_t i = 0; i < sechser_size; i++) {  
      if (zeiten[i] == 0) { // Wenn das Zeiten-Array an der Stelle leer ist, springt die Funktion an der Stelle raus
        break;
      }
      signale[i] = k; // Alle Signale werden im Signal-Array als "kurz" geschrieben
    }
  } else {
    toleranzbereich = (gr-kl)/2; // Toleranzbereich wird ermittelt
    
    for (uint8_t i = 0; i < sechser_size; i++) { // Signalwerte in neues Array schreiben
      if (zeiten[i] == 0) { // Wenn das Zeiten-Array an der Stelle leer ist, springt die Funktion an der Stelle raus
        break;
      }
      if (zeiten[i] > (gr-toleranzbereich)){ // langes Signal
        signale[i] = l;
        continue;
      }
      if (zeiten[i] < (kl+toleranzbereich)){ // kurzes Signal
        signale[i] = k;
        continue;
      }
    }
  }
  auswertung(signale);
}  

void auswertung(uint8_t signale[6]) {
  uint8_t id = 0; // 200 Muster werden nicht erreicht. 200 dient zur Fehlererkennung.
  boolean fehler = 1;
  
  checksum = 1*signale[0]+8*signale[1]+27*signale[2]+125*signale[3]+343*signale[4]+1331*signale[5]; // Checksumme des erhaltenen Musters wie oben bilden
  for (uint8_t j = 0; j < muster_size; j++) {
    if (muster[j].checksum == checksum) { // Checksumme vergleichen statt jedes Element einzeln, um die Operationen zu veringern und eine schnellere Laufzeit zu schaffen
      schalten(j);  
      fehler = 0;
      break;
    }
  }
  if (fehler) { // Falls kein Muster erkannt wurde, wird eine Fehlermeldung auf dem seriellen Monitor sowie auf dem LC-Display ausgegeben
    Serial.println(F("Fehler: Muster nicht erkannt."));
    Serial.println(F("Da hat jemand keinen Rhythmus!"));
    digitalWrite(PIEPER,1); // Zusätzlich akustisches Signal, dass Muster nicht erkannt
    delay(20);
    digitalWrite(PIEPER,0);
    delay(50); // Damit der Pieper den Sensor nicht erneut auslöst
  }
  
  // Alle Variablen auf ihren Ausgangswert setzen
  ende = 1;
  eingang = 0;
  p_s = 0;
  s_z = s_s = s_e = 0;
  for(uint8_t i = 0; i < sechser_size; i++) { // Speicher leeren
    zeiten[i] = 0;
    signale[i] = 0;
  } 
}

void schalten(uint8_t musterid) {        // Geräte werden geschaltet
  switch (muster[musterid].action) {     // Gucken, welche Action angegeben (an, aus, invertiert)
    case 2: // Ganz oben, da häufigste Action
      for (uint8_t i = 0; i < (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = !geraete[muster[musterid].geraete_ids[i]].zustand; // Jeweilige Geräte invertieren. Nicht mehr TILDE, da 0 zu = -1 wurde
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand); // Pin auf HIGH oder LOW setzen
      }
      break;
    case 0: // An zweiter Stelle, da Action "0" am zweit-häufigsten verwendet wird
      for (uint8_t i = 0; i < (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = 0; // Jeweilige Geräte ausschalten
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand);
      }
      break;
    case 1: // An lezter Stelle, da am wenigsten auftretend
      for (uint8_t i = 0; i < (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = 1; // Jeweilige Geräte einschalten
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand);
      }
      break;
    default: // Bei falscher Eingabe einer Action erscheint eine Fehlermeldung, nur durch seriellen Modus möglich
      Serial.println(F("Fehler: \"action\" nicht bekannt. In Daten ändern."));
      return;
  }
  // Gerätename und Zustand wird auf dem LC-Display ausgegeben
  Serial.print(muster[musterid].titel);
  if (muster[musterid].geraete_count == 1) { // Pluralunterscheidungen
    Serial.print(F(" ist "));
  } else {
    Serial.print(F(" sind "));
  }
  // Je nach Action unterschiedlichen Text anzeigen
  Serial.print(meldung[muster[musterid].action]);
  Serial.println(F(""));
}

int check_config() {
  if (SOUNDSENSOR > -1 && PIEPER > -1) {
    if (SOUNDSENSOR == PIEPER) {
      //Serial.println(F("Error02:Soundsensor-Pin gleich Pieper-Pin"));
      return 0;
    }
    // Alles ok
  } else {
    Serial.println(F("Error05:Soundsensor-Pin o. Pieper-Pin o. Config-Pin leer"));
    return 0;
  }
  if (geraete_size <= 0) {
    //Serial.println(F("Error06:Keine Geräte angegeben"));
    return 0;
  }
  if (muster_size <= 0) {
    //Serial.println(F("Error07:Keine Muster angeben"));
    return 0;
  }
  Serial.println(F("Alles paletti."));
  loaded_config = 1;
  return 1;
}

