/**
 * @file robotka.h
 *
 * Metody v tomto souboru vám dovolují jednoduše obsluhovat Robotku.
 *
 */

#ifndef _LIBRB_H
#define _LIBRB_H

#include <memory>

#include <fmt/core.h>
#include <fmt/printf.h>

#include <Arduino.h>
#include <Wire.h>
#include "SmartServoBus.hpp"
#include <Adafruit_TCS34725.h>
#include "RBCX.h"
#include "gridui.h"
#include "rbprotocol.h"

using namespace gridui;

/**
 * \defgroup general .INICIALIZACE ROBOTA
 *
 * Tato sekce je určená k počátečnímu nastavení knihovny pro Robotku.
 *
 * @{
 */

/**
 * \brief Nastavení čísel pinů různých periferií.
 *
 * Zde můžete přenastavit piny, pokud máte periferie připojené na desce na jíném pinu.
 */
struct rkPinsConfig {
    rkPinsConfig()
        : ir_adc_chan_left(ADC1_CHANNEL_0)
        , ir_adc_chan_right(ADC1_CHANNEL_3) {
    }

    adc1_channel_t ir_adc_chan_left;
    adc1_channel_t ir_adc_chan_right;
};


/**
 * \brief Nastavení SW pro Robotky
 *
 * Tato struktura obsahuje konfigurační hodnoty pro software Robotky.
 * Předává se funkci rkSetup(). Ve výchozím stavu má smysluplné hodnoty
 * a není třeba nastavovat všechny, ale jen ty, které chcete změnit.
 */
