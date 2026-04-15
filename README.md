# Crossmixer

**Echtzeit Stereo → 5.1 Downmix / Router für Windows.**
Nimmt jeden Stereo-Ausgang (z.B. Spotify, Browser, Spiele) per WASAPI-Loopback auf und verteilt ihn flexibel auf alle 6 Kanäle einer 5.1-USB-Soundkarte — inklusive LFE-Boost, Preset-System und Tray-Betrieb.

Entstanden, weil Stereo-Musik auf einer 5.1-Anlage sonst nur aus zwei Lautsprechern spielt.

---

## Features

- **6-Kanal WASAPI-Output** (Shared Mode, Float32, KSAUDIO_SPEAKER_5POINT1) auf FL/FR/Center/LFE/RL/RR
- **WASAPI-Loopback-Capture** als Eingangsquelle (System-Output abgreifen ohne Kabel/Stereomix)
- **Pro-Kanal-Strip:** Volume + Mute
- **Master-Section** mit Peak-Meter und Master-Volume
- **LFE-Boost** (+10 dB) mit 120 Hz Tiefpass
- **Spektrum-Analyzer** (48 log-skalierte Bänder, 40 Hz – 16 kHz, Hann-FFT 1024) auf L+R-Summe
- **Preset-System** (Save / Save As / Delete) — letztes Preset wird beim Start geladen
- **Swap-Buttons** für FL↔FR und RL↔RR (z.B. falls Kabel vertauscht)
- **Tray-Modus** (Minimize to Tray, Autostart via Registry)
- **Device-Selector** mit persistenter Auswahl

---

## Download

Fertige Binary (Windows x64) im [Releases-Bereich](../../releases).

Einfach `Crossmixer.exe` herunterladen und starten — kein Installer nötig. Einstellungen werden unter `%AppData%\Crossmixer\` abgelegt.

---

## Systemvoraussetzungen

- Windows 10 / 11 x64
- Eine 5.1-fähige Soundkarte (entwickelt & getestet mit **C-Media CM6206 USB 5.1**)
- Bei C-Media/Xear-Treibern: **DSP-Modus deaktivieren** (7.1 Virtual und Xear Surround aus), sonst remixt der Treiber die Kanäle und individuelle Mutes sind wirkungslos

---

## Aus dem Quellcode bauen

```bash
git clone --recurse-submodules https://github.com/clrsrc/Crossmixer.git
cd Crossmixer
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Voraussetzungen: **Visual Studio 2022 Build Tools**, **CMake ≥ 3.22**, **Git**.
Die Binary landet unter `build/Crossmixer_artefacts/Release/Crossmixer.exe`.

---

## Verwendete Frameworks & Quellen

### JUCE Framework
- <https://juce.com> — Cross-Platform-C++-Framework für Audio-Apps
- <https://github.com/juce-framework/JUCE> — als Git-Submodul eingebunden
- Lizenz: AGPLv3 (siehe `JUCE/LICENSE.md`) — daher ist Crossmixer ebenfalls AGPLv3

### Windows Core Audio APIs (WASAPI)
- [Core Audio APIs in Windows Vista+](https://learn.microsoft.com/en-us/windows/win32/coreaudio/core-audio-apis-in-windows-vista)
- [Loopback Recording](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording)
- [Rendering a Stream](https://learn.microsoft.com/en-us/windows/win32/coreaudio/rendering-a-stream)
- [KSAUDIO_SPEAKER_5POINT1](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/multichannel-audio) — Kanal-Mapping

### Hardware-Referenz
- C-Media CM6206 USB Audio Controller — [Datasheet](https://www.cmedia.com.tw/products/usb20_high_speed_true_51_usb_audio/cm6206lx)

---

## Ein Wort zu Claude Code — Ehre, wem Ehre gebührt

Dieses Projekt wurde **vollständig mit [Claude Code](https://claude.com/claude-code) (Anthropic)** entwickelt.

Der Autor ist kein hauptberuflicher Softwareentwickler. Die gesamte Architektur, der C++/JUCE-Code, die WASAPI-Integration, das Preset-System, das Threading-Modell, das GUI-Design und die komplette Fehlersuche (inkl. der kniffligen Themen wie das korrekte WASAPI-Format-Handshake mit 5.1, die richtige Platzierung der Channel-Swaps **nach** Mute/Volume, und der Trick mit `juce::Image::setPixelAt` für das Tray-Icon, das sonst auf Windows komplett schwarz bleibt) wurde iterativ im Dialog mit Claude (Modell: Opus 4.x) erarbeitet.

Der menschliche Anteil bestand in: Idee, Anforderungen, Akzeptanztests am realen Audio-System, Hardware-Tuning, Bug-Reports aus dem Praxisbetrieb, Lizenz- und Release-Entscheidungen. Claude Code lieferte Entwurf, Implementierung, Debugging und Refactoring.

Ohne dieses Werkzeug wäre Crossmixer nicht entstanden. Wenn dir das Projekt gefällt, ist ein guter Teil des Lobs an Anthropic und das Team hinter Claude Code zu richten.

---

## Lizenz

Crossmixer ist freie Software unter der **GNU Affero General Public License v3.0** (siehe [`LICENSE`](LICENSE)).

Das bedeutet unter anderem:
- Du darfst die Software nutzen, studieren, ändern und weitergeben.
- Weitergabe (auch modifiziert) muss unter AGPLv3 erfolgen und den Quellcode enthalten bzw. zugänglich machen.
- Wird die Software über ein Netzwerk angeboten, müssen Nutzer ebenfalls Zugang zum (ggf. modifizierten) Quellcode bekommen.

JUCE wird dabei unter seiner AGPLv3-Option eingebunden — siehe `JUCE/LICENSE.md`.

---

## Autor

Stefan Kammann — <184531164+clrsrc@users.noreply.github.com>

Issues und Pull Requests willkommen.
