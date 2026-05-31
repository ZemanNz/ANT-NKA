#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // Vytvoření UDP socketu
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Chyba vytváření socketu!" << std::endl;
        return 1;
    }

    // Nastavení adresy robota
    struct sockaddr_in robotAddr;
    memset(&robotAddr, 0, sizeof(robotAddr));
    robotAddr.sin_family = AF_INET;
    robotAddr.sin_port = htons(1234);  // Port musí sedět s robotem
    
    // 👇 SEM DOSAĎ IP ADRESU ROBOTA
    if (inet_pton(AF_INET, "192.168.4.1", &robotAddr.sin_addr) <= 0) {
        std::cerr << "Neplatná IP adresa!" << std::endl;
        close(sock);
        return 1;
    }

std::cout << "🤖 POKROČILÝ OVLADAČ ROBOTA" << std::endl;
std::cout << "==============================" << std::endl;
std::cout << "Dostupné příkazy:" << std::endl;
std::cout << "🎯 POHYB:" << std::endl;
std::cout << "  forward(délka, rychlost)     - např: forward(2000,50)" << std::endl;
std::cout << "  backward(délka, rychlost)    - např: backward(1000,30)" << std::endl;
std::cout << "  turn_left(úhel, rychlost)    - např: turn_left(90,40)" << std::endl;
std::cout << "  turn_right(úhel, rychlost)   - např: turn_right(90,40)" << std::endl;
std::cout << "  radius_left(r,úhel,rychlost) - zatáčka vlevo s poloměrem" << std::endl;
std::cout << "  radius_right(r,úhel,rychlost)- zatáčka vpravo s poloměrem" << std::endl;
std::cout << "  forward_acc(délka, rychlost) - vpřed s akcelerací" << std::endl;
std::cout << "  backward_acc(délka, rychlost)- vzad s akcelerací" << std::endl;
std::cout << "  back_buttons(rychlost)       - couvání na tlačítka" << std::endl;
std::cout << "  power(levý, pravý)           - přímý výkon motorů -100..100" << std::endl;
std::cout << "  speed(levý, pravý)           - nastavení rychlosti -100..100" << std::endl;
std::cout << "  drive(lmm,rmm,lrychl,prychl) - přesné řízení pojezdu" << std::endl;
std::cout << std::endl;
std::cout << "🔧 SERVA:" << std::endl;
std::cout << "  servo(id,úhel)               - hloupé servo -90..90°" << std::endl;
std::cout << "  servo_off(id)                - vypnutí serva" << std::endl;
std::cout << "  smart_servo(id,pozice)       - chytré servo 0..1000" << std::endl;
std::cout << std::endl;
std::cout << "💡 LED:" << std::endl;
std::cout << "  led(id,stav)                 - LED podle ID 1-4" << std::endl;
std::cout << "  led_red(1/0)                 - červená LED" << std::endl;
std::cout << "  led_yellow(1/0)              - žlutá LED" << std::endl;
std::cout << "  led_green(1/0)               - zelená LED" << std::endl;
std::cout << "  led_blue(1/0)                - modrá LED" << std::endl;
std::cout << "  led_all(1/0)                 - všechny LED" << std::endl;
std::cout << std::endl;
std::cout << "📊 SENZORY:" << std::endl;
std::cout << "  ultra(id)                    - ultrazvuk 1-4" << std::endl;
std::cout << "  color(název)                 - barevný senzor (RGB)" << std::endl;
std::cout << "  laser(název)                 - laserový dálkoměr" << std::endl;
std::cout << "  ir_left()                    - IR senzor vlevo" << std::endl;
std::cout << "  ir_right()                   - IR senzor vpravo" << std::endl;
std::cout << "  battery()                    - stav baterie a teplota" << std::endl;
std::cout << std::endl;
std::cout << "🔊 BZUČÁK:" << std::endl;
std::cout << "  buzzer(1/0)                  - zap/vyp bzučák" << std::endl;
std::cout << std::endl;
std::cout << "🔄 OSTATNÍ:" << std::endl;
std::cout << "  on()                         - zapne motory" << std::endl;
std::cout << "  off()                        - vypne motory" << std::endl;
std::cout << "  stop                         - okamžité zastavení" << std::endl;
std::cout << "  help()                       - tento help" << std::endl;
std::cout << "  exit                         - ukončí program" << std::endl;
std::cout << "==============================" << std::endl;

    std::string command;
    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, command);

        if (command == "exit") {
            break;
        }

        // Odeslání příkazu robotovi
        int result = sendto(sock, command.c_str(), command.length() + 1, 0,
                           (struct sockaddr*)&robotAddr, sizeof(robotAddr));
        
        if (result < 0) {
            std::cerr << "Chyba odesílání!" << std::endl;
        } else {
            std::cout << "📤 Odesláno: " << command << std::endl;
        }
    }

    // Úklid
    close(sock);
    std::cout << "Program ukončen." << std::endl;
    
    return 0;
}
//g++ -o robot_controller_terminal robot_controller_terminal.cpp
//./robot_controller_terminal