# Projekt Antenka 🐜🤖

Tento soubor README slouží jako příručka pro obsluhu a přípravu robota **Antenka** před startem zápasu. Popisuje kompletní startovací menu, předzápasový checklist a strategii.

---

## 📋 Předzápasový Checklist (Co zkontrolovat před startem)

Před každým zápasem je nutné provést následující kroky:

1. **🔋 Kontrola akumulátoru**:
   * Zkontroluj napětí baterie na displeji/přes sériový monitor.
   * Ujisti se, že je baterie plně nabitá pro stabilní napájení motorů i elektroniky.
2. **🧹 Čištění kol**:
   * Očisti gumová kola od prachu a nečistot (např. izopropylalkoholem nebo vlhkým hadříkem).
   * Čistá kola jsou naprosto klíčová pro přesné měření ujeté vzdálenosti a správné fungování odometrie s gyroskopem.
3. **🎯 Kalibrace gyroskopu (DŮLEŽITÉ)**:
   * **Po zapnutí robota s ním vůbec nehýbej!**
   * Robot provádí automatickou kalibraci gyroskopu (měří offsety).
   * **Počkej, dokud se nerozsvítí žlutá LED dioda na desce (`rkLedYellow`)**. Jakmile žlutá LED svítí, inicializace je kompletní a robot je připraven ke konfiguraci.
4. **🛠️ Mechanická kontrola**:
   * Ověř, že se rameno volně pohybuje a magnet drží na svém místě.
   * Zkontroluj čistotu optických senzorů (TCS34725/VL53L0X).

---

## 🕹️ Startovací Menu (Konfigurace před zápasem)

Robot se konfiguruje přímo na startovní pozici pomocí tlačítek na těle robota: zadního levého (**LEFT**), zadního pravého (**RIGHT**), horního (**UP**) a dolního (**DOWN**).

### 1. 🔴/🔵 Výběr barvy týmu (Tlačítko UP)
Tým se vybírá cyklicky stisknutím tlačítka **UP**.
* **Výchozí stav**: Červený tým (RED).
* **Krátký stisk tlačítka UP**: Přepíná barvu zápasu RED $\leftrightarrow$ BLUE.
* **Indikace na desce**:
  * Tým **RED** $\rightarrow$ Svítí červená desková LED (`rkLedRed`).
  * Tým **BLUE** $\rightarrow$ Svítí modrá desková LED (`rkLedBlue`).

### 2. 🔍 Procházení kombinací (Tlačítko DOWN)
Stisknutím tlačítka **DOWN** můžeš cyklicky měnit kombinace **1 -> 2 -> 3 -> 4 -> 1...** pro náhled na LED pásku, **aniž by to odstartovalo jízdu**. Žlutá LED po stisku zabliká (index + 1)krát jako potvrzení zvolené kombinace.

---

## 🚀 Spuštění programu (START + Výběr kombinace)

Výběr rozložení docků (kombinace 1 až 4) se provádí zadními tlačítky **LEFT** a **RIGHT**. Uvolněním tlačítka se zvolí kombinace a **okamžitě startuje soutěžní jízda** (po uvolnění proběhne bezpečnostní prodleva **300 ms** pro oddálení ruky, zhasne se LED pásek a robot vyjede).

| Tlačítko | Typ stisku | Délka stisku | Akce po uvolnění |
| :--- | :--- | :--- | :--- |
| **RIGHT** (Pravé) | Krátký | $\le 0{,}5$ s | **START** s **Kombinací 1** (index 0) |
| **RIGHT** (Pravé) | Dlouhý | $> 0{,}5$ s | **START** s **Kombinací 2** (index 1) |
| **LEFT** (Levé) | Krátký | $\le 0{,}5$ s | **START** s **Kombinací 3** (index 2) |
| **LEFT** (Levé) | Dlouhý | $> 0{,}5$ s | **START** s **Kombinací 4** (index 3) |

> [!NOTE]
> Po uvolnění se detekovaný stisk a zvolená kombinace vypíší na Serial monitor (např. `[CONF] Pravé tlačítko uvolněno (DLOUHÝ stisk: 580 ms) -> START (Kombinace 2)`).

---

## 🌈 Indikace na LED pásku (iLED)

8-diodový adresovatelný LED pásek (připojený na pinu `12`) v reálném čase zobrazuje rozložení barev baterek v docku z pohledu tvé startovní pozice:

* **Orientace (odspodu nahoru)**:
  * **Nejspodnější dioda (index 0 pásku)** představuje baterku **nejblíže** tvé startovní zóně (index 7 pro RED, index 0 pro BLUE).
  * Diody pokračují nahoru směrem k nejvzdálenější baterce (nejvrchnější dioda index 7).
* **Barevná shoda**:
  * Červená kostka na hřišti $\rightarrow$ dioda svítí **tlumeně červeně**.
  * Modrá kostka na hřišti $\rightarrow$ dioda svítí **tlumeně modře**.
* **Jas**: Jas pásku je nastaven na velmi nízkou hodnotu (2 z 255), aby diody neoslňovaly okolí a byly snadno čitelné.
* **Konec zápasu / start**: Jakmile je zápas spuštěn, LED pásek se **úplně zhasne**, aby nesvítil během soutěžní jízdy.