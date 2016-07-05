/****************************
 **                        **
 **    Klatschsensor UI    **
 **                        **
 **    Christian Mundt     **
 **    Louis Kniefs        **
 **    Tobias Trautmann    ** 
 **                        **
 ****************************/

#include <stdint.h>

/**   Funktionen   **/
int  check_config();
void schalten(uint8_t musterid);
void auswertung(uint8_t signale[6]);
void auswertung_vorbereiten();
void zeit(uint8_t zustand);
//vllt noch mehr Funktionen vorhanden


/**   Variablen    **/

int16_t MAX_STILLE, TOLERANZ = -1; // MAX_STILLE: Zeit in Millisekunden bis die Auswertung gestartet wird; TOLERANZ: Unterschreitet die Differenz zw. längster und kürzester Pause diesen Wert, werden die Pausen als gleich lang betrachtet
int8_t  SOUNDSENSOR, PIEPER, AUFNAHMEZEIT, SCHWELLE = -1;   // Anschlusspin von Soundsensor und Pieper; Zeit für Aufnahme und Wert für akustiche Schwelle

boolean       eingang, loaded_config, ende, just_shown, ConfigSend = 0;
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

uint8_t muster_size, geraete_size, delimGeraete, currentCount = 0;
int8_t delimName, delimMuster, delimAction, delimTilde, delimKomma = 0; ///// hier geändert
String input, inputName, inputMuster, inputGeraete, inputAction, inputEinGeraet, inputGeraetestring, pin = "";

struct Geraet geraete[11] = {};

struct Muster {                     // Struct für eingespeicherte Muster
  uint8_t rhythmus[6];              // Klatschrhythmus
  uint8_t geraete_ids[12];          // Angesteuerte Geräte, maximal 12, da mit dieser Arduino-Verion nicht mehr Pins vorhanden sind
  uint8_t action;                   // 0 = aus, 1 = an, 2 = toggle;
  uint8_t geraete_count;            // Im Setup wird die Anzahl der angesteuerten Geräte gezählt und hier gespeichert
  int checksum;                     // Im Setup wird aus dem gespeicherten Rhythmus eine Checksum gebildet und gespeichert
};

struct Muster muster[10] = {};

void setup() {
  Serial.begin(9600);                                           // Serielle Verbindung initiieren

  Serial.println(F("setPinModePieper2"));
  Serial.println(F("setPinModeSound10"));
  Serial.println(F("setIntAufnahme100"));      // 0-100
  Serial.println(F("setIntSchwelle10"));       // 0-10
  Serial.println(F("setIntStille200"));        // 0-2
  Serial.println(F("setIntToleranz1000"));     // 0-1000
  Serial.println(F("setLampeHaakeBeck~kkk~0,9,3,11~2"));
  Serial.println(F("setGeraete3,1~91,1~5,1"));
  Serial.println(F("setGeraete3,1~11,1~8,1~2,0~5,1~12,1~"));
  Serial.println(F("setGeraete11,0~8,0~3,0~"));
  Serial.println(F("processed"));

  
}

