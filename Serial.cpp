#include "Serial.h"

Serial::Serial() {
    this->connected = false;
    dcbSerialParams = { 0 };
}

std::vector<int> Serial::getAvailablePorts()
{
    wchar_t lpTargetPath[5000]; // buffer to store the path of the COM PORTS
    std::vector<int> portList;

    for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
    {
        std::wstring str = L"COM" + std::to_wstring(i); // converting to COM0, COM1, COM2
        DWORD res = QueryDosDevice(str.c_str(), lpTargetPath, 5000);

        if (res != 0)
        {
            portList.push_back(i);
        }

    }
    return portList;
}

// flag can be NULL or FILE_FLAG_OVERLAPPED
bool Serial::openPort(int numPort, DWORD flag) {

    std::string name = std::string("COM" + std::to_string(numPort)).c_str();

    LPCSTR COMname = name.c_str();

    HANDLE hComm;

    hComm = CreateFileA(COMname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, flag, NULL);

    if (hComm == INVALID_HANDLE_VALUE) {
        printf("Error in opening serial port");
        return false;
    }

    this->hCOM = hComm;
    return true;
}

std::string Serial::getDeviceName(int numPort) {

    wchar_t lpTargetPath[5000];
    std::wstring str = L"COM" + std::to_wstring(numPort);

    DWORD res = QueryDosDevice(str.c_str(), lpTargetPath, 5000);

    int count = 0;
    int countBackSlash = 0;
    std::string nome;

    for (count = 0; lpTargetPath[count] != L'\0'; count++) {
        if (lpTargetPath[count] == '\\')
            countBackSlash++;
        
        if (countBackSlash == 3) {
            nome.push_back(lpTargetPath[count]);
        }

        if (countBackSlash == 2) {
            countBackSlash++;
        }
    }
   
    return nome;
}

bool Serial::connect() {

    if (!GetCommState(hCOM, &dcbSerialParams)) {
        printf("Warning: Failed to get current serial params");
        return false;
    }
    else {

        if (!SetCommState(hCOM, &dcbSerialParams)) {
            printf("Warning: could not set serial port params\n");
            return false;
        }
        else {
            connected = true;
            PurgeComm(hCOM, PURGE_RXCLEAR | PURGE_TXCLEAR);
        }
    }
    return true;
}

bool Serial::WriteSerialPort(LPCSTR data_sent)
{
    DWORD bytes_sent;
    COMSTAT status;
    DWORD errors;

    int data_sent_length = strlen(data_sent);

    if (!WriteFile(hCOM, (void*)data_sent, data_sent_length, &bytes_sent, NULL)) {
        ClearCommError(hCOM, &errors, &status);
        std::cout << errors << std::endl;
        return false;
    }

    return true;
}

bool Serial::WriteSerialPortOverlapped(LPCSTR data_sent)
{
    OVERLAPPED osWrite = { 0 };
    DWORD dwBytesTransferred = 0;
    COMSTAT status;
    DWORD errors;

    int data_sent_length = strlen(data_sent);

    if (!WriteFile(hCOM, (void*)data_sent, data_sent_length, &dwBytesTransferred, &osWrite)) {
        ClearCommError(hCOM, &errors, &status);
        //std::cout << errors << std::endl;
        return false;
    }

    GetOverlappedResult(hCOM, &osWrite, &dwBytesTransferred, TRUE);

    std::cout << dwBytesTransferred;

    return true;
}

// can be a listener (blocking) and non overlapped (synchronous)
// this synchronous version rely on ClearCommError() so that we keep track of the internal buffer queue, and dont need to use timers.
void Serial::ReadSerialPort(bool listen)
{

    char* message = NULL;
    int size = 0;

    DWORD nBytesRead;
    DWORD dwEvtMask;
    BOOL bResult;

    COMSTAT status;
    DWORD errors;

    SetCommMask(hCOM, EV_RXCHAR | EV_ERR); //receive character event

    //wait for character (this will listen indefinitely)
    while (WaitCommEvent(hCOM, &dwEvtMask, 0)) {

        // im using this function here so we can retrieve the status of the internal buffer queue every new event received on the serial buffer
        ClearCommError(hCOM, &errors, &status);

        size = status.cbInQue; // getting internal buffer queue

        if (message) {
            delete[] message;
        }

        message = new char[size + 1];
        
        if (dwEvtMask & EV_RXCHAR)
            bResult = ReadFile(hCOM, message, size, &nBytesRead, NULL);
        else if (dwEvtMask & EV_ERR)
            std::cout << "error: " << dwEvtMask << std::endl;

        /*
        // it should trigger the end of file/stream, but never happens (nBytesRead is never null)
        if (bResult && nBytesRead == 0) 
        {
            std::cout << "end of stream\n";
        }
        */

        message[size] = '\0';

        std::cout << message;

        if (!listen)
            break;
    }

}

void Serial::ReadSerialPortOverlapped(bool listen) {

    OVERLAPPED osReader = { 0 };
    BOOL bResult;
    DWORD dwBytesTransferred = 0;
    DWORD dwEvtMask;
    COMSTAT status;
    DWORD errors;

    char* buffer = NULL;
    int length = 1;
    std::string message("");


    SetCommMask(hCOM, EV_RXCHAR | EV_ERR); //receive character event

    // Create the overlapped event. Must be closed before exiting
    // to avoid a handle leak.
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Error creating overlapped event; abort.
    if (osReader.hEvent == NULL) {
        std::cout << "error";
    }

    //while makes it a blocking listener
    while (!WaitCommEvent(hCOM, &dwEvtMask, &osReader)) {

        length = 1;

        do {

            if (buffer) {
                delete[] buffer; //freeing
            }
            
            buffer = new char[length + 1]; // malloc(sizeof(char)*length;

            // cant know beforehand how many characters, but you know at least one came, because the WaitCommEvent signaled
            ReadFile(hCOM, buffer, length, &dwBytesTransferred, &osReader);

            bResult = GetOverlappedResult(hCOM, &osReader, &dwBytesTransferred, TRUE); // blocking, intended to be used with threads

            buffer[length] = '\0';

            message = message + std::string(buffer);

            //now you can get the amount of characters still on the internal buffer queue (ClearCommError() will return 0 if used before ReadFile() on overlapped)
            ClearCommError(hCOM, &errors, &status);
            length = status.cbInQue;
            
        } while (length != 0);

        std::cout << message;
        message.clear(); // reset string to build next string
        
        ResetEvent(osReader.hEvent);

        if (!listen)
            break;
    }

    CloseHandle(osReader.hEvent);
}


LPCTSTR Serial::ErrorMessage(DWORD error)

// Routine Description:
//      Retrieve the system error message for the last-error code
{

    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    return((LPCTSTR)lpMsgBuf);
}

bool Serial::CloseSerialPort()
{
    if (connected) {
        connected = false;
        CloseHandle(hCOM);
        return true;
    }
    else
        return false;
}