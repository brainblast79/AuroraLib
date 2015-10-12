// Header for defining the serial thread class

//#include <QThread.h>
//#include <qmutex.h>
#include "CommandHandling.h"
#include "serialCommunicator.h"
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#pragma once

// Define constants
const int MAX_NUM_OF_SENSORS = 4; 
const int BUFFER_SIZE = 4096;
const int LOG_BUFFER_SIZE = 1024;

// Define a single unit in the buffer, i.e. one whole set of sensor data
// Note boost::array is used so typedef does not decay into a pointer (awkward syntax)
typedef boost::array<Position3d, MAX_NUM_OF_SENSORS> bufferUnit;

// Log file buffer has frame number as well
typedef struct logFileBufferTypeStruct
{
	unsigned long frameNumber;
	bufferUnit sensorData;
} logBufferUnit;


class serialThread
{

public:
	// Constructor, destructor
	serialThread();
	~serialThread();

	// Interface commands
	bool connectToAurora(std::string &portName, std::string &baudRate, bool hardwareHandshake);
	bool resetAurora(std::string &portName);
	void activateSensors();
	void startTracking();
	void stopTracking();
	int getNumOfSensors();

	// Log file commands
	void setLogFile(const std::string &logFile);
	std::string getLogFile();

	// Retrieve sensor data for the controller
	void getSensorData(std::vector<logBufferUnit> &sensorDataStore);
	
	// Check for broken sensors
	bool anyBrokenSensors();
	void getSensorsStatus(boost::array<bool, MAX_NUM_OF_SENSORS> &sensorDataValid);

protected:
	void stop();
	void runTracking();
	void writeSensorDataToLogFile();

private:
	volatile bool stopTrackingFlag;
	volatile bool stopLoggingFlag;
	void connectToCOMPort();
	void setCurrentSensorData();

	void setNumOfSensors();
	CCommandHandling SerialCommands;
	serialCommunicator SerialPort;
	int numSensors;
	std::string logFileName;

	unsigned long currentFrameNumber;

	volatile bool brokenSensors;
	boost::array<bool, MAX_NUM_OF_SENSORS> sensorsValid;
	//Position3d currentSensorData[MAX_NUM_OF_SENSORS];

	//logBufferUnit currentSensorDataLog;

	boost::mutex stopTrackMutex;
	boost::mutex stopLogMutex;
	boost::mutex sensorStatusMutex;
	//boost::mutex serialMutex;	// Shouldn't need this, should disable sending commands when tracking

	// Threads
	boost::thread TrackingThread;
	boost::thread LoggingThread;
	
	// Data Buffers - these are circular(rings) buffers
	boost::lockfree::spsc_queue<logBufferUnit, boost::lockfree::capacity<LOG_BUFFER_SIZE>> LoggingCircularBuffer;
	boost::lockfree::spsc_queue<logBufferUnit, boost::lockfree::capacity<BUFFER_SIZE>> ControllerCircularBuffer;
};