void loop() {
  if(ConfigSend)
  {
    Serial.print(F("SoundWert"));
    Serial.println(analogRead(SOUNDSENSOR));
    delay(800);
  }
  
  if (Serial.available() > 0) {
      
    input = Serial.readStringUntil(10);    // 10 = linefeed -> Zeilenende
    
    Serial.println(input);
    
    if (input.substring(0,16) == "setPinModePieper")
    { //Befehl zum Konfigurieren sieht dann so aus: setPinModePieper6 -> macht Pin 6 zum Output
          //Überprüfen, ob wirklich eine Nummer, bzw. lesen, was .toInt macht, falls es keine Nummer ist
          Serial.println(input.substring(16,18));
          PIEPER = input.substring(16,18).toInt();
          pinMode(PIEPER, OUTPUT);
          Serial.println(F("processed"));
    }
    
    else if (input.substring(0,11) == "updateLampe")
    {
      delimTilde = input.indexOf("~");
      SingleSchalten(input.substring(11,delimTilde).toInt(), input.substring(delimTilde+1).toInt());
      Serial.println(F("processed"));
    }
    
    else if (input.substring(0,17) == "clearAllMuster")
    {
      muster_size = 0;
      Serial.println(F("processed"));
    }
    
    else if (input.substring(0,15) == "setPinModeSound")
    {
          //Überprüfen, ob wirklich eine Nummer, bzw. lesen, was .toInt macht, falls es keine Nummer ist
          Serial.println(input.substring(15,17));
          SOUNDSENSOR = input.substring(15,17).toInt();
          pinMode(SOUNDSENSOR, OUTPUT);
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,14) == "setIntAufnahme")
    {
          //Einspeichern der Aufnahmezeit (0-100)
          Serial.println(input.substring(14,17));
          AUFNAHMEZEIT = input.substring(14,17).toInt();
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,14) == "setIntSchwelle")
    {
          //Einspeichern der Schwelle (0-10)
          Serial.println(input.substring(14,17));
          SCHWELLE = input.substring(14,16).toInt();
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,12) == "setIntStille")
    {
          //Einspeichern der Stille (0-2 * 1000)
          Serial.println(input.substring(12,13));
          MAX_STILLE = input.substring(12,13).toInt() * 1000;
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,14) == "setIntToleranz")
    {
          //Einspeichern der Toleranz (0-1000)
          Serial.println(input.substring(14,18));
          TOLERANZ = input.substring(14,18).toInt();
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,16) == "ConfigSoundStart")
    {
          ConfigSend = 1;
          Serial.println(F("processed"));
    }
    
      else if (input.substring(0,15) == "ConfigSoundStop")
    {
          ConfigSend = 0; 
          Serial.println(F("processed")); 
    }
    
      else if (input.substring(0,8) == "setLampe") //.readString
    {
      // HaakeBeck ~ kkk    ~ 0,9,3,11 ~ 2
      // Hilfsvariablen für die Trennung der einzelnen Komponenten:
      delimName        = input.indexOf('~', 0);
      delimMuster      = input.indexOf('~', delimName+1);
      delimGeraete     = input.indexOf('~', delimMuster+1);
      delimAction      = input.indexOf('~', delimGeraete+1);
      
      //inputName     = input.substring(8, delimName);                  // Name des Musters
      inputMuster   = input.substring(delimName+1, delimMuster);      // Muster der Lampe
      inputGeraete  = input.substring(delimMuster+1, delimGeraete);   // Angesteuerte Lampen
      inputAction   = input.substring(delimGeraete+1, delimAction);   // Aktion (An/Aus/Toggle)
         
      for (int i = 0; i < inputMuster.length(); i++) {    // Einspeichern des Musters
        if (inputMuster.charAt(i) == 'k') {
          muster[muster_size].rhythmus[i] = k;
        } else if (inputMuster.charAt(i) == 'l'){
          muster[muster_size].rhythmus[i] = l;
        } else {
          Serial.println(F("Error08: Abkuerzung fuer Muster nicht bekannt."));
        }
      }  
      
      currentCount = 0;
      uint8_t i = 0;
      delimKomma = 0;
      while (i < inputGeraete.length()) {   // Einspeichern der einzelnen Angesteuerten Lampen
        delimKomma = inputGeraete.indexOf(',', i);
        muster[muster_size].geraete_ids[currentCount] = inputGeraete.substring(i, delimKomma).toInt();
        i = delimKomma+1;
        if (delimKomma == -1) {
          break;
        }
        currentCount++;
      }
      muster[muster_size].geraete_count = currentCount;   // Anzahl aller angesteuerten Lampen
      currentCount = 0;
      i = 0;
      delimKomma = 0;
      
      muster[muster_size].action = inputAction.toInt();   // Einspeichern der Aktion

      // Bildung der Checksumme:
      muster[muster_size].checksum = 1*muster[muster_size].rhythmus[0]+8*muster[muster_size].rhythmus[1]+27*muster[muster_size].rhythmus[2]+125*muster[muster_size].rhythmus[3]+343*muster[muster_size].rhythmus[4]+1331*muster[muster_size].rhythmus[5];
      Serial.println(muster[muster_size].checksum);
      /*
      // Serial.println(muster[muster_size].titel);
      Serial.print(muster[muster_size].rhythmus[0]);
      Serial.print(muster[muster_size].rhythmus[1]);
      Serial.print(muster[muster_size].rhythmus[2]);
      Serial.print(muster[muster_size].rhythmus[3]);
      Serial.print(muster[muster_size].rhythmus[4]);
      Serial.print(muster[muster_size].rhythmus[5]);
      Serial.println(F(""));
      Serial.print(F("CurrentCount: "));Serial.println(muster[muster_size].geraete_count);
      Serial.print(muster[muster_size].geraete_ids[0]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[1]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[2]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[3]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[4]);Serial.print(" ");
      Serial.println(F(""));
      Serial.println(muster[muster_size].action);
      Serial.println(muster[muster_size].geraete_count);
      Serial.println(muster[muster_size].checksum);
      */
      muster_size++;                  // Hochzählen der Anzahl der Muster
      Serial.println(F("processed"));    // Statusmeldung OK
      
    }
    
      else if (input.substring(0,8) == "showInfo")
    { 
      muster_size = input.substring(8,9).toInt();
      Serial.print(muster[muster_size].rhythmus[0]);
      Serial.print(muster[muster_size].rhythmus[1]);
      Serial.print(muster[muster_size].rhythmus[2]);
      Serial.print(muster[muster_size].rhythmus[3]);
      Serial.print(muster[muster_size].rhythmus[4]);
      Serial.print(muster[muster_size].rhythmus[5]);
      Serial.println("");
      Serial.print(F("CurrentCount: "));Serial.println(muster[muster_size].geraete_count);
      Serial.print(muster[muster_size].geraete_ids[0]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[1]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[2]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[3]);Serial.print(" ");
      Serial.print(muster[muster_size].geraete_ids[4]);Serial.print(" ");
      Serial.println(F(""));
      Serial.println(muster[muster_size].action);
      Serial.println(muster[muster_size].geraete_count);
      Serial.println(muster[muster_size].checksum);
      Serial.println(F("processed"));
      
    }
    
      else if (input.substring(0,10) == "setGeraete")
    {
      
      inputGeraetestring = input.substring(10);   // Substring mit Pins und Zuständen
      Serial.println(inputGeraetestring);
      //Serial.print(F("Laenge:    "));Serial.println(inputGeraetestring.length());Serial.println("");

      for (int i = 0; i < geraete_size; i++) {    // Gerätespeicher leeren 
          digitalWrite(geraete[i].pin, 0);        // Beim Löschen eines Pins alle zurücksetzen
          geraete[i].pin = 0;
          geraete[i].zustand = 0;
      }

      geraete_size = 0;
      uint8_t i = 0;
      delimTilde = 0;
      delimKomma = 0;
      inputEinGeraet = "";
            
      while (i < inputGeraetestring.length()) {     // Abspeichern von Pin und Zustand
        //vllt hier Daten nur zwischenspeichern, auf Fehler überprüfen und erst am Ende speichern
        
        delimTilde = inputGeraetestring.indexOf('~',i);    // Aufteilen des Strings in "Untergeräte"
        Serial.print(F("delimTilde:        "));Serial.println(delimTilde);
        
        inputEinGeraet = inputGeraetestring.substring(i,delimTilde);
        Serial.print(F("inputEinGeraet:    "));Serial.println(inputEinGeraet);
        
        delimKomma = inputEinGeraet.indexOf(',',0);    // Aufteilen der "Untergeräte" in Pin und Zustand
        Serial.print(F("delimKomma:        "));Serial.println(delimKomma);
        
        geraete[geraete_size].pin = inputEinGeraet.substring(0,delimKomma).toInt();    // Abspeichern des Gerätepins        Hier war der Fehler, Leerzeichen vor delimKomma.
        Serial.print(F("Pin:               "));Serial.println(geraete[geraete_size].pin);

        Serial.print(F("iput.gerstr: "));Serial.println(inputGeraetestring);
        
        if (inputEinGeraet.substring(delimKomma+1,delimKomma+2) == "1") {    // Abspeichern des Zustandes
            geraete[geraete_size].zustand = 1;    // an
            Serial.print(F("Zustand:           "));Serial.println(F("an"));
        } else if (inputEinGeraet.substring(delimKomma+1,delimKomma+2) == "0") {
            geraete[geraete_size].zustand = 0;    // aus
            Serial.print(F("Zustand:           "));Serial.println(F("aus"));
        } else {
            Serial.print(F("Error09: Falscher Zustand bei Pin "));Serial.println(geraete[geraete_size].pin);
        }
        
        pinMode(geraete[geraete_size].pin, OUTPUT);
        digitalWrite(geraete[geraete_size].pin, geraete[geraete_size].zustand); 
        
        i = delimTilde + 1;
        Serial.print(F("i:                 "));Serial.println(i);
        
        geraete_size++;
        Serial.print(F("Anzahl der Geraete: "));Serial.println(geraete_size);
        
        if (delimTilde == -1) {
          break;
        }
      }
      //Serial.println(geraete_size);
      for (int i = 0; i < geraete_size; i++) {
        Serial.print(F("Pin: "));Serial.println(geraete[i].pin);
        Serial.print(F("Zustand: "));Serial.println(geraete[i].zustand);
      }
      Serial.println(F("processed"));
    }
    
      else
    {
      Serial.println(F("Error10:Befehl nicht gefunden."));
      Serial.println(F("processed"));
    }
    
    //if (!loaded_config) {
      check_config();
    //}
  }
  
 
  if (!loaded_config) { //Falls keine Konfiguration geladen wurde: Auf neue Eingabe warten.
    //Serial.println(F("Error01:Keine Konfiguration gefunden"));
    if (!just_shown) {
      Serial.println(F("Noch fehlen Daten."));
      just_shown = 1;
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
        Serial.println(F("Vorzeitiger Auswertungsbeginn, da maxiamale Anzahl an Klatschern erreicht."));
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
    //Serial.println("Da hat jemand keinen Rhythmus!");
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
      for (uint8_t i = 0; i <= (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = !geraete[muster[musterid].geraete_ids[i]].zustand; // Jeweilige Geräte invertieren. Nicht mehr TILDE, da 0 zu = -1 wurde
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand); // Pin auf HIGH oder LOW setzen
        
        Serial.print(F("statusLampe"));Serial.print(muster[musterid].geraete_ids[i]);Serial.print(F("~"));Serial.println(geraete[muster[musterid].geraete_ids[i]].zustand);   // Senden an Qt
      }
      break;
    case 0: // An zweiter Stelle, da Action "0" am zweit-häufigsten verwendet wird
      for (uint8_t i = 0; i <= (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = 0; // Jeweilige Geräte ausschalten
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand);
        
        Serial.print(F("statusLampe"));Serial.print(muster[musterid].geraete_ids[i]);Serial.print(F("~"));Serial.println(geraete[muster[musterid].geraete_ids[i]].zustand);   // Senden an Qt
      }
      break;
    case 1: // An lezter Stelle, da am wenigsten auftretend
      for (uint8_t i = 0; i <= (muster[musterid].geraete_count); i++) {
        geraete[muster[musterid].geraete_ids[i]].zustand = 1; // Jeweilige Geräte einschalten
        digitalWrite(geraete[muster[musterid].geraete_ids[i]].pin, geraete[muster[musterid].geraete_ids[i]].zustand);
        
        Serial.print(F("statusLampe"));Serial.print(muster[musterid].geraete_ids[i]);Serial.print(F("~"));Serial.println(geraete[muster[musterid].geraete_ids[i]].zustand);   // Senden an Qt
      }
      break;
    default: // Bei falscher Eingabe einer Action erscheint eine Fehlermeldung, nur durch seriellen Modus möglich
      Serial.println(F("Fehler: 'action' nicht bekannt. In Daten aendern."));
      return;
  }
}