struct rkConfig {
    rkConfig()
        : prevod_motoru(1983.3f) // pro 12v ==  41.62486f * 48.f, pro 6v == 1981.3f
        , left_wheel_diameter(61.0) // v mm
        , right_wheel_diameter(61.0) // v mm
        , roztec_kol(270.0) // v mm
        , konstanta_radius_vnejsi_kolo(0.96f) // Korekční faktor pro vnější kolo při zatáčení
        , konstanta_radius_vnitrni_kolo(0.96f) // Korekční faktor pro vnitřní kolo při zatáčení
        , korekce_nedotacivosti_left(0.97f)// Korekce nedotáčivosti při otaceni na miste do leva
        , korekce_nedotacivosti_right(0.97f)// Korekce nedotáčivosti při otaceni na miste do prava
        , Button1(NULL)
        , Button2(NULL)
        , motor_id_left(4)
        , motor_id_right(1)
        , motor_max_power_pct(100)
        , motor_polarity_switch_left(false)
        , motor_polarity_switch_right(true)
        , motor_enable_failsafe(false)
        , motor_wheel_diameter(61.0)
        , motor_max_ticks_per_second(5200) // vyzkousite tak ze spustite funkci max_rychlost() a podle toho nastavite
        , motor_max_acceleration(50000)
        , stupid_servo_min(-1.65f)
        , stupid_servo_max(1.65f)
        , pocet_chytrych_serv(0)
        , enable_wifi_log(false)
        , enable_wifi_control_wasd(false)
        , enable_wifi_terminal(false)
        , wifi_ssid("robot1234")        // pro wasd a wifi_terminal je to jmeno wifi ktere robot vytvori!!! , pro logovani je to wifi ke ktere se pripoji ----> bacha to jmeno musi byt nejak dlouhy, jinak vam to nepujde prejmenovat
        , wifi_password("1234robot") {       // pro wasd a wifi_terminal je to heslo wifi ktere robot vytvori!!! , pro logovani je to wifi ke ktere se pripoji ----> bacha to heslo musi byt nejak dlouhy, jinak vam to nepujde prejmenovat
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /**
     * \brief prevod_motoru - počet ticků enkodéru na jeden mm.
     * 
     * Používá se pro přepočet ujeté vzdálenosti na počet ticků enkodéru.
     * 
     * Výchozí hodnota je pro 6V motory s koly o průměru 62mm je ´1979.3f´.
     * 
     * Vypočítáte tak, že zadáte at ujede robot treba 1000mm s zmerite kolik realne ujedete a touto hodnotou vydelite tuto konstantu.
     */
    float prevod_motoru;

    /**
     * \brief left_wheel_diameter - Průměr levého kola robota v mm.
     * 
     * Výchozí hodnota je `62.2` mm.
     * 
     * Rozdíl mezi koly vyrovnáte nastavením této hodnoty.
     */
    float left_wheel_diameter;

    /**
     * \brief right_wheel_diameter - Průměr pravého kola robota v mm.
     * 
     * Výchozí hodnota je `62.0` mm.
     * 
     * Rozdíl mezi koly vyrovnáte nastavením této hodnoty.
     */
    float right_wheel_diameter;

    /**
     * \brief Rozteč kol robota v mm, použito na počítání ujeté vzdálenosti při zatáčení.
     * 
     * Pokud jsou kola široká, počítejte spíše s menšímy hodnotami.
     */
    float roztec_kol;

    /**
     * \brief Korekční faktor pro vnější kolo při zatáčení
     * 
     * Výchozí hodnota je `1.035f`.
     */
    float konstanta_radius_vnejsi_kolo = 1.035f;

    /**
     * \brief Korekční faktor pro vnitřní kolo při zatáčení
     * 
     * Výchozí hodnota je `1.0084f`.
     */
    float konstanta_radius_vnitrni_kolo = 1.0084f;


    /**
     * \brief Korekce nedotáčivosti při otáčení na místě doleva
     * 
     * Výchozí hodnota je `1.1f`.
     */
    float korekce_nedotacivosti_left;

    /**
     * \brief Korekce nedotáčivosti při otáčení na místě doprava
     * 
     * Výchozí hodnota je `1.0f`.
     */
    float korekce_nedotacivosti_right;

    /**
     * \brief Číslo GPIO pro tlačítko v zadu robota na zastavení až se dotkne zdi
     * 
     * Výchozí hodnota je `14`, možné je pouýžít třeba 17,34, 35 ...  Ale můžete nastavit na `NULL`, pokud tlačítko nechcete používat.
     */
    byte Button1;

    /**
     * \brief Číslo GPIO pro tlačítko v zadu robota na zastavení až se dotkne zdi
     * 
     * Výchozí hodnota je `35`, možné je pouýžít třeba 14,17,34 ...  Ale můžete nastavit na `NULL`, pokud tlačítko nechcete používat.
     */
    byte Button2;


    uint8_t motor_id_left; //!< Které M číslo motoru patří levému, podle čísla na desce. Výchozí: `2`
    uint8_t motor_id_right; //!< Které M číslo motoru patří pravému, podle čísla na desce. Výchozí: `1`

    uint8_t motor_max_power_pct; //!< Limit výkonu motoru v procentech od 0 do 100. Ovlivňuje všechny režimy motorů. Výchozí: `60`
    bool motor_polarity_switch_left; //!< Prohození polarity levého motoru. Výchozí: `false`
    bool motor_polarity_switch_right; //!< Prohození polarity pravého motoru. Výchozí: `true`
    bool motor_enable_failsafe; //!< Zastaví motory po 500ms, pokud není zavoláno rkSetMotorPower nebo rkSetMotorSpeed. Výchozí: `false`
    uint16_t motor_wheel_diameter; //!< Průměr kol robota v mm, použito na počítání ujeté vzdálenosti. Výchozí: `67` mm.

    /**
     * \brief Maximální rychlost motorů v ticích enkodéru za vteřinu. Výchozí: `2000`
     *
     * Ovlivňuje regulátor rychlosti motorů, který se používá u funkcí rkMotorsSetSpeed
     * a rkMotorsDrive. Oba dva motory by měli být schopny dosáhnout tuto rychlost.
     * 
     * Rychlostní regulátor je ovlivněn i `motor_max_power_pct` hodnotou.
     */
    uint32_t motor_max_ticks_per_second;

    /**
     * \brief Maximální zrychlení motorů v ticích enkodéru za vteřinu. Výchozí: `50000`
     *
     * Ovlivňuje regulátor rychlosti motorů, který se používá u funkcí rkMotorsSetSpeed
     * a rkMotorsDrive. Vyšší číslo znamená, že motory budou mít rychlejší náběh na cílovou rychlost,
     * ale za to se mohou smýkat po podlaze.
     */
    uint32_t motor_max_acceleration;

    float stupid_servo_min; //!< Spodní hranice signálu pro hloupá serva, která se robvná -90 stupňům. Výchozí: `-1.65`
    float stupid_servo_max; //!< Horní hranice signálu pro hloupá serva, která se rovná 90 stupňům. Výchozí: `1.65`

    int8_t pocet_chytrych_serv;

    bool enable_wifi_log; //!< Povolení WiFi logování. Výchozí: `false` ---> Jestli se má na začátku inicializovat wifi, bez toho nepojedou wifi logy.
    bool enable_wifi_control_wasd; //!< Povolení WiFi ovládání přes SWAD. Výchozí: `false` ---> Jestli se má na začátku inicializovat wifi, bez toho nepojedou wifi ovládání přes wasd.
    bool enable_wifi_terminal; //!< Povolení WiFi terminálu. Výchozí: `false` ---> Jestli se má na začátku inicializovat wifi, bez toho nepojedou wifi terminál.
    const char* wifi_ssid; //!< SSID WiFi sítě pro připojení. Výchozí: `nullptr`  ----> Jmeno wifi na kterou se ma robot pripojit
    const char* wifi_password; //!< Heslo WiFi sítě pro připojení. Výchozí: `nullptr` ---> Heslo wifi na kterou se ma robot pripojit

    rkPinsConfig pins; //!< Konfigurace pinů pro periferie, viz rkPinsConfig
};

/**
 * \brief Inicializační funkce Robotky
 *
 * Tuhle funci MUSÍTE zavolat vždy na začátku vaší funkce setup() v main.cpp.
 * Můžete jí předat nastavení ve formě struktury rkConfig.
 */
void rkSetup(const rkConfig& cfg = rkConfig());

/**@}*/
/**
 * \defgroup motors Motory
 *
 * Metody pro obsluhu motorů.
 * @{
 */

/**
 * \brief Nastavení výkonu motorů.
 *
 * \param left výkon levého motoru od od -100% do 100%
 * \param right výkon pravého motoru od od -100 do 100%
 */
void rkMotorsSetPower(int8_t left, int8_t right);

/**
 * \brief Nastavení výkonu levého motoru.
 *
 * \param power výkon levého motoru od od -100% do 100%
 */
void rkMotorsSetPowerLeft(int8_t power);

/**
 * \brief Nastavení výkonu pravého motoru.
 *
 * \param power výkon levého motoru od od -100% do 100%
 */
void rkMotorsSetPowerRight(int8_t power);

/**
 * \brief Nastavení výkonu motoru podle jeho čísla (M1...M4) na desce.
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param power výkon motoru od od -100% do 100%
 */
void rkMotorsSetPowerById(uint8_t id, int8_t power);

/**
 * \brief Nastavení rychlosti motorů.
 *
 * \param left rychlost levého motoru od od -100% do 100%
 * \param right rychlost pravého motoru od od -100% do 100%
 */
void rkMotorsSetSpeed(int8_t left, int8_t right);

/**
 * \brief Nastavení rychlosti levého motoru.
 *
 * \param speed rychlost levého motoru od od -100% do 100%
 */
void rkMotorsSetSpeedLeft(int8_t speed);

/**
 * \brief Nastavení rychlosti pravého motoru.
 *
 * \param speed rychlost levého motoru od od -100% do 100%
 */
void rkMotorsSetSpeedRight(int8_t speed);

/**
 * \brief Nastavení rychlosti motoru podle jeho čísla (M1...M4) na desce.
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param speed rychlost motoru od od -100% do 100%
 */
void rkMotorsSetSpeedById(uint8_t id, int8_t speed);

/**
 * \brief Popojetí oběma motory o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mmLeft kolik milimetrů má ujet levý motor
 * \param mmRight kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 */
void rkMotorsDrive(float mmLeft, float mmRight, float speed_left, float speed_right);

/**
 * \brief Popojetí levým motorem o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mm kolik milimetrů má ujet levý motor
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveLeft(float mm, uint8_t speed);

/**
 * \brief Popojetí pravým motorem o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveRight(float mm, uint8_t speed);

/**
 * \brief Popojetí motorem podle ID o vzdálenost v mm (blokující).
 *
 * Tato funkce zablokuje program, dokud robot danou vzdálenost neujede.
 *
 * \param id číslo motoru, který má jet, od 1 do 4 včetně
 * \param mm kolik milimetrů má motor ujet
 * \param speed rychlost, kterou má motor jet od -100% do 100%
 */
void rkMotorsDriveById(uint8_t id, float mm, uint8_t speed);

/**
 * \brief Popojetí oběma motory o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveAsync(100, 100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mmLeft kolik milimetrů má ujet levý motor
 * \param mmRight kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveAsync(float mmLeft, float mmRight, uint8_t speed_left, uint8_t speed_right, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí levým motorem o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveLeftAsync(100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mm kolik milimetrů má ujet levý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveLeftAsync(float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí pravým motorem o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveRightAsync(100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveRightAsync(float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Popojetí motorem podle ID o vzdálenost v mm (asynchroní).
 *
 * Tato funkce vrátí okamžítě, a po dojetí zavolá callback.
 *
 * Příklad použití:
 * \code{cpp}
 * rkMotorsDriveByIdAsync(2, 100, 50, []() {
 *     printf("Dojel jsem!\n");
 * });
 * \endcode
 *
 * \param id číslo motoru, který má jet, od 1 do 4 včetně
 * \param mm kolik milimetrů má ujet pravý motor
 * \param speed rychlost, kterou má robot jet od -100% do 100%
 * \param callback funkce, která je zavolána jakmile robot dojede o určenou vzdálenost
 */
void rkMotorsDriveByIdAsync(uint8_t id, float mm, uint8_t speed, std::function<void()> callback = nullptr);

/**
 * \brief Vrátí absolutní najetou vzálenost na levém motoru v mm
 *
 * \param fetch zeptá se koprocesoru na opravdovou pozici, místo toho, aby vrátil "cachovanou" hodnotu, ale volání trvá déle
 * \return absolutní (celková) najetá vzdálenost na levém motoru v mm
 */
float rkMotorsGetPositionLeft(bool fetch = false);

/**
 * \brief Vrátí absolutní najetou vzálenost na pravém motoru v mm
 *
 * \param fetch zeptá se koprocesoru na opravdovou pozici, místo toho, aby vrátil "cachovanou" hodnotu, ale volání trvá déle
 * \return absolutní (celková) najetá vzdálenost na pravém motoru v mm
 */
float rkMotorsGetPositionRight(bool fetch = false);

/**
 * \brief Vrátí absolutní najetou vzálenost na motoru podle ID
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param fetch zeptá se koprocesoru na opravdovou pozici, místo toho, aby vrátil "cachovanou" hodnotu, ale volání trvá déle
 * \return absolutní (celková) najetá vzdálenost na motoru v mm
 */
float rkMotorsGetPositionById(uint8_t id, bool fetch = false);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na levém motoru na hodnotu v mm
 *
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionLeft(float positionMm = 0.f);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na pravém motoru na hodnotu v mm
 *
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionRight(float positionMm = 0.f);

/**
 * \brief Nastaví absolutní počítadlo vzdálenosti na motoru podle ID na hodnotu v mm
 *
 * \param id číslo motoru od 1 do 4 včetně
 * \param positionMm absolutní pozice, na kterou nastavit čítač
 */
void rkMotorsSetPositionById(uint8_t id, float positionMm = 0.f);

/**
 * \brief Nastavení rychlosti motorů podle joysticku.
 *
 * Tato funkce nastaví rychlost motorů podle výstupu z joysticku. Očekává dvě
 * hodnoty od -32768 do 32768, posílané například aplikací RBController.
 * Funkce tyto hodnoty převede na rychlost a nastaví ji.
 *
 * \param x X hodnota z joysticku.
 * \param y Y hodnota z joysticku.
 */
void rkMotorsJoystick(int32_t x, int32_t y);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**@}*/

