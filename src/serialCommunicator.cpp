#include "serialCommunicator.h"
#include <iostream>
#include <boost\bind.hpp>

#if defined WIN32
#include <WinBase.h>
#endif

serialCommunicator::serialCommunicator()
{
	newSerial = new boost::asio::serial_port(IO_service);

	pTimer = new boost::asio::deadline_timer(IO_service);

	serialBreakms = 250;
	writeError, readError = false;
}

serialCommunicator::~serialCommunicator()
{
	SerialClose();
	//delete serial;

	delete newSerial;
	delete pTimer;
	//delete ptr_io_service;
}

void serialCommunicator::SerialClose()
{
	//serial->close();
	newSerial->close();
}

int serialCommunicator::SerialOpen(unsigned Port, unsigned long BaudRate, unsigned Format,
					bool RtsCts, unsigned long SerialBreakDelay)
{

	// Dummy Function - this should not be used - TO BE REMOVED
	return -1;	// Always returns an error
}

int serialCommunicator::SerialOpen(const std::string &portName)
{

	// Try and open serial port
	newSerial->open(portName);

	// Try to set defaults
	return SerialSetDefaultOptions();
}

int serialCommunicator::SerialSetDefaultOptions()
{
	
	if( newSerial->is_open() )
	{
		// Set baud rate
		boost::asio::serial_port_base::baud_rate baudRateOption(DEFAULT_BAUD_RATE);
		newSerial->set_option(baudRateOption);

		// Set parity
		boost::asio::serial_port_base::parity parityOption(boost::asio::serial_port_base::parity::none);	// Default no parity
		newSerial->set_option(parityOption);

		// Set stop bits
		boost::asio::serial_port_base::stop_bits stopBitsOption(boost::asio::serial_port_base::stop_bits::one);	// Deafult one bit
		newSerial->set_option(stopBitsOption);

		// Set size of data bits
		boost::asio::serial_port_base::character_size dataBitsOptions(boost::asio::serial_port_base::character_size::character_size());	// Default 8 bits
		newSerial->set_option(dataBitsOptions);

		// Set flow control (hardware handshaking)
		boost::asio::serial_port_base::flow_control flowControlOption(boost::asio::serial_port_base::flow_control::none);	// Default no flow control
		newSerial->set_option(flowControlOption);

		return 1;
	}
	else return 0;
}

int serialCommunicator::SerialSetBaud(unsigned int baudRate)
{
	// Set baud rate
	boost::asio::serial_port_base::baud_rate baudRateOption(baudRate);
	newSerial->set_option(baudRateOption);
	return 1;
}

int serialCommunicator::SerialBreak()
{

#if defined WIN32

	// Asio library doesn't support serial breaks on windows, so use win api
	boost::asio::deadline_timer timer(IO_service);

	// Set serial break
	SetCommBreak(newSerial->native_handle());
	
	// Wait for defined period
	timer.expires_from_now(boost::posix_time::milliseconds(serialBreakms));
	timer.wait();	// Blocks till the timer expires

	// Clear serial break
	ClearCommBreak(newSerial->native_handle());

#else 
	// Otherwise send using the asio interface -NOTE this hasn't been tested yet.
	newSerial->send_break();
#endif

	return 1;
}

int serialCommunicator::SerialFlush()
{
	//// Check to see if port is working correctly
	//if( serial->error() ) return -1;

	//// Flush input buffer
	//if( serial->flush() ) return 1;
	//else return -1;

	// Using the asio library this function should not be needed
	return 1;
}

int serialCommunicator::SerialPutChar( unsigned char uch, unsigned int msTimeout )
{
	std::string character(1, uch);

	return SerialPutString(character, character.length(), msTimeout);
}

