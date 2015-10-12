#include "serialThread.h"
#include <iostream>
#include <fstream>

serialThread::serialThread()
{
	// Set class variables
	brokenSensors = false;
	stopTrackingFlag = false;
	stopLoggingFlag = false;
	numSensors = 0;
	currentFrameNumber = 0;

	// Give a COM port to the command handling class
	SerialCommands.setCOMPort(SerialPort);
}

serialThread::~serialThread()
{
	stopTracking();	// Make sure everything has stopped when we exit
}

void serialThread::runTracking()
{	
		for(;;)
		{
			// Create an interruption period
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
			
			// Check to see if we need to stop the thread
			stopTrackMutex.lock();
			if( stopTrackingFlag ) break;
			stopTrackMutex.unlock();

			// Get sensor data
			if( SerialCommands.nGetBXTransforms(false) == 1 )
			{			
				// Set local copy of current data
				setCurrentSensorData();
			}
		}
		
		stopTrackMutex.unlock();	// Make sure stopMutex is unlocked
		std::cout << "Serial Tracking thread terminated!" << std::endl;
}

void serialThread::stop()
{
	std::cout << "Attempting to stop the  Serial threads..." << std::endl;
	
	// Lock mutex
	boost::lock_guard<boost::mutex> lockTrack(stopTrackMutex);
	stopTrackingFlag = true;

	boost::lock_guard<boost::mutex> lockLog(stopLogMutex);
	stopLoggingFlag = true;

	std::cout << "Set Flags" << std::endl;
}

bool serialThread::connectToAurora(std::string &portName, std::string &baudRate, bool hardwareHandshake)
{
	// Reset to give clean slate
	if( !serialThread::resetAurora(portName) ) return false;

	std::cout << "Setting system params!" << std::endl;
	
	// Set system (Aurora) baud rate
	SerialCommands.nSetSystemComParms( atoi(baudRate.c_str()), hardwareHandshake );

	std::cout << "Setting comp params!" << std::endl;
	
	// Set computer rate
	SerialCommands.nSetCompCommParms( atoi(baudRate.c_str()), hardwareHandshake );

	std::cout << "Initialising System!" << std::endl;
	SerialCommands.nInitializeSystem();

	return true;
}

bool serialThread::resetAurora(std::string &portName)
{
	// Close all COM ports
	SerialCommands.nCloseComPorts();

	// Open COM port
	if( !SerialCommands.nOpenComPort(portName))
	{
		std::cout << "Failed to open COM port!" << std::endl;
		return false;
	}
	
	// Reset Aurora
	if( !SerialCommands.nHardWareReset(false) )
	{
		std::cout << "Failed to reset Aurora!" << std::endl;
		return false;
	}

	return true;
}

int serialThread::getNumOfSensors()
{
	return numSensors;
}

void serialThread::activateSensors()
{
	// Activate sensor handles and enable them
	SerialCommands.nActivateAllPorts();

	// Get the number of sensors
	setNumOfSensors();
}

void serialThread::setNumOfSensors()
{
	numSensors = SerialCommands.GetNumEnabledHandles();
}

void serialThread::setLogFile(const std::string &logFile)
{
	logFileName = logFile;
}

std::string serialThread::getLogFile()
{
	return logFileName;
}

void serialThread::startTracking()
{
	// Send command to Aurora to start tracking
	SerialCommands.nStartTracking();

	// Start a tracking thread
	TrackingThread = boost::thread(&serialThread::runTracking, this);

	// Start a logging thread
	LoggingThread = boost::thread(&serialThread::writeSensorDataToLogFile, this);

}

void serialThread::stopTracking()
{
	// Tell recording thread that we are to stop tracking, will block till thread terminates
	stop();

	std::cout << "waiting for serial threads to terminate..." << std::endl;
	
	// Wait for threads to terminate
	TrackingThread.join();
	LoggingThread.join();

	// Tell aurora to stop tracking
	SerialCommands.nStopTracking();

	// After everything has terminated set flag back to false state
	stopTrackingFlag = false;
	stopLoggingFlag = false;

	std::cout << "Serial Threads stopped!" << std::endl;
}