/******************************************************************************
 * Komplexní funkce pro pohyb
 *
 * Toto jsou funkce které při správném nastavení v rkConfig() pojedou přesně a jsou
 * zde tedy univerzální.
 *
 * Všechny funkce jsou blokující -> pokud si chcete udělat podobnou, ale dělat při
 * do ní ještě jednu věc (měřit barvu, ...), tak není nic jednoduššího:
 * vykopírujte si ji z _librk_motors.cpp a upravte podle toho co potřebujete.
 *
 * Všechny funkce mají výpisi printf(), nebo std::cout<< ------> nyní zakomentované 
 * ----> pokud chcete upravovat/ vylepsovat / zjišťovat procto nejede ----> tak by jste to meli odkomentovat !!!!!!!

 ******************************************************************************/

/**
 * \defgroup Komplexni_funkce_pro_pohyb Komplexní funkce pro pohyb
 *
 * @{
 */


/**
 * \brief Vrátí maximální rychlost robota, to co potom zadate na motor_max_ticks_per_second.
 * 
 * Pozor mue chvily trvat a motory se budou tocit.
 */
int16_t max_rychlost();

/**
 * \brief Pohyb robota vpřed (pokud máte správně nastavený polarity switch) o zadanou vzdálenost v mm  a rychlost v %. (blokující)
 * 
 * Tato funkc hned nastavý rychlost motorů ---> robot s sebou může trochu trhnout na začátku pohybu. ---->  Použít raději forward_acc pro plynulý rozjezd.
 * 
 * Robot využívá P - regulátor.
 * 
 * Timeout si robot vypočítá podle rychlosti a vzdálenosti s rezervou.
 */
void forward(float mm, float speed);

/**
 * \brief Pohyb robota vzad (pokud máte správně nastavený polarity switch) o zadanou vzdálenost v mm a rychlost v %. (blokující)
 * 
 * Tato funkc hned nastavý rychlost motorů ---> robot s sebou může trochu trhnout na začátku pohybu. ---->  Použít raději backward_acc pro plynulý rozjezd.
 * 
 * Robot využívá P - regulátor.
 * 
 * Timeout si robot vypočítá podle rychlosti a vzdálenosti s rezervou.
 */
void backward(float mm, float speed);

/**
 * \brief Otočení se robota na místě proti směru hodinových ručiček.
 * 
 * Úhel od 0 do 360, a doporučujeme rychlost do 60%
 * 
 * Robot využívá P - regulátor.
 * 
 * Robot během pohybu zrychlujeazpomaluje
 * 
 * Timeout je nastavený na 10 sekund.
 */
void turn_on_spot_left(float angle, float speed);

/**
 * \brief Otočení se robota na místě po směru hodinových ručiček.
 * 
 * Úhel od 0 do 360, a doporučujeme rychlost do 60%
 * 
 * Robot využívá P - regulátor.
 * 
 * Robot během pohybu zrychluje a zpomaluje
 * 
 * Timeout je nastavený na 10 sekund.
 */
void turn_on_spot_right(float angle, float speed);

