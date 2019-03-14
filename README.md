Team 66
-------

**Programm compilieren**:

  <code>./build</code>

**Server starten**:

1. Möglichkeit: binary ausführen:

  <code>./bin/server</code>

2. Möglichkeit: run-script ausführen (kompiliert automatisch):

  <code>./server_run</code>

**Client starten**:

1. Möglichkeit: binary ausführen: 

  <code>./bin/client GET [HUMIDITY|TEMPERATURE|LIGHT|DISTANCE]</code>

2. Möglichkeit: run-script ausführen (kompiliert automatisch):

  <code>./client_run [HUMIDITY|TEMPERATURE|LIGHT|DISTANCE]</code>

Sensoren
--------

- **Licht**: Analog Port 0
- **Temperatur/Humidity**: Digital Port 3
- **Ultraschall**: Digital Port 5


Troubleshooting
---------------

wenn reset LED vom GrovePi an ist:

  <code>avrdude -c gpio -p m328p</code>