//int serialCommunicator::SerialPutString( QString &string, unsigned long len )
int serialCommunicator::SerialPutString( std::string &string, unsigned long len, unsigned int msTimeout )
{

	// Reset service to deal with being cancelled or error
	newSerial->get_io_service().reset();

	// Start write operation
	boost::asio::async_write(*newSerial, boost::asio::buffer(string, string.length()), 
		boost::bind(&serialCommunicator::SerialWriteComplete, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	// Set timer going
	pTimer->expires_from_now(boost::posix_time::milliseconds(msTimeout));
	pTimer->async_wait(boost::bind(&serialCommunicator::SerialTimeOut, this, boost::asio::placeholders::error));

	// Block until either the operation times out or completes
	newSerial->get_io_service().run();

	if(!writeError) return 1;
	else return SERIAL_WRITE_ERROR;
}

int serialCommunicator::SerialGetChar( unsigned int msTimeout )
{
	// Temp variable to store result
	char res;

	// Reset service to deal with being cancelled or error
	newSerial->get_io_service().reset();

	// Start read operation
	boost::asio::async_read(*newSerial, boost::asio::buffer(&res, 1),
		boost::bind(&serialCommunicator::SerialReadComplete, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	// Set timer going
	pTimer->expires_from_now(boost::posix_time::milliseconds(msTimeout));
	pTimer->async_wait(boost::bind(&serialCommunicator::SerialTimeOut, this, boost::asio::placeholders::error));

	// Block until either the operation times out or completes
	newSerial->get_io_service().run();

	if(!readError) return res;
	else return SERIAL_READ_ERROR;
}

int serialCommunicator::SerialCharsAvailable()
{
	// Return number of bytes available
	/*return serial->bytesAvailable();*/
	return 0;
}

int serialCommunicator::SerialGetString(char *pStr, unsigned long maxLen)
{
	// Return number of bytes read
	//return serial->read(pStr, maxLen);
	return 0;	// This function is not used
}

int serialCommunicator::SerialWaitForResponse( int timeoutMSec )
{
	// Wait for response, if timeout return a error
	//if( serial->waitForReadyRead(timeoutMSec) ) return 1;
	//else return -1;
	return 1;
}

int serialCommunicator::SerialWaitForSend( int timeoutMSec )
{
	// Wait for response, if timeout return a error
	//if( serial->waitForBytesWritten(timeoutMSec) ) return 1;
	//else return -1;
	return 1;
}

int serialCommunicator::SerialSetHardwareHandshaking( int hardwareHandshake )
{
	if( hardwareHandshake )
	{
		// Set flow control (hardware handshaking)
		boost::asio::serial_port_base::flow_control flowControlOption(boost::asio::serial_port_base::flow_control::hardware);
		newSerial->set_option(flowControlOption);
		return 1;
	}
	else
	{
		boost::asio::serial_port_base::flow_control flowControlOption(boost::asio::serial_port_base::flow_control::none);	// Default no flow control
		newSerial->set_option(flowControlOption);
		return 1;
	}
	
	// function will return error if any of the above fails
	return 0;
}

void serialCommunicator::SerialWriteComplete(  const boost::system::error_code &error, size_t bytes_written )
{
	// Set bool to see if there was an error
	writeError = (error || bytes_written == 0 );

	if(writeError) std::cout << "Write: " << error.message() <<std::endl;
	// Cancel timer as write command has completed
	pTimer->cancel();
}

void serialCommunicator::SerialTimeOut( const boost::system::error_code &error )
{
	// Timeout was cancelled and therefore the operation completed
	if(error) return;
	else 
	{
		std::cout << "Serial Timeout!" << std::endl;
		newSerial->cancel();	// Else timed out so cancel all operations
	}
}

void serialCommunicator::SerialReadComplete(  const boost::system::error_code &error, size_t bytes_read )
{
	// Set bool to see if there was an error
	readError = (error || bytes_read == 0 );

	//if(readError) std::cout << "Read: " << error.message() <<std::endl;

	// Cancel timer as write command has completed
	pTimer->cancel();
}