/**
 * \brief Robot jede rádius do prava.
 * 
 * radius == vzdálenost od stredu otáčení k vnitřnímu kolu
 * 
 * Úhel od 0 do 360, a doporučujeme rychlost do 60%
 * 
 * Robot využívá P - regulátor.
 * 
 * Robot během pohybu zrychluje a zpomaluje
 * 
 * Timeout je nastavený na 10 sekund.
 */
void radius_right(float radius, float angle, float speed);

/**
 * \brief Robot jede rádius do leva.
 * 
 * radius == vzdálenost od stredu otáčení k vnitřnímu kolu
 * 
 * Úhel od 0 do 360, a doporučujeme rychlost do 60%
 * 
 * Robot využívá P - regulátor.
 * 
 * Robot během pohybu zrychluje a zpomaluje
 * 
 * Timeout je nastavený na 10 sekund.
 */
void radius_left(float radius, float angle, float speed);

/**
 * \brief Pohyb robota vpřed se zrychkením a zpomalením (robot s sebou necukne)
 * 
 * Lze vyuzit i pro male vzdalenosti.
 * 
 * Robot využívá P - regulátor.
 * 
 * Timeout je vypočítaný stejně jako u forward jen vynásoben 2.
 */
void forward_acc(float mm, float speed);

/**
 * \brief Pohyb robota vzad se zrychkením a zpomalením (robot s sebou necukne)
 * 
 * Lze vyuzit i pro male vzdalenosti.
 * 
 * Robot využívá P - regulátor.
 * 
 * Timeout je vypočítaný stejně jako u forward jen vynásoben 2.
 */
void backward_acc(float mm, float speed);

/**
 * \brief Zjistí, zda je stisknuto vlastní fyzické tlačítko 1 (napojeno na pin z rkConfig.Button1)
 * \param waitForRelease Přejete-li si po vyhodnocení stisku počkat na puštění tlačítka (výchozí: nepauznout - false)
 * \return Vrátí `true` pokud je tlačítko stisknuto na hodnotě LOW.
 */
bool rkButton1(bool waitForRelease = false);

/**
 * \brief Zjistí, zda je stisknuto vlastní fyzické tlačítko 2 (napojeno na pin z rkConfig.Button2)
 * \param waitForRelease Přejete-li si po vyhodnocení stisku počkat na puštění tlačítka (výchozí: nepauznout - false)
 * \return Vrátí `true` pokud je tlačítko stisknuto na hodnotě LOW.
 */
bool rkButton2(bool waitForRelease = false);

/**
 * \brief Pohyb robota vzad, dokud obě zadané podmínky nevrátí true.
 * Timeout je 10 sekund. Po stisku prvního se na druhé čeká 3 vteřiny.
 * Udržuje rovný směr pomocí P-regulátoru a enkodérů.
 * \param speed Rychlost (0-100), se kterou se bude couvat.
 * \param first_button Funkce vyhodnocující první stranu (výchozí je konfigurace tlačítek 1)
 * \param second_button Funkce vyhodnocující druhou stranu (výchozí je konfigurace tlačítek 2)
 */
void back_buttons(float speed, std::function<bool()> first_button = []{ return rkButton1(); }, std::function<bool()> second_button = []{ return rkButton2(); });

/**
 * \brief Pohyb robota vpřed, dokud obě zadané podmínky nevrátí true.
 * Timeout je 10 sekund. Po stisku prvního se na druhé čeká 3 vteřiny.
 * Udržuje rovný směr pomocí P-regulátoru a enkodérů.
 * \param speed Rychlost (0-100) vpřed.
 * \param first_button Funkce vyhodnocující první stranu (výchozí je konfigurace tlačítek 1)
 * \param second_button Funkce vyhodnocující druhou stranu (výchozí je konfigurace tlačítek 2)
 */
void front_buttons(float speed, std::function<bool()> first_button = []{ return rkButton1(); }, std::function<bool()> second_button = []{ return rkButton2(); });

/**
 * \brief Pohyb robota vpřed, a jede podél zdi pomocí dvou senzorů vzdálenosti
 * 
 * Timeout je vypočítaný jako u forward;
 * 
 * Při pohybu jede robut s P - regulátorem.
 * 
 * Pokud je speed záporné ---> robit pojede do zadu. V takovem pripade je potreba zadat prvni senzor ten v zadu robota.
 * 
 * First senzor je ten, kterej je prvni ve smeru jizdy robota.
 * 
 * Nejspise je potreba donastavovat podle aktualniho robota.
 * 
 * okolik_je zadni dal ---> treba -23 pokud je bliz zdi
 * 
 * pokud je bool automatic_distance_of_wall tru tak se ignoruje distance of wall
 */
void wall_following(float distance_to_drive, float speed, bool automatic_distance_of_wall ,float distance_of_wall, bool is_wall_on_right,
                   std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni);


/**
 * \brief Srovnání robota podle zdi pomocí dvou senzorů vzdálenosti
 * 
 * Timeout je 5 000 ms == 5 sekund, pokud chcete zmenit tak v _librk_motors.cpp v teto funkci zmente int timeut_ms = 5000;
 * 
 * Důležité je jaký senzor je first ---> First je prvni ve predni casti robota , pokud je na boku. First je na prave strane, pokud je v zadu nebo ve predu robota.
 * 
 * Důležité je aby jste spravne nastavily button_or_right ---> pokud je zed od robota na prave stranenebo vzadu tak nastavte true, pokud je zed na leve strane nebo ve predu tak nastavte false
 * 
 * Pokud je robot vyrovnanej na zactku tak se nic nestane.
 * 
 * Volte spise nizsi rychlosti do 30 % --- 10% je defaultni hodnota
 * 
 * Funkce nezvládne větší úhly ---- > pokud kolem neni zadna jina stena pouzit orient_to_wall_any_price() !!!!
 * 
 * Funkce first_sensor a second_sensor vrací hodnoty v mm ze senzorů vzdálenosti --- ultrazvuky nebo laserový s.
 * 
 */
void orient_to_wall(bool button_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni = 0, float speed = 10);

