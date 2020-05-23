# Der MiXXer (Arduino_Cocktail_NANO)

Dieses Projekt ist nur aus Spaß entstanden und dient auch nur selbigem.<br />
Es handelt sich hier um mein erstes "größeres" Arduino-Projekt was über das blinken von LEDs hinausgeht. <br />
Die Idee war eine Maschine zu kreieren die automatisiert Cocktails zubereiten kann.<br />
Eine Auswahl an verschiedenen Getränken kann über eine Android-App getätigt werden, welche die Daten kabellos per Bluetooth an den MiXXer überträgt. Somit ist eine Bedienung bequem vom Sofa garantiert.<br />
Es können eigene Rezepte aus den vorhandenen Zutaten erstellt werden und so ganz individuelle Cocktails zubereitet werden.<br />
Sirup oder dickflüssige Zutaten sollten vermieden werden.<br />

Das Herz von "Der MiXXer" besteht aus einem Arduino Nano der über ein Relay-Shield die "Peristaltic Dosierpumpen"  ansteuert.
Der ganze Aufbau ist sowohl in der Arduino-Software und in dem "Hardware-Design" leicht skalierbar, dass Der MiXXer einfach mit n-Pumpen erweitert werden kann um eine größere Auswahl an Cocktails zubereiten zukönnen. Die Füllmenge der Zutaten wird über die Laufzeit der Pumpen errechnet. Dies hat den Vorteil, dass keine zusätzliche Hardware notwendig ist die die Durchflussmenge oder das Gewicht des Getränkes misst.

Der MiXXer zeigt seinen Gerätezustand über eine RGB-LED an.
- Blau: Bereit
- Rot: Fehler
- Grün: In Zubereitung
- Weiß: Handeingriff (Eine Pumpe auf Hand EIN) 

Die Android-App kommuniziert mit "Der MiXXer" über Bluetooth.<br />
(Die App ist in einem frühen Stadium und kann noch Fehler enthalten.)<br />
Die Datenpakete via Bluetooth werden nicht auf plausibilität geprüft! (Kein CRC o.ä.)<br />

![Alt text](Media/img_1.jpg?raw=true "Title")

# Aufbau der Hardware
Bauteile:
- Arduino Nano
- Breakout Board für Arduino Nano
- 4 Relay-module
- XL4015 DC Step Down
- 4x DC 12V Peristalische Pumpe
- Silikonschlauch (für Lebensmittel)
- 12V Netzteil 120W
- Diverse Klemmen

Schematischer Aufbau der mit Fritzing realisiert wurde.<br />
(Dieses Schema entspricht nicht 100% dem tatsächlichen Aufbau, da kein Steckbrett verwendet wurde)<br />
![Alt text](Media/DerMiXXer_Steckplatine.png?raw=true "Steckplatine")


# Inbetriebnahme

Konfiguration der Pumpen in der ```void setup():```<br />

```
pumpen[n].pinPump = 17; //!Relay-Pin for this Pump!
pumpen[n].statePump = INIT_STATE;
pumpen[n].startTimePump = 0;
pumpen[n].fillInMlPump = 0;
pumpen[n].resetErrorStatePump = false;
pumpen[n].pumpMlPerMin = 500; //!Flow rate in ml/minute for this pump!
```

Um eine korrektes Mischverhältnis zu gewehrleisten muss die Durchflussmenge der verwendeten Pumpen gemessen und im Arduino-Code hinterlegt werden. Da die eBay-Pumpen in ihren tatsächlcihen Durchflussmengen nicht immer mit den angegebenen Werten übereinstimmen ist dieser Schritt sehr wichtig.<br />
<br />
Über die Konstante kann die Anzahl der Pumpen festgelegt werden die während der Zubereitung gleichzeitig laufen durfen:<br />
```
const int MAX_PUMP_AT_SAME =4;
```
<br />
Diese Einstellung ist abhängig von dem verwendeten Netzeil und der Leistungsaufnahme der Pumpen.<br />
<br />
Dieser Schritt muss nur einmalig asugeführt werden!

# Verbinden via Blutooth

Blinkt die LED des Bluetooth-Modul kann eine Verbindung mit "Der MiXXer" hergestellt werden. 
Über die Android-App kann eine Bluetooth-Verbindung mit "Der MiXXer" hergestellt werden.
Wichtig ist hierbei, dass die Kopplung über die App passiert und nicht über das "Pairing" in den Android-Bluetootheinstellungen.
# ToDo
Gehäuse für den MiXXer.
![Alt text](Media/Case.png?raw=true "Gehäuse")

