/* 
	This class definition is meant to mimic that of the Comm32.h file provided 
	with the northern digital API sample.

	Chris Burrows
*/

#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>

#pragma once

// Baud constants
const unsigned int MIN_BAUD_RATE = 9600;
const unsigned int MAX_BAUD_RATE = 115200;
const unsigned int DEFAULT_BAUD_RATE = 9600;

// READ ERROR
const int SERIAL_READ_ERROR = -1000;	// Any number that a char can't take
// WRITE ERROR
const int SERIAL_WRITE_ERROR = -1001;	// Any number that a char can't take

class serialCommunicator
{

public:
	serialCommunicator();
	~serialCommunicator();

	void SerialClose();
	int SerialOpen( unsigned Port, unsigned long BaudRate, unsigned Format,
					bool RtsCts, unsigned long SerialBreakDelay );

	int SerialOpen( const std::string &portName );
	int SerialSetBaud(unsigned int baudRate);

	int SerialBreak();
	int SerialFlush();

	int SerialPutChar( unsigned char uch, unsigned int msTimeout = 3000 );
	int SerialPutString( std::string &string, unsigned long len, unsigned int msTimeout = 3000 );

	int SerialGetChar( unsigned int msTimeout = 3000 );
	int SerialCharsAvailable();
	int SerialGetString(char *pStr, unsigned long maxLen);

	int SerialWaitForResponse( int timeoutMSec );
	int SerialWaitForSend( int timeoutMSec );

	int SerialSetHardwareHandshaking( int hardwareHandshake );

	int SerialSetDefaultOptions();


private:
	boost::asio::io_service IO_service;
	boost::asio::serial_port *newSerial;
	unsigned long serialBreakms;

	boost::asio::deadline_timer *pTimer;
	bool writeError;
	bool readError;

	void SerialWriteComplete(  const boost::system::error_code &error, size_t bytes_written );
	void SerialReadComplete(  const boost::system::error_code &error, size_t bytes_written );
	void SerialTimeOut( const boost::system::error_code &error );

};