/**
 * \brief Srovnání robota podle nejbližší zdi pomocí dvou senzorů vzdálenosti
 * 
 * Timeout je 8 000 ms == 8 sekund, pokud chcete zmenit tak v _librk_motors.cpp v teto funkci zmente int timeut_ms = 8000;
 * 
 * Důležité je jaký senzor je first ---> First je prvni ve predni casti robota , pokud je na boku. First je na prave strane, pokud je v zadu nebo ve predu robota.
 * 
 * Důležité je aby jste spravne nastavily button_or_right ---> pokud je zed od robota na prave stranenebo vzadu tak nastavte true, pokud je zed na leve strane nebo ve predu tak nastavte false
 * 
 * Pokud je robot vyrovnanej na zactku tak se nic nestane.
 * 
 * Volte spise nizsi rychlosti do 30 % --- 10% je defaultni hodnota
 * 
 * Funkce udela to ze se robot otoci o 360 stupnu a zjisti, kde nejblize je zed a k ni se vyrovna
 * 
 * Funkce first_sensor a second_sensor vrací hodnoty v mm ze senzorů vzdálenosti --- ultrazvuky nebo laserový s.
 */

void orient_to_wall_any_price(bool button_or_right, std::function<uint32_t()> first_sensor, 
                   std::function<uint32_t()> second_sensor, int o_kolik_je_dal_zadni = 0, float speed = 20);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**@}*/

/**
 * \defgroup battery Baterie
 *
 * Metody pro získání informací o stavu baterie.
 * @{
 */

/**
 * \brief Úroveň baterie v procentech
 *
 * \return Hodnota od 0 do 100 udávající nabití baterie.
 */
uint32_t rkBatteryPercent();

/**
 * \brief Úroveň baterie v mV.
 *
 * \return Naměřené napětí na baterii v milivoltech.
 */
uint32_t rkBatteryVoltageMv();

/**
 * \brief Teplota desky v Robotce
 *
 * \return Naměřená teplota v °C
 */
int16_t rkTemperature();

/**@}*/
/**
 * \defgroup rbcontroller Aplikace RBController
 *
 * Metody pro komunikaci s aplikací RBController pro Android.
 * @{
 */

/**
 * \brief Odeslat text do aplikace.
 *
 * Tato metoda odešle text, který se zobrazí v aplikaci v černém okně nahoře.
 * Příklad:
 *
 *     rkControllerSendLog("Test logu! Stav Baterie: %u mv", rkBatteryVoltageMv());
 *
 * \param format Text k odeslání, může obsahovat formátovací značky jako C funkce printf().
 * \param ... argumenty pro format, funguje stejně jako printf().
 */
void rkControllerSendLog(const char* format, ...);
void rkControllerSendLog(const std::string& text);

void rkControllerSend(const char* cmd, rbjson::Object* data = nullptr);
void rkControllerSendMustArrive(const char* cmd, rbjson::Object* data = nullptr);

/**@}*/
/**
 * \defgroup leds LEDky
 *
 * Metody pro zapínaní a vypínání LEDek na desce.
 * @{
 */

/**
 * \brief Zapnout/vypnout červenou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedRed(bool on = true);
/**
 * \brief Zapnout/vypnout žlutou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedYellow(bool on = true);
/**
 * \brief Zapnout/vypnout zelenou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedGreen(bool on = true);
/**
 * \brief Zapnout/vypnout modrou LED
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedBlue(bool on = true);

/**
 * \brief Zapnout/vypnout všechny LED zaráz
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedAll(bool on = true);

/**
 * \brief Zapnout/vypnout LED podle jejich čísla na desce, od 1 do 4 včetně.
 * \param on `true` pro zapnuto, `false` pro vypnuto.
 */
void rkLedById(uint8_t id, bool on = true);

/**@}*/
/**
 * \defgroup buttons Tlačítka
 *
 * Funkce pro vyčítání stavu tlačítek.
 * @{
 */

/**
 * \brief Seznam konstant označujícíh tlačítka, pro použítí v rkButtonIsPressed a dalších.
 */
enum rkButtonId {
    BTN_DOWN = rb::ButtonId::Down, //!< Tlačítko dolů
    BTN_UP = rb::ButtonId::Up, //!< Tlačítko nahoru
    BTN_LEFT = rb::ButtonId::Left, //!< Tlačítko doleva
    BTN_RIGHT = rb::ButtonId::Right, //!< Tlačítko doprava
    BTN_ON = rb::ButtonId::On, //!< Tlačítko ON/Ok (prostřední)
    BTN_OFF = rb::ButtonId::Off, //!< Tlačítko Off/Esc
};

#define EXTRA_BUTTON1 27 //!< Číslo pinu pro připojení extra koncového tlačítka, blíže okraji desky
#define EXTRA_BUTTON2 14 //!< Číslo pinu pro připojení extra koncového tlačítka, blíže do středu desky

/**
 * \brief Je teď stisknuto tlačítko?
 *
 * \param id ID tlačítka z enumu rkButtonId
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
bool rkButtonIsPressed(rkButtonId id, bool waitForRelease = false);

/**
 * \brief Asynchroní zpracování události o stisku tlačítka.
 *
 * Ukázka použití:
 *
 * \code{cpp}
 * rkButtonOnChange([](rkButtonId id, bool pressed) -> bool {
 *     if (id == BTN_DOWN) {
 *         printf("Dolů: %d\n", pressed);
 *     } else if (id == BTN_UP) {
 *         printf("Nahoru: %d\n", pressed);
 *     } else if (id == BTN_LEFT) {
 *         printf("Doleva: %d\n", pressed);
 *     } else if (id == BTN_RIGHT) {
 *         printf("Doprava: %d\n", pressed);
 *     }
 *     return true;
 * });
 * \endcode
 *
 * \param callback funkce, která je zavolána pokud se stav kteréhokoliv tlačítka změní.
 *    parametry jsou ID tlačítka z rkButtonId a bool isPressed. Funkce musí vrátit
 *    true nebo false, podle toho, jestli má čekat na další události (true) nebo
 *    se má odstranit a další události už nepřijmat (false).
 */
void rkButtonOnChangeAsync(std::function<bool(rkButtonId, bool)> callback = nullptr);

/**
 * \brief Je teď stisknuto "dolů"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonDown(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_DOWN, waitForRelease);
}

/**
 * \brief Je teď stisknuto "nahoru"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonUp(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_UP, waitForRelease);
}

/**
 * \brief Je teď stisknuto "doleva"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonLeft(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_LEFT, waitForRelease);
}

/**
 * \brief Je teď stisknuto "doprava"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonRight(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_RIGHT, waitForRelease);
}

/**
 * \brief Je teď stisknuto "ON"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonOn(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_ON, waitForRelease);
}

/**
 * \brief Je teď stisknuto "OFF"?
 *
 * \param waitForRelease pokud je stisknuto, počká před vrácením výsledku na jeho uvolnění (default: false)
 * \return Vrátí `true` pokud je tlačítko stisknuto.
 */
