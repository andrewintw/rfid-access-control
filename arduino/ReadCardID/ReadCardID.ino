#include <SoftwareSerial.h>

SoftwareSerial RDM630Uart(11, 10); /* RX (connect to TX in RDM630)
                    TX (connect to RX in RDM630) */
void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    RDM630Uart.begin(9600);
    Serial.println("setup done");
}

void loop() {
    if (RDM630Uart.available()) {
        char c = RDM630Uart.read();
        if (c == '\x2') {
            Serial.println("\n========================");
        }
        Serial.print(c);
        Serial.print('(');
        Serial.print(c, HEX);
        Serial.println("h)");
        if (c == '\x3') {
            Serial.println("------------------------");
        }
    }
}
