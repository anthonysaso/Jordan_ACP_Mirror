#include "spark_wiring_usartserial.h"
#include "spark_wiring_constants.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring.h"
class S3B{
public:
    bool transmit(byte *address, byte *data, int len);
    int parseReceive(byte *s3bData, char *buffer, int len);
    bool validateReceivedData(byte *s3bData, int len);
    int getReceiveDataLength(byte *s3bData);
    bool parseAddress(String addr, byte *buffer);
    int getRSSI();

private:
    byte startDelimiter = 0x7E;
    byte frameType = 0x10;
    byte frameID = 0x01;
    byte reserved1 = 0xFF;
    byte reserved2 = 0xFE;
    byte bRadius = 0x00;
    byte transmitOptions = 0x00;

    unsigned long ackTimeOut = 50;

    void flashLED(int count);
    void flushSerialPort();
};