inline bool rkButtonOff(bool waitForRelease = false) {
    return rkButtonIsPressed(BTN_OFF, waitForRelease);
}

/**
 * \brief Počkat, dokud není tlačítko uvolněno.
 *
 * Pokud tlačítko není stisknuté, počká pouze několik desítek ms, tedy nečeká na stisknutí.
 *
 * \param id ID tlačítka z enumu rkButtonId
 */
void rkButtonWaitForRelease(rkButtonId id);


/**@}*/
/**
 * \defgroup irmodules Sledování čáry (!! IR moduly !!)
 *
 * Funkce pro čtení hodnot z obdélníkových IR modulů, které mají každý jeden senzor.
 *
 * @{
 */

/**
 * \brief Hodnota z levého IR senzoru
 * \return naměřená hodnota od 0(nejvíce bílá) do 4095(nejvíce černá)
 */
uint16_t rkIrLeft();

/**
 * \brief Hodnota z pravého IR senzoru
 * \return naměřená hodnota od 0(nejvíce bílá) do 4095(nejvíce černá)
 */
uint16_t rkIrRight();

/**@}*/
/**
 * \defgroup ultrasound Ultrazvuky
 *
 * Funkce pro meření vzálenosti pomocí ultrazvuků. V mm !!!
 * @{
 */

/**
 * \brief Změřit vzálenost (blokující)
 *
 * Změří vzálenost ultrazvukem a vrátí výsledek v milimetrech.
 * Může blokovat program až 30ms, podle toho, co ultrazvuk naměří.
 *
 * \param id Id ultrazvuku, od 1 do 4 včetně, podle popisků na desce.
 * \return Naměřená vzdálenost v mm, 0 pokud se měření nepodaří.
 */
uint32_t rkUltraMeasure(uint8_t id);

/**
 * \brief Změřit vzálenost (asynchroní)
 *
 * Přidá požadavek na měření vzdálenosti do fronty a okamžitě vrátí.
 * Jakmile je změřeno, je zavolána funkce předaná jako callback,
 * jako parametr bude mít naměřenou vzálenost v mm.
 *
 * Příklad použití:
 *
 * \code{.cpp}
 * rkUltraMeasureAsync(1, [](uint32_t distance_mm) -> bool {
 *    printf("Namereno: %u mm\n", distance_mm);
 *    return true;
 * });
 *
 * rkLedBlue(true); // provede se ještě před tím, než se stihne změřit vzdálenost.
 * \endcode
 *
 * \param id Id ultrazvuku, od 1 do 4 včetně, podle popisků na desce.
 * \param callback funkce, která bude zavolána po naměření. Do jejího parametru
 *     bude předána naměřená vzdálenost v mm. 0 znamená chybu v měření.
 *     Pokud chcete měřit jen jednou, vraťe z callbacku `false`, pokud opakovaně,
 *     pak vracejte `true`.
 */
void rkUltraMeasureAsync(uint8_t id, std::function<bool(uint32_t)> callback);

/**@}*/

/**@}*/
/**
 * \defgroup buzzer Bzučák
 *
 * Ovládání bzučáku na desce.
 * @{
 */

/**
 * \brief Zapnout/vypnout bzučák
 * \param on Zapnout(true) nebo vypnout(false)
 */
void rkBuzzerSet(bool on);


/**
 * @brief Inicializuje TCS34725 barevný senzor na zadané I2C sběrnici.
 *
 * Funkce ukládá instanci senzoru do interního pole podle jména a spouští
 * komunikaci s modulem. Každý senzor je identifikován unikátním jménem.
 *
 * @param name    Textový identifikátor senzoru (např. "front" nebo "down").
 * @param bus     Referenční I2C sběrnice (Wire nebo Wire1) pro komunikaci.
 * @param tcs     Reference na instanci Adafruit_TCS34725 pro daný senzor.
 * @return true   Pokud se senzor úspěšně inicializoval.
 * @return false  Pokud se nepodařilo spojení s modulem.
 */
bool rkColorSensorInit(const char* name, TwoWire& bus, Adafruit_TCS34725& tcs);

/**
 * @brief Načte hodnoty RGB z barevného senzoru podle jeho identifikátoru.
 *
 * Funkce vyhledá senzor v interním seznamu podle jména a použije
 * metodu getRGB z knihovny Adafruit, která vrací normalizované
 * RGB hodnoty v rozsahu 0–255.
 *
 * @param name  Textový identifikátor senzoru (stejný jako při init).
 * @param r     Ukazatel na float pro červenou složku (0.0–255.0).
 * @param g     Ukazatel na float pro zelenou složku (0.0–255.0).
 * @param b     Ukazatel na float pro modrou složku (0.0–255.0).
 * @return true   Pokud se měření úspěšně provedlo.
 * @return false  Pokud senzor není nalezen.
 */
bool rkColorSensorGetRGB(const char* name, float* r, float* g, float* b);


#ifdef USE_VL53L0X // Podmíněná kompilace pro barevný senzor
#include <Adafruit_VL53L0X.h> // Přidání této řádky

/**
 * @brief Inicalizuje VL53L0X ToF senzor a přepíše jeho I2C adresu, pokud je potřeba.
 *
 * Provádí hardwarový reset senzoru pomocí XSHUT pinu, poté:
 *   - Pokusí se bootnout na zadanou new_address; pokud se to podaří, inicializace skončí.
 *   - Jinak provede boot na výchozí adrese 0x29, zapíše do registru 0x8A new_address
 *     a provede druhý boot na new_address.
 *
 * Defaultne je maximalni pocet senzoru 6
 * Před inicializovanim senzoru nastavit xshut pin na low --- jak je v examplu
 * Senzor defaultně měří do 1,2 m
 * 
 * 
 * @param name        Textový identifikátor senzoru pro ladicí výpisy.
 * @param bus         Referenční I2C sběrnice (Wire nebo Wire1) pro komunikaci.
 * @param lox         Reference na instanci Adafruit_VL53L0X pro daný senzor.
 * @param xshut_pin   GPIO číslo pinu připojeného na XSHUT senzoru (hard reset).
 * @param new_address Nová 7bit I2C adresa senzoru (např. 0x30).
 */
void rk_laser_init(const char*      name,
    TwoWire&         bus,
    Adafruit_VL53L0X& lox,
    uint8_t          xshut_pin,
    uint8_t         new_address);
/**
 * @brief Přečte jednorázově vzdálenost z jednoho senzoru podle jména.
 */
