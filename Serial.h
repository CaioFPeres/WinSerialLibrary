#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

class Serial
{

private:
	bool connected;
	DCB dcbSerialParams;
	HANDLE hCOM;

public:

	Serial();
	~Serial();

	std::vector<int> getAvailablePorts();
	std::string getDeviceName(int numPort);
	static LPCTSTR ErrorMessage(DWORD error);
	bool openPort(int numPort, DWORD flag);
	bool connect();
	bool WriteSerialPort(LPCSTR data_sent);
	bool WriteSerialPortOverlapped(LPCSTR data_sent);
	void ReadSerialPort(bool listen);
	void ReadSerialPortOverlapped(bool listen);
	bool CloseSerialPort();

};

