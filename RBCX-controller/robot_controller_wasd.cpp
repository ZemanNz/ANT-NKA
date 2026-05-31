#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// Funkce pro čtení klávesy bez čekání na enter
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// Funkce pro čtení šipek (speciální znaky)
int readArrowKey() {
    if (!kbhit()) return 0;
    
    int ch = getchar();
    if (ch == 27) { // ESC sequence
        if (!kbhit()) return 0;
        ch = getchar();
        if (ch == 91) { // [
            if (!kbhit()) return 0;
            ch = getchar();
            return ch + 1000; // Vrátí 1065=↑, 1066=↓, 1067=→, 1068=← (abychom se vyhnuli konfliktu)
        }
    }
    return ch;
}

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
    
    // 👇 SEM DOSAĎ IP ADRESU ROBOTA (tu co vidíš v sériové konzoli)
    if (inet_pton(AF_INET, "192.168.4.1", &robotAddr.sin_addr) <= 0) {
        std::cerr << "Neplatná IP adresa!" << std::endl;
        close(sock);
        return 1;
    }

    std::cout << "🤖 OVLADAČ ROBOTA - UDP VERZE" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Klávesy WASD:" << std::endl;
    std::cout << "  W = Dopředu" << std::endl;
    std::cout << "  S = Dozadu" << std::endl;
    std::cout << "  A = Doleva" << std::endl;
    std::cout << "  D = Doprava" << std::endl;
    std::cout << std::endl;
    std::cout << "Klávesy šipky:" << std::endl;
    std::cout << "  ↑ = Dopředu" << std::endl;
    std::cout << "  ↓ = Dozadu" << std::endl;
    std::cout << "  ← = Doleva" << std::endl;
    std::cout << "  → = Doprava" << std::endl;
    std::cout << std::endl;
    std::cout << "Další funkce"<< std::endl;
    std::cout << "  L = funkce1" << std::endl;
    std::cout << "  K = funkce2" << std::endl;
    std::cout << "  J = funkce3" << std::endl;
    std::cout << "  H = funkce4" << std::endl;
    std::cout << "  G = funkce5" << std::endl;
    std::cout << "  M = funkce6" << std::endl;
    std::cout << "  N = funkce7" << std::endl;
    std::cout << "  B = funkce8" << std::endl;
    std::cout << "  V = funkce9" << std::endl;
    std::cout << "Ostatní klávesy:" << std::endl;
    std::cout << "  P = Vypni wasd ovladani" << std::endl;
    std::cout << "  ESC = Konec" << std::endl;
    std::cout << "==============================" << std::endl;

    int speed = 50; // Výchozí rychlost
    
    while (true) {
        int key = readArrowKey();
        
        if (key) {
            std::string message;
            bool validKey = true;

            // Zpracování WASD a šipek
            switch (key) {
                // WASD
                case 'w':
                case 'W': 
                    message = "FORWARD"; 
                    break;
                case 's':
                case 'S': 
                    message = "BACKWARD"; 
                    break;
                case 'a':
                case 'A': 
                    message = "LEFT"; 
                    break;
                case 'd':
                case 'D': 
                    message = "RIGHT"; 
                    break;
                
                // Šipky (1065-1068)
                case 1065:   // Šipka nahoru
                    message = "FORWARD"; 
                    break;
                case 1066:   // Šipka dolů
                    message = "BACKWARD"; 
                    break;
                case 1068:   // Šipka doleva
                    message = "LEFT"; 
                    break;
                case 1067:   // Šipka doprava
                    message = "RIGHT"; 
                    break;

                // Funkce L, K, J, H, G, M, N, B, V
                case 'l':
                case 'L': message = "FUNC1"; break;
                case 'k':
                case 'K': message = "FUNC2"; break;
                case 'j':
                case 'J': message = "FUNC3"; break;
                case 'h':
                case 'H': message = "FUNC4"; break;
                case 'g':
                case 'G': message = "FUNC5"; break;
                case 'm':
                case 'M': message = "FUNC6"; break;
                case 'n':
                case 'N': message = "FUNC7"; break;
                case 'b':
                case 'B': message = "FUNC8"; break;
                case 'v':
                case 'V': message = "FUNC9"; break;
                
                // Ostatní příkazy
                case 'p':
                case 'P': message = "OFF"; break;
                
                
                // ESC pro ukončení
                case 27:  
                    message = "EXIT";
                    break;
                    
                default: 
                    validKey = false;
                    continue;
            }

            // Odeslání zprávy robotovi
            if (validKey) {
                int result = sendto(sock, message.c_str(), message.length() + 1, 0,
                                   (struct sockaddr*)&robotAddr, sizeof(robotAddr));
                
                if (result < 0) {
                    std::cerr << "Chyba odesílání!" << std::endl;
                } else {
                    // Speciální výpis pro rychlost
                    if (message.find("SPEED_") == 0) {
                        std::cout << "🎚️  Rychlost nastavena na: " << speed << "%" << std::endl;
                    } else {
                        std::cout << "📤 Odesláno: " << message;
                        if (message == "FORWARD" || message == "BACKWARD" || 
                            message == "LEFT" || message == "RIGHT") {
                        }
                        std::cout << std::endl;
                    }
                }

                // Ukončení při ESC
                if (key == 27) break;
            }
        }
        
        usleep(10000);  // 10ms zpoždění
    }

    // Úklid
    close(sock);
    std::cout << "Program ukončen." << std::endl;
    
    return 0;
}

// g++ -o robot_controller_wasd robot_controller_wasd.cpp
// ./robot_controller_wasd