/**
 * @brief Přečte jednorázově vzdálenost z VL53L0X senzoru podle jeho identifikátoru.
 *
 * Vyhledá senzor v interním seznamu podle jména a provede jednorázové měření.
 *
 * @param name  Textový identifikátor senzoru (stejný, jaký byl zadán při init).
 * @return      Naměřená vzdálenost v milimetrech, nebo -1 při chybě či mimo rozsah.
 */
int rk_laser_measure(const char* name);
/**@}*/
#endif

/**@}*/
/**
 * \defgroup stupidservos Serva (hloupá)
 *
 * Metody pro ovládání hloupých servo.
 * @{
 */

/**
 * \brief Nastaví pozici hloupého serva na zadaný úhel
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 * \param angleDegrees úhel natočení serva od -90 do 90 stupňů.
 */
void rkServosSetPosition(uint8_t id, float angleDegrees);

/**
 * \brief Vrátí naposledy nastavenou pozici hloupého serva.
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 * \return nastavený úhel serva ve stupňích, nebo NaN (viz std::isnan()) pokud je servo vypnuté.
 */
float rkServosGetPosition(uint8_t id);

/**
 * \brief Vypne hloupé servo
 *
 * Přestane se do serva posílát signál, ono tedy přestane držet svoji pozici.
 *
 * \param id číslo serva od 1 do 4 včetně, podle popisku na desce (SERVO1...SERVO4)
 */
void rkServosDisable(uint8_t id);

/**@}*/

/**
 * \defgroup smartservos Serva (chytrá)
 *
 * Metody pro ovládání chytrých serv. Chytrá serva se ovládájí přes knihovnu Esp32-lx16a,
 * knihovna robotky má pouze pár pomocných metod pro získání správně nastavené instance.
 * @{
 */

/**
 * \brief Vrátí instanci SmartServoBus pro ovládání chytrých serv.
 *
 * 
 *
 * \param servo_count počet serv napojených na sběrnici. Serva musí mít ID nastavená
 *     v rozmezí <0;servo_count). servo_count je použito pouze při prvním volání
 *     rkSmartServoBus fce, v dalších můžete prostě použít 0.
 *                    
 * \return reference na SmartServoBus objekt, vrací pokaždé tu stejnou referenci.
 */
lx16a::SmartServoBus& rkSmartServoBus(uint8_t servo_count);

/**
 * \brief Smart Servo funkce
 */

/**
 * \brief Inicializace smart serva
 * 
 * @param id ID serva (0–253)
 * @param low Dolní limit úhlu v ° (výchozí 0)
 * @param high Horní limit úhlu v ° (výchozí 240)
 * @param max_diff_centideg Maximum difference between expected and actual servo angle before the AutoStop Triggers ---- for soft_move
 * @param max_diff_readings How many times must the difference between expected and actual servo angle ---- for soft_move
 */
void rkSmartServoInit(int id, int low = 0, int high = 240, int16_t max_diff_centideg = 400, uint8_t  max_diff_readings = 3);

/**
 * \brief Rychlý pohyb serva bez regulace
 * 
 * @param bus Reference na SmartServoBus
 * @param id ID serva (0–253)
 * @param angle Cílový úhel v ° (0-240)
 * @param speed Rychlost (výchozí 200)
 */
void rkSmartServoMove(int id, int angle, int speed = 200);

/**
 * \brief Plynulý pohyb serva s ochranou proti zaseknutí
 * 
 * @param bus Reference na SmartServoBus
 * @param id ID serva (0–253)
 * @param angle Cílový úhel v ° (0-240)
 * @param speed Rychlost (výchozí 200)
 */
void rkSmartServoSoftMove(int id, int angle, int speed = 200);

/**
 * \brief Přečte aktuální pozici smart serva
 * 
 * @param bus Reference na SmartServoBus
 * @param id ID serva 1 nebo 0
 */
byte rkSmartServosPosicion(int id);
////////////////////////////////////////////////////////////////

/**
 * \brief Při inicializaci robota a pokud je zapnuté enable_wifi_log se do serial monitoru vypise ip adresa, na niž se posilaji výpisy , které napíšete do tehle funkce.
 * 
 * Můžete napsat třeba : ´printf_wifi(" Start forward - %.1f mm, %.1f%% speed", mm, speed)´
 */
void printf_wifi(const char* format, ...);

/**
 * \brief Při používání print_wifi musíte ve smyčce volat tuto funkci s nejakým delay(), pokud ne tak se nebudou posílát hodnoty ---> web se nebude refreshovat
 * 
 * Buď to tedy volat ve smyčce třeba v main, nebo vytvořit vlákno.
 */
void handleWebClients();


//////////////////////////////////////////////////////

/**
 * \brief Po zavolání této funkce se dostanete do nekonečné smyčky ovládání robota přes WiFi, kterou lze ukončit klávesou P z ovladače
 * 
 * Robot vytvoří vlastní WiFi síť s následujícím nastavením:
 * - SSID (jméno sítě): nastaveno v cfg.wifi_ssid (výchozí "robot123")
 * - Heslo: nastaveno v cfg.wifi_password (výchozí "1234")
 * - IP adresa robota: 192.168.4.1
 * 
 * Postup připojení:
 * 1. Připojte se na WiFi síť vytvořenou robotem
 * 2. Spusťte program "robot_controller_wasd" z RBCX-controller
 * 3. V programu použijte IP adresu 192.168.4.1
 * 
 * Ovládání:
 * - WASD nebo šipky pro pohyb robota ---- (sipka doprede --> 60% , otaceni po 20% ----> lze nastavit, viz dale)
 * - P pro ukončení ovládání
 * - Klávesy L,K,J,H,G,M,N,B pro spuštění vlastních funkcí
 * 
 * Pro úpravu vlastních funkcí (FUNC1-FUNC8) upravte soubor:
 * lib/RB3204-RBCX-Robotka-library-master/src/wifi_control.cpp v metodě Wifi::handleWebClients()
 */
void wifi_control_wasd();

///////////////////////////////

/**
 * \brief Po zavolání této funkce se dostanete do nekonecne smycky.
 * 
 * Funkce slouží k ovládání robota na dálku přes terminál ---> můžete zadat např. forward(1000, 50) a příkaz se vykonná
 * 
 * Robot vytvoří vlastní WiFi síť s následujícím nastavením:
 * - SSID (jméno sítě): nastaveno v cfg.wifi_ssid (výchozí "robot123")
 * - Heslo: nastaveno v cfg.wifi_password (výchozí "1234")
 * - IP adresa robota: 192.168.4.1
 * 
 * Postup připojení:
 * 1. Připojte se na WiFi síť vytvořenou robotem
 * 2. Spusťte program "robot_controller_terminal" z RBCX-controller-----> tak jak to tam je nejspiš jen na linuxu
 * 3. V programu použijte IP adresu 192.168.4.1
 */