void SingleSchalten(int geraet, int action) {
  switch (action) {     // Gucken, welche Action angegeben (an, aus, invertiert)
    case 2: // Ganz oben, da häufigste Action
        geraete[geraet].zustand = !geraete[geraet].zustand; // Jeweilige Geräte invertieren. Nicht mehr TILDE, da 0 zu = -1 wurde
        digitalWrite(geraete[geraet].pin, geraete[geraet].zustand); // Pin auf HIGH oder LOW setzen
        
        Serial.print(F("statusLampe"));Serial.print(geraet);Serial.print(F("~"));Serial.println(geraete[geraet].zustand);   // Senden an Qt
      break;
    case 0: // An zweiter Stelle, da Action "0" am zweit-häufigsten verwendet wird
        geraete[geraet].zustand = 0; // Jeweilige Geräte ausschalten
        digitalWrite(geraete[geraet].pin, geraete[geraet].zustand);
        
        Serial.print(F("statusLampe"));Serial.print(geraet);Serial.print(F("~"));Serial.println(geraete[geraet].zustand);   // Senden an Qt
      break;
    case 1: // An lezter Stelle, da am wenigsten auftretend
        geraete[geraet].zustand = 1; // Jeweilige Geräte einschalten
        digitalWrite(geraete[geraet].pin, geraete[geraet].zustand);
        
        Serial.print(F("statusLampe"));Serial.print(geraet);Serial.print(F("~"));Serial.println(geraete[geraet].zustand);   // Senden an Qt
      break;
    default: // Bei falscher Eingabe einer Action erscheint eine Fehlermeldung, nur durch seriellen Modus möglich
      Serial.println(F("Fehler: 'action' nicht bekannt. In Daten aendern."));
      return;
  }
}

int check_config() {
  if (SOUNDSENSOR > -1) {
    /*if (SOUNDSENSOR == PIPEPR) {
      //Serial.println(F("Error02:Soundsensor-Pin fehlt"));
      return 0;
    }*/
    // Alles ok
  } else {
    Serial.println(F("Error05:Soundsensor-Pin fehlt"));
    return 0;
  }
  if (geraete_size <= 0) {
    Serial.println(F("Error06:Keine Geraete angegeben"));
    return 0;
  }
  if (muster_size <= 0) {
    Serial.println(F("Error07:Keine Muster angeben"));
    return 0;
  }
  Serial.println(F("Alles paletti."));
  loaded_config = 1;
  return 1;
}

