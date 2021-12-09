#include <iostream>
#include "Serial.h"

DWORD WINAPI readThread(LPVOID lParam) {

    Serial* port = (Serial*)lParam;

    port->ReadSerialPortOverlapped(true);

    return 0;
}

DWORD WINAPI writeThread(LPVOID lParam) {

    Serial* port = (Serial*)lParam;

    while (1) { // this will cause blue screen after some seconds ( too fast for windows i guess)
        port->WriteSerialPortOverlapped("teste");
    }
    
    return 0;
}

int main()
{
    int porta = 0;
    Serial* port = new Serial();

    std::vector<int> ports = port->getAvailablePorts();

    for (int i = 0; i < ports.size(); i++) {
        porta = ports[i];
    }

    if (port->getDeviceName(porta) != "Silabser0") {
        std::cout << "Wemos is not connected" << std::endl;
        return 0;
    }


    port->openPort(porta, FILE_FLAG_OVERLAPPED);
    port->connect();

    DWORD threadID;
    DWORD threadID2;

    //listener
    
    HANDLE writeThreadHandle = CreateThread(NULL, 0, writeThread, port, 0, &threadID);
    HANDLE readThreadHandle = CreateThread(NULL, 0, readThread, port, 0, &threadID2);
    

    while (1) {
        Sleep(1000);
    }

    return 0;
}