void wifi_control_terminal();

//////////////////////////////////////////////////////
/**
 * \brief Spustí serialový terminál pro příkazy
 * 
 * Tato funkce běží nekonečně a čeká na příkazy ze sériového portu.
 * POZOR: Serial musí být inicializován v main.cpp (Serial.begin(115200))
 * 
 * Dostupné příkazy:
 * 
 * === POHYB ROBOTA ===
 * - forward(mm, speed)           - pohyb vpřed o zadanou vzdálenost (mm) a rychlost (%)
 * - forward_acc(mm, speed)       - pohyb vpřed s plynulým zrychlením a zpomalením
 * - backward(mm, speed)          - pohyb vzad o zadanou vzdálenost (mm) a rychlost (%)
 * - backward_acc(mm, speed)      - pohyb vzad s plynulým zrychlením a zpomalením
 * - turn_on_spot_left(angle, speed) - otočení na místě doleva o úhel (stupně) a rychlost (%)
 * - turn_on_spot_right(angle, speed) - otočení na místě doprava o úhel (stupně) a rychlost (%)
 * - radius_left(radius, angle, speed) - zatáčka doleva s poloměrem (mm), úhlem (stupně) a rychlost (%)
 * - radius_right(radius, angle, speed) - zatáčka doprava s poloměrem (mm), úhlem (stupně) a rychlost (%)
 * - back_buttons(speed)          - couvání, dokud nenarazí oběma tlačítky na zeď
 * - max_rychlost()               - změří maximální rychlost motorů (ticks za sekundu)
 * - stop()                       - okamžité zastavení motorů
 * - set_speed(left, right)       - nastaví rychlost motorů v % (-100 až 100)
 * - set_power(left, right)       - nastaví výkon motorů v % (-100 až 100)
 * 
 * === SMART SERVA ===
 * - servo_init(id, [low, high])  - inicializace smart serva s ID, volitelně limity úhlu (low, high)
 * - servo_move(id, angle, [speed]) - rychlý pohyb serva na úhel (0-240°) s volitelnou rychlostí
 * - servo_soft_move(id, angle, [speed]) - plynulý pohyb serva s ochranou proti zaseknutí
 * - servo_position(id)           - přečte aktuální pozici smart serva
 * 
 * Pokud si chcete doplnit příkazy, upravte funkci processCommand v robotka.cpp
 */
void rkSerialTerminal();
/////////////////////////////////////////////////////////////////
/**
 * \defgroup uart UART Komunikace
 * 
 * Funkce pro UART komunikaci s externími zařízeními.
 * 
 * V examplupro UART je code i pro samostatny esp32 který komunikuje s robotkou přes UART
 * @{
 */

/**
 * \brief Inicializace UART komunikace
 * 
 * V examplupro UART je code i pro samostatny esp32 který komunikuje s robotkou přes UART
 * 
 * @param baudRate Rychlost komunikace (default: 115200)
 * @param rxPin RX pin (default: 16)
 * @param txPin TX pin (default: 17)
 * @return true pokud inicializace proběhla úspěšně
 */
bool rkUartInit(int baudRate = 115200, int rxPin = 16, int txPin = 17);

/**
 * \brief Přijetí UART zprávy (blokující)
 * 
 * @param msg Ukazatel na strukturu, kam se uloží přijatá data
 * @param timeoutMs Timeout v milisekundách (default: 1000)
 * @return true pokud byla zpráva úspěšně přijata
 */
bool rkUartReceive_blocking(void* msg, size_t msgSize, uint32_t timeoutMs = 1000);

/**
 * \brief Přijetí UART zprávy (neblokující)
 * 
 * @param msg Ukazatel na strukturu, kam se uloží přijatá data
 * @return true pokud byla zpráva úspěšně přijata
 */
bool rkUartReceive(void* msg, size_t msgSize);

/**
 * \brief Odeslání UART zprávy
 * 
 * @param msg Ukazatel na strukturu s daty k odeslání
 * @param msgSize Velikost struktury v bytech
 */
void rkUartSend(const void* msg, size_t msgSize);


//////////////////////////////////////////////////////
/**@}*/

/**
 * \defgroup mpu Gyroskop a Akcelerometr (MPU6050)
 * 
 * Funkce pro práci s na desce napájeným gyroskopem a zjišťování náklonu / rotace robota.
 * @{
 */

/**
 * \brief Inicializuje gyroskop (MPU6050) a provede základní kalibraci.
 * 
 * Během inicializace robot nesmí být v pohybu, jinak se gyroskop zkalibruje špatně
 * a úhel Z bude neustále ujíždět.
 * 
 * Pokud na desce fyzicky nemate gyroskop ---> budou se vypisovat 0.
 */
void rkMpuInit();

/**
 * \brief Vrátí aktuální úhel natočení robota (osa Z) ve stupních.
 * \return Úhel ve stupních.
 */
float rkMpuGetAngleZ();

/**
 * \brief Vrátí aktuální úhel náklonu robota dopředu/dozadu (osa X) ve stupních.
 * \return Úhel ve stupních.
 */
float rkMpuGetAngleX();

/**
 * \brief Vrátí aktuální úhel náklonu robota do stran (osa Y) ve stupních.
 * \return Úhel ve stupních.
 */
float rkMpuGetAngleY();

/**
 * \brief Vynuluje aktuální úhel na ose Z.
 * Ideální použít těsně před zatáčením, abyste se mohli otočit relativně z aktuální pozice.
 */
void rkMpuResetZ();

/**
 * \brief Vynuluje aktuální úhel na ose X.
 * Pomocí této funkce se aktuální náklon robota dopředu/dozadu začne hlásit jako 0.
 */
void rkMpuResetX();

/**
 * \brief Vynuluje aktuální úhel na ose Y.
 * Pomocí této funkce se aktuální náklon robota do stran začne hlásit jako 0.
 */
void rkMpuResetY();

/**
 * \brief Vynuluje úhly na všech osách (X, Y, Z) naráz.
 */
void rkMpuResetAll();

/**@}*/

#endif // _LIBRB_H