void serialThread::setCurrentSensorData()
{
	// Local variables for storing the data instant
	logBufferUnit currentSensorDataLog;
	bufferUnit  currentSensorData;

	// Current handle holder
	int currentHandle;

	// Set the number of sensors
	numSensors = SerialCommands.ActivatedPortHandles.size();

	// Iterate over the activated port handles and set the postion data
	for( int i = 0; i != numSensors; ++i )
	{
		// If the number of sensors is greater than that supported just take first ones up to the max num.
		if( i > MAX_NUM_OF_SENSORS ) break;		

		currentHandle = SerialCommands.ActivatedPortHandles[i];

		// Note no check is made to see if the data is valid, BAD FLOAT will be set if the data is not valid
		currentSensorData[i].x = SerialCommands.m_dtHandleInformation[currentHandle].Xfrms.translation.x;
		currentSensorData[i].y = SerialCommands.m_dtHandleInformation[currentHandle].Xfrms.translation.y;
		currentSensorData[i].z = SerialCommands.m_dtHandleInformation[currentHandle].Xfrms.translation.z;

		// Check to see if sensor is broken
		bool broken = SerialCommands.m_dtHandleInformation[currentHandle].HandleInfo.bBrokenSensor;
		brokenSensors = brokenSensors || broken;

		// Set sensor status
		boost::lock_guard<boost::mutex> lock(sensorStatusMutex);
		sensorsValid[i] = !broken;
	}

	// Fill out the rest of the data with zeros for a bit of safety :)
	if( numSensors < MAX_NUM_OF_SENSORS )
	{
		for( int i = numSensors; i != MAX_NUM_OF_SENSORS; ++i)
		{
			currentSensorData[i].x = 0;
			currentSensorData[i].y = 0;
			currentSensorData[i].z = 0;

			boost::lock_guard<boost::mutex> lock(sensorStatusMutex);
			sensorsValid[i] = false;
		}
	}

	// Copy the data over to the log buffer
	currentSensorDataLog.sensorData = currentSensorData;

	// Set frame number
	if( numSensors > 0 ) 
	{
		currentFrameNumber = SerialCommands.m_dtHandleInformation[currentHandle].Xfrms.ulFrameNumber;
		currentSensorDataLog.frameNumber = SerialCommands.m_dtHandleInformation[currentHandle].Xfrms.ulFrameNumber;
	}

	// Push the data to the buffers
	LoggingCircularBuffer.push(currentSensorDataLog);
	ControllerCircularBuffer.push(currentSensorDataLog);

}

void serialThread::writeSensorDataToLogFile()
{
	// Local variables
	std::ofstream logFileStream;	
	logBufferUnit latestSensorData;

	// Open log file
	logFileStream.open(logFileName, std::ofstream::app);

	if( logFileStream.is_open() )
	{
		// Get current time
		boost::posix_time::ptime currentTime = boost::posix_time::microsec_clock::local_time();
		long milliseconds = currentTime.time_of_day().total_milliseconds();

		// Display a number representing the time started
		logFileStream << "Starting Time:" << milliseconds << std::endl;

		// Close log file
		logFileStream.close();
	}


	for(;;)
	{
		// Create a short break between logging sessions - sleep to wait for data
		boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

		// Check to see if we need to stop the thread
		stopLogMutex.lock();
		if( stopLoggingFlag ) break;
		stopLogMutex.unlock();

		while( LoggingCircularBuffer.pop(latestSensorData) )	// Continue until all the latest data has been recorded
		{

			// Open log file
			logFileStream.open(logFileName, std::ofstream::app);

			if( logFileStream.is_open() )	// Checked if file opened correctly
			{
				// Put frame number
				logFileStream << latestSensorData.frameNumber << "\t";

				for( int i = 0; i != numSensors; ++i )
				{
					logFileStream << latestSensorData.sensorData[i].x << "\t" << latestSensorData.sensorData[i].y << "\t" << latestSensorData.sensorData[i].z << "\t";
				}

				// Add new line at the end
				logFileStream << std::endl;

				// Close log file
				logFileStream.close();
			}
			else
			{
				std::cout << "Error opening sensor log file!" << std::endl;
			}
		}
	}

	std::cout << "Serial Logging thread stopped!" << std::endl;

	stopLogMutex.unlock();	// Make sure mutex is unlocked
}

void serialThread::getSensorData(std::vector<logBufferUnit> &sensorDataStore)
{
	// Temporary storage variable
	logBufferUnit latestSensorData;

	// Get all the data that hasn't been stored yet
	while( ControllerCircularBuffer.pop(latestSensorData) )
	{
		sensorDataStore.push_back(latestSensorData);
	}
}

bool serialThread::anyBrokenSensors()
{
	// Return the flag
	return brokenSensors;
}

void serialThread::getSensorsStatus(boost::array<bool, MAX_NUM_OF_SENSORS> &sensorDataValid)
{
	// Wait till mutex becomes available and then status'
	boost::lock_guard<boost::mutex> lock(sensorStatusMutex);
	sensorDataValid = sensorsValid;
}