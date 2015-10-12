#include "CommandHandling.h"
#include "Conversions.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/thread/thread.hpp>

extern int bFirst;
extern unsigned int CrcTable[256];

/*****************************************************************
Name:			CCommandHandling	

Inputs:
	None.

Return Value:
	None.

Description:   CCommandHandling Constructor
*****************************************************************/
CCommandHandling::CCommandHandling()
{
	/* set up com port class, start from new. */
	pCOMPort = NULL;
	//pCOMPort = pSerial;

	/* reinitialize the com ports */
	for ( int i = 0; i < NUM_COM_PORTS; i++ )
	{
		bComPortOpen[i] = 0;
	}

	/* set variables */
	m_bLogToFile = false;
	m_bDateTimeStampFile = false;

	/* variable intialization */
 	m_bClearLogFile = false;
	m_bDisplayErrorsWhileTracking = false;

	m_nRefHandle = -1;

	m_nTimeout = 3;
	m_nDefaultTimeout = 10;
	waitForResponse = 500;

} /* CCommandHandling()

/*****************************************************************
Name:			~CCommandHandling	

Inputs:
	None.

Return Value:
	None.

Description:   CCommandHandling Destructor
*****************************************************************/
CCommandHandling::~CCommandHandling()
{
	/* clean up */
	//if ( pCOMPort )
	//	delete( pCOMPort );
} /* ~CCommandHandling */

void CCommandHandling::setCOMPort(serialCommunicator &COMPort)
{
	pCOMPort = &COMPort;
}


/*****************************************************************
Name:				nInitializeSystem

Inputs:
	None.

Return Value:
	int - 0 if fails, else nCheckResponse

Description:   This routine initializes the System by sending the
			   INIT command.  Remember to reset the appropriate 
			   variables.
*****************************************************************/
int CCommandHandling::nInitializeSystem()
{

	/* send the message */
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "INIT " );

	if(nSendMessage( m_szCommand, true ))
	{
		if ( nGetResponse( ) )
			return nCheckResponse( nVerifyResponse(m_szLastReply, true) );
	} /* if */

	return 0;
} /* nInitializeSystem */


/*****************************************************************
Name:				nSendMessage

Inputs:
	char * m_szCommand - the command string, coming in.  The command
						 to be sent to the System
	bool bAddCRC - if true, we add the CRC to the command and replace the
				   space with the :

Return Value:
	int -  0 if fails, 1 if passes.

Description:   
	This command takes in a command string and parses it depending
	on the value of bAddCRC.  If bAddCRC is true, we replace the 
	space with a : and calculate and add the CRC to the command.
	We then send the command to the System.
*****************************************************************/
int CCommandHandling::nSendMessage( char *m_szCommand, bool bAddCRC )
{
	unsigned int 
		i;
	bool 
		bComplete = false;

	/* Check COM port */
	if( pCOMPort == NULL )
	{
		return bComplete;
	}/* if */

	m_nTimeout = nLookupTimeout( m_szCommand );

	/* build the command, by adding a carraige return to it and crc if specified */
	if (!nBuildCommand( m_szCommand, bAddCRC ))
		return bComplete;

	if(strlen(m_szCommand) >= (MAX_COMMAND_MSG))
	{
		return bComplete;
	} /* if */

	for( i = 0; i < strlen(m_szCommand); i++ )
	{
		if ( pCOMPort->SerialPutChar( m_szCommand[i] ) == SERIAL_WRITE_ERROR )
		{
			bComplete = false;
			break;
		} /* if */
		else if( m_szCommand[i] == CARRIAGE_RETURN )
		{
			bComplete = true;
			break;
		}/* if */
	} /* for */

	return bComplete;
} /* nSendMessage */

/*****************************************************************
Name:				nCheckResponse

Inputs:
	int nReturnedValue - the value returned by nVerifyResponse

Return Value:
	int - 1 if the response is valid, 0 if the response is invalid
		  or an error.

Description:   
	This routine checks the value from nVerifyResponse.
	The following occurs:
	REPLY_ERROR - the response from the system was an error, we
				  beep the system if required and post the error
				  message ( ErrorMessage() )
	REPLY_BADCRC - a bad crc was returned with the response
				   i.e. the crc returned doesn't match the one
				   calculated for the response. Post a message
	REPLY_WARNING - the warning message was recieve from the system
					while intializing a tool (see API for reasons)
					post a message and beep if required.
	REPLY_INVALID - an invalid response was received from the system
					post a message
*****************************************************************/
int CCommandHandling::nCheckResponse( int nReturnedValue )
{
	// Create message box in order to show any messages
	//QMessageBox mesgBox;
	//mesgBox.setIcon(QMessageBox::Warning);

	if ( nReturnedValue == REPLY_ERROR )
	{
		//ErrorMessage();
		//mesgBox.setWindowTitle("Error");
		//mesgBox.setText("REPLY ERROR");
		//mesgBox.exec();
		//QMessageBox::warning( NULL, "Error", "REPLY ERROR");
		std::cout << "REPLY ERROR!" << std::endl;
		return 0;
	} /* if */

	if ( nReturnedValue == REPLY_BADCRC )
	{
		//mesgBox.setWindowTitle("CRC Error");
		//mesgBox.setText("Bad CRC received with reply.");
		//mesgBox.exec();
		//QMessageBox::warning( NULL, "CRC Error", "Bad CRC received with reply.");
		//MessageBox( NULL, "Bad CRC received with reply.", "CRC Error", MB_ICONERROR|MB_SYSTEMMODAL|MB_SETFOREGROUND );
		std::cout << "Bad CRC received with reply!" << std::endl;
		return 0;
	} /* if */

	if ( nReturnedValue == REPLY_WARNING )
	{
		//WarningMessage();
		//mesgBox.setWindowTitle("Warning");
		//mesgBox.setText("REPLY WARNING");
		//mesgBox.exec();
		//QMessageBox::warning( NULL, "Warning", "REPLY WARNING");
		std::cout << "REPLY WARNING!" << std::endl;
		return 1;

	} /* if */

	if ( nReturnedValue == REPLY_INVALID )
	{
		//MessageBox( NULL, "Invalid response received from the system", 
		//			"Invalid Response", MB_ICONERROR|MB_SYSTEMMODAL|MB_SETFOREGROUND );
		//mesgBox.setWindowTitle("Reply Invalid");
		//mesgBox.setText("Invalid response received from the system");
		//mesgBox.exec();
		//QMessageBox::warning( NULL, "Reply Invalid", "Invalid response received from the system");
		std::cout << "Invalid response received from the system!" << std::endl;
		return 0;
	} /* if */

	return 1;
} /* nCheckResponse */

/***********************************************************
  Name: nLookupTimeout

  Returns: int 
  Argument: char *szCommand

  Description: Looks up the command in the Timeout value list
				(m_dtTimeoutValues) and returns the timeout 
				value for the specified command
***********************************************************/
int CCommandHandling::nLookupTimeout(char *szCommand)
{

//	if( m_dtSystemInformation.nTypeofSystem == AURORA_SYSTEM )
		return m_nDefaultTimeout;
}

/*****************************************************************
Name:				

Inputs:
	None.

Return Value:
	int - 0 if it fails, nCheckResponse if passes

Description:   
	This routine sends a serial break to the system, reseting it.
*****************************************************************/
int CCommandHandling::nHardWareReset(bool bWireless)
{
	int
		nResponse = 0,
		nInitTO = 3; 

	/* Check COM port */
	if( pCOMPort == NULL )
	{
		return 0;
	}/* if */

	if( !bWireless )
	{
		/* send serial break */
		pCOMPort->SerialBreak();

		/* Give the break sometime to set */
		//Sleep(500); 
		
		//QTime t;
		//t.start();

		//while( t.elapsed() < 500 );

		boost::this_thread::sleep_for(boost::chrono::milliseconds(500));

		memset(m_szCommand, 0, sizeof(m_szCommand));
		if (!nGetResponse( ))
		{
			return 0;
		}/* if */

		/* check for the RESET response */
		nResponse = nVerifyResponse(m_szLastReply, TRUE);
		if ( !nCheckResponse( nResponse ) )
		{
			return 0;
		}/* if */

		/*
		 * In order to support NDI enhanced Tool Interface Unit and Tool Docking Station,
		 * a time delay is recommended so that the Tool Docking Station
		 * can be recognised by the eTIU.
		 */
		//Sleep(nInitTO * 1000);

		//t.restart();
		//while( t.elapsed() < (nInitTO * 1000));

		if ( nResponse & REPLY_RESET )
		{
			if (!SystemCheckCRC( m_szLastReply ) )
				return REPLY_BADCRC;
			else
				return nResponse;
		} /* if */
		else
		{
			return nResponse;
		}/* else */
	} /* if */
	else
	{
		memset(m_szCommand, 0, sizeof(m_szCommand));
		sprintf( m_szCommand, "RESET 0" );
		if (nSendMessage( m_szCommand, TRUE ))
			if (nGetResponse( ))
				return nCheckResponse( nVerifyResponse(m_szLastReply, TRUE) );
		return 0;
	} /* else */
} /* nHardwareReset */

/*****************************************************************
Name:				nSetSystemComParms

Inputs:
	int nBaudRate - the baud rate to set
	int nDateBits - the data bit setting
	int nParity - the parity setting
	int nStopBits - the stop bit setting
	int nHardware - whether or not to use hardware handshaking

Return Value:
	int - 0 if fails, else nCheckResponse

Description:   
	This routine sets the systems com port parameters, remember
	to immediatley set the computers com port settings after this
	routine is called.
*****************************************************************/
int CCommandHandling::nSetSystemComParms( int nBaudRate,
										  int nHardware,
										  int nDataBits, 
										  int nParity, 
										  int nStopBits)
{
	int nBaud = 0;
	
	// Ugly
	switch( nBaudRate )
	{
		case 9600:
			nBaud = 0;
			break;
		case 14400:
			nBaud = 1;
			break;
		case 19200:
			nBaud = 2;
			break;
		case 38400:
			nBaud = 3;
			break;
		case 57600:
			nBaud = 4;
			break;
		case 115200:
			nBaud = 5;
			break;
		default:
			nBaud = 0;
			break;
	}

	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "COMM %d%d%d%d%d", nBaud, 
											nDataBits, 
											nParity, 
											nStopBits, 
											nHardware );
	if (nSendMessage( m_szCommand, TRUE ))
		if (nGetResponse( ))
			return nCheckResponse( nVerifyResponse(m_szLastReply, TRUE) );

	return 0;
} /* nSetSystemComParms */
/*****************************************************************
Name:				nSetCompCommParms

Inputs:
	int nBaud - the baud rate to set
	int nDateBits - the data bit setting
	int nParity - the parity setting
	int nStop - the stop bit setting
	int nHardware - whether or not to use hardware handshaking

Return Value:
	int - 0 if fails, else 1

Description:   
	This routine sets the computer's com port parameters, remember
	to immediatley set the computer's com port settings after the 
	system's com port parameters.
*****************************************************************/
int CCommandHandling::nSetCompCommParms( int nBaud,
										 int nFlowControl,
					 				     int nDataBits,
									     int nParity,
									     int nStop)
{

	/* Check COM port */
	if( pCOMPort == NULL ) return 0;
	
	if ( pCOMPort->SerialSetBaud( nBaud ) && 
		pCOMPort->SerialSetHardwareHandshaking(nFlowControl) ) return 1;

	return 0;
} /* nSetCompCommParms */

/*****************************************************************
Name:				nOpenComPort

Inputs:
	int nPort - the port number to be opened

Return Value:
	int - 1if successful, 0 otherwise

Description:   
	This routine opens the selected com port and sets its settings
	to the default communication parameters
*****************************************************************/
int CCommandHandling::nOpenComPort( int nPort )
{
	/*
	 * If the COM Port is open there is no sense in re-opening it.
	 * You can still change the PARAMETERS with no problem.  This
	 * reduces the time it takes to re-initialize the System
	 */
	if ( bComPortOpen[nPort] )
		return 1;
	else
	{
		if ( pCOMPort != NULL )
		{
			/* set the parameters to the defaults */
			if ( pCOMPort->SerialOpen( nPort, 9600, 1, FALSE, 256 ) )
			{
				bComPortOpen[nPort] = TRUE;
				return 1;
			} /* if */ 
		} /* if */
	} /* else */

	return 0;
} /* nOpenComPort */

int CCommandHandling::nOpenComPort( const std::string &Port )
{
	/*
	 * If the COM Port is open there is no sense in re-opening it.
	 * You can still change the PARAMETERS with no problem.  This
	 * reduces the time it takes to re-initialize the System
	 */
	if ( std::find(openCOMPorts.begin(), openCOMPorts.end(), Port) != openCOMPorts.end() )
	{
		pCOMPort->SerialSetBaud( 9600 );	// Set the COM port back to default baudrate
		return 1;
	}
	else
	{
		if ( pCOMPort != NULL )
		{
			/* set the parameters to the defaults */
			if ( pCOMPort->SerialOpen( Port ) )
			{
				openCOMPorts.push_back(Port);
				return 1;
			} /* if */ 
		} /* if */
	} /* else */

	return 0;
} /* nOpenComPort */

/*****************************************************************
Name:				nCloseComPorts

Inputs:
	None.

Return Value:
	int , 0 if fails and 1 is passes

Description: 
	This routine closes all open COM ports.   
*****************************************************************/
int CCommandHandling::nCloseComPorts()
{
	/*
	 * if the COM Port is already closed no need to check to see if 
	 * we should close it
	 */

	if( (!openCOMPorts.empty()) && (pCOMPort != NULL) )
	{
		pCOMPort->SerialClose();
		openCOMPorts.clear();
		return 1;
	}

	return 0;
} /* nCloseComPorts */

int CCommandHandling::nVerifyResponse( char *pszReply, bool bCheckCRC )
{
	int
		nResponse = 0;

	/* verify the response by comparing it with the possible responses */
	if (!strnicmp(pszReply, "RESET",5))
		nResponse = REPLY_RESET;
	else if (!strnicmp(pszReply, "OKAY",4))
		nResponse = REPLY_OKAY;
	else if (!strnicmp(pszReply, "ERROR",5))
		nResponse = REPLY_ERROR;
	else if (!strnicmp(pszReply, "WARNING",7))
		nResponse = REPLY_WARNING;
	else if ( strlen( pszReply ) > 0 )
		nResponse = REPLY_OTHER;
	else
		return REPLY_OTHER;

	if ( nResponse & REPLY_OKAY  || nResponse & REPLY_OTHER && bCheckCRC )
	{
		if (!SystemCheckCRC( pszReply ) )
			return REPLY_BADCRC;
		else
			return nResponse;
	} /* if */
	else
		return nResponse;
} /* nVerifyResponse */

/*****************************************************************
Name:				nGetResponse

Inputs:
	None.

Return Value:
	int - 1 if passes, else 0

Description:   
	This routine gets the response from the system that is to be 
	poled through the com port.  The routine gets it start time 
	( the time it started polling ) and continues until one of two
	things happens.  First, if the end of response (Carriage Return)
	if found the reply is complete and 1 is returned.  If the time
	it takes to get a response exceeds the timeout value, the system
	assumes no response is coming and timeouts.  The timeout dialog
	is then displayed.  Use this response routine for all calls except
	the BX call.
*****************************************************************/
int CCommandHandling::nGetResponse()
{
	char
		chChar;

	bool bDone = FALSE;
	int 
		nCount = 0,
		nRet = 0,
		nRetry = 0;


    memset(m_szLastReply, 0, sizeof( m_szLastReply ) );

	do
	{
		/* Check COM port */
		if( pCOMPort == NULL )
		{
			return FALSE;
		}/* if */

        chChar = pCOMPort->SerialGetChar();

		// Check for error
		if( chChar == NULL ) return false;

		/* if carriage return, we are done */
		if ( chChar == '\r' )
   		{
			m_szLastReply[nCount] = CARRIAGE_RETURN;
			m_szLastReply[nCount+1] = '\0';
			bDone = TRUE;
		} /* if */
		else
		{
			m_szLastReply[nCount] = chChar;
			nCount++;
		} /* else */
		
	} while ( !bDone );


	return 1;
} /* nGetResponse */

/*****************************************************************
Name:				nActivateAllPorts

Inputs:
	None.

Return Value:
	int - 1 if successful, 0 otherwise

Description:    This is the routine that activates all ports using
				
*****************************************************************/
int CCommandHandling::nActivateAllPorts()
{
	if (!nFreePortHandles())
		return 0;

	if (!nInitializeAllPorts())
		return 0;

	if (!nEnableAllPorts())
		return 0;

	return 1;
} /* nActivateAllPorts */

int CCommandHandling::nFreePortHandles()
{
	int
		nNoHandles = 0,
		nHandle = 0,
		n = 0;
	char
		szHandleList[MAX_REPLY_MSG];

	/* get all the handles that need freeing */
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "PHSR 01" );

	if(nSendMessage( m_szCommand, TRUE ))
	{
		if (!nGetResponse( ))
			return 0;
		if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
			return 0;

		sprintf(szHandleList, m_szLastReply);
		nNoHandles = uASCIIToHex(&szHandleList[n], 2);
		n+=2;
		for ( int i = 0; i < nNoHandles; i++ )
		{
			nHandle = uASCIIToHex( &szHandleList[n], 2 );
			memset(m_szCommand, 0, sizeof(m_szCommand));
			sprintf( m_szCommand, "PHF %02X", nHandle);
			n+=5;
			if (!nSendMessage( m_szCommand, TRUE ))
				return 0;
			if ( !nGetResponse() )
				return 0;
			if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
				return 0;
			m_dtHandleInformation[nHandle].HandleInfo.bInitialized = FALSE;
			m_dtHandleInformation[nHandle].HandleInfo.bEnabled = FALSE;
			/* EC-03-0071 */
			memset(m_dtHandleInformation[nHandle].szPhysicalPort, 0, 5);
		} /* for */
		return 1;
	} /* if */

	return 0;
} /* nFreePortHandles */

/*****************************************************************
Name:				nInitializeAllPorts

Inputs:
	None.

Return Value:
	int - 1 is successful, 0 otherwise

Description:   This routine intializes all the ports using the
			   PINIT call.  It also makes calls to the PVWR routine
			   and TTCFG routine to virtual load tool definitions.
*****************************************************************/
int CCommandHandling::nInitializeAllPorts()
{
	int
		i = 0,
		nRet = 0,
		nPhysicalPorts = 0,
		nNoHandles = 0,
		nHandle = 0,
		n = 0;
	char
		pszINISection[25],
		pszPortID[8],
		szHandleList[MAX_REPLY_MSG],
		szErrorMessage[50];

	do
	{
		n = 0;
		/* get the handles that need to be initialized */
		memset(m_szCommand, 0, sizeof(m_szCommand));
		sprintf( m_szCommand, "PHSR 02" );

		if (!nSendMessage( m_szCommand, TRUE ))
			return 0;

		if (!nGetResponse( ))
			return 0;

		if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
			return 0;

		sprintf( szHandleList, m_szLastReply );
		nNoHandles = uASCIIToHex( &m_szLastReply[n], 2 );
		n+=2;

		if ( nNoHandles > 0 )
		{
			// active
			nPhysicalPorts = (m_dtSystemInformation.nNoActivePorts > 0 ?
							  m_dtSystemInformation.nNoActivePorts :
							  m_dtSystemInformation.nNoMagneticPorts > 0 ?
							  m_dtSystemInformation.nNoMagneticPorts : 0);

			if ( m_dtSystemInformation.nNoActivePorts == 4 )
				nPhysicalPorts = 12;

			//sprintf( pszINISection, m_dtSystemInformation.nNoActivePorts > 0 ?
			//						"POLARIS SROM Image Files\0" :
			//						m_dtSystemInformation.nNoMagneticPorts > 0 ?
			//						"AURORA SROM Image Files\0" : "" );

			for ( int i = 0; i < nNoHandles; i++ )
			{
				nHandle = uASCIIToHex( &szHandleList[n], 2 );
				if ( !nGetPortInformation( nHandle ) )
					return 0;

				if ( !m_dtHandleInformation[nHandle].HandleInfo.bInitialized )
				{
					if (!nInitializeHandle( nHandle ))
					{
						/* Inform user which port fails on PINIT */
						//sprintf(szErrorMessage, "Port %s could not be initialized.\n"
						//"Check your SROM image file settings.", m_dtHandleInformation[nHandle].szPhysicalPort );
						//MessageBox( NULL,  szErrorMessage,
						//"PINIT ERROR", MB_ICONERROR|MB_SYSTEMMODAL|MB_SETFOREGROUND );
						
						//QMessageBox mesgBox;
						//mesgBox.setWindowTitle("Error");
						//mesgBox.setText("Cannot initialise handle (sensor port)!");
						//mesgBox.exec();

						std::cout << "Cannot initialise handle (sensor port)!" << std::endl;
						
						return 0;
					}/* if */
					n+=5;
				} /* if */
			} /* for */
		} /* if */
		/* do this until there are no new handles */
	}while( nNoHandles > 0 );

	return 1;
} /* nInitializeAllPorts */

/*****************************************************************
Name:				nEnableAllPorts

Inputs:
	None.

Return Value:
	int - 1 if successful, 0 otherwise

Description:   This routine enables all the port handles that need
			   to be enabled using the PENA command.
*****************************************************************/
int CCommandHandling::nEnableAllPorts()
{
	int
		nNoHandles = 0,
		nPortHandle = 0,
		n = 0;
	char
		szHandleList[MAX_REPLY_MSG];

	m_nPortsEnabled = 0;
	/* get all the ports that need to be enabled */
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "PHSR 03" );

	if(nSendMessage( m_szCommand, TRUE ))
	{
		if (!nGetResponse( ))
			return 0;

		if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
			return 0;

		sprintf( szHandleList, m_szLastReply );
		nNoHandles = uASCIIToHex( &szHandleList[n], 2 );
		n+=2;

		for ( int i = 0; i < nNoHandles; i++ )
		{
			nPortHandle = uASCIIToHex( &szHandleList[n], 2 );
			memset(m_szCommand, 0, sizeof(m_szCommand));
			sprintf( m_szCommand, "PENA %02X%c", nPortHandle, 'D' );
			n+=5;
			if (!nSendMessage( m_szCommand, TRUE ))
				return 0;
			if ( !nGetResponse() )
				return 0;
			if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
				return 0;
			nGetPortInformation( nPortHandle );
			m_nPortsEnabled++;
		} /* for */
		return 1;
	} /* if */
	return 0;
} /* nEnableAllPorts */

/*****************************************************************
Name:				nInitializeHandle

Inputs:
	int nHandle - the handle to be intialized

Return Value:
	int - 1 if successful, otherwise 0

Description:   This routine initializes the specified handle using
			   the PINIT command.
*****************************************************************/
int CCommandHandling::nInitializeHandle( int nHandle )
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "PINIT %02X", nHandle );
	if ( !nSendMessage( m_szCommand, TRUE ))
		return 0;
	if ( !nGetResponse() )
		return 0;

	if (!nCheckResponse(nVerifyResponse(m_szLastReply, TRUE )))
	{
		return 0;
	} /* if */
	m_dtHandleInformation[nHandle].HandleInfo.bInitialized = TRUE;

	return 1;
} /* nInitializeHandle */

/*****************************************************************
Name:				nGetPortInformation

Inputs:
	int nPortHandle - the handle to get information for

Return Value:
	int - 1 if successful, 0 otherwise

Description:   
	This routine gets the port handling information for the supplied
	handle.  It uses the PHINF call to get this information.
*****************************************************************/
int CCommandHandling::nGetPortInformation(int nPortHandle)
{
	unsigned int
		uASCIIConv = 0; 
	char
		*pszPortInformation = NULL;

	memset(m_szCommand, 0, sizeof(m_szCommand));


	//if( m_dtSystemInformation.nTypeofSystem == VICRA_SYSTEM ||
	//	m_dtSystemInformation.nTypeofSystem == SPECTRA_SYSTEM)
	//	sprintf( m_szCommand, "PHINF %02X0005", nPortHandle );
	//else

	sprintf( m_szCommand, "PHINF %02X0025", nPortHandle );

	if ( nSendMessage( m_szCommand, TRUE ) )
	{
		if ( nGetResponse() )
		{
			if ( !nCheckResponse( nVerifyResponse(m_szLastReply, TRUE )))
				return 0;

			pszPortInformation = m_szLastReply;

			strncpy( m_dtHandleInformation[nPortHandle].szToolType, pszPortInformation, 8 );
			m_dtHandleInformation[nPortHandle].szToolType[8] = '\0';
			pszPortInformation+=8;
			strncpy( m_dtHandleInformation[nPortHandle].szManufact, pszPortInformation, 12 );
			m_dtHandleInformation[nPortHandle].szManufact[12] = '\0';
			pszPortInformation+=12;
			strncpy( m_dtHandleInformation[nPortHandle].szRev, pszPortInformation, 3 );
			m_dtHandleInformation[nPortHandle].szRev[3] = '\0';
			pszPortInformation+=3;
			strncpy( m_dtHandleInformation[nPortHandle].szSerialNo, pszPortInformation, 8 );
			m_dtHandleInformation[nPortHandle].szSerialNo[8] = '\0';
			pszPortInformation+=8;
			uASCIIConv = uASCIIToHex( pszPortInformation, 2 );
			pszPortInformation+=2;
			m_dtHandleInformation[nPortHandle].HandleInfo.bToolInPort = ( uASCIIConv & 0x01 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bGPIO1 = ( uASCIIConv & 0x02 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bGPIO2 = ( uASCIIConv & 0x04 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bGPIO3 = ( uASCIIConv & 0x08 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bInitialized = ( uASCIIConv & 0x10 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bEnabled = ( uASCIIConv & 0x20 ? 1 : 0 );
			m_dtHandleInformation[nPortHandle].HandleInfo.bTIPCurrentSensing = ( uASCIIConv & 0x80 ? 1 : 0 );

			/* parse the part number 0x0004 */
			strncpy( m_dtHandleInformation[nPortHandle].szPartNumber, pszPortInformation, 20 );
			m_dtHandleInformation[nPortHandle].szPartNumber[20] = '\0';
			pszPortInformation+=20;

			if( m_dtSystemInformation.nTypeofSystem != VICRA_SYSTEM &&
				m_dtSystemInformation.nTypeofSystem != SPECTRA_SYSTEM )
			{
				pszPortInformation+=10;
				sprintf( m_dtHandleInformation[nPortHandle].szPhysicalPort, "%d", nPortHandle );
				strncpy( m_dtHandleInformation[nPortHandle].szPhysicalPort, pszPortInformation, 2 );
				/* EC-03-0071
				m_dtHandleInformation[nPortHandle].szPhysicalPort[2] = '\0';
				 */
				pszPortInformation+=2;
				strncpy( m_dtHandleInformation[nPortHandle].szChannel, pszPortInformation, 2 );
				m_dtHandleInformation[nPortHandle].szChannel[2] = '\0';
				if ( !strncmp( m_dtHandleInformation[nPortHandle].szChannel, "01", 2 ) )
				{
					/* EC-03-0071
					strncat(m_dtHandleInformation[nPortHandle].szPhysicalPort, "-b", 2 );
					 */
					strncpy(&m_dtHandleInformation[nPortHandle].szPhysicalPort[2], "-b", 2 );
					for ( int i = 0; i < NO_HANDLES; i++ )
					{
						if ( strncmp( m_dtHandleInformation[i].szPhysicalPort, m_dtHandleInformation[nPortHandle].szPhysicalPort, 4 ) &&
							 !strncmp( m_dtHandleInformation[i].szPhysicalPort, m_dtHandleInformation[nPortHandle].szPhysicalPort, 2 ) )
							/* EC-03-0071
							strncat(m_dtHandleInformation[i].szPhysicalPort, "-a", 2 );
							 */
							strncpy(&m_dtHandleInformation[i].szPhysicalPort[2], "-a", 2 );
					} /* for */
				} /* if */
			} /* if */
		} /* if */
		else
			return 0;
	} /* if */

	return 1;
} /* nGetPortInformation */

/*****************************************************************
Name:				nGetBXTransforms

Inputs:
	bool bReturnOOV - whether or not to return values outside
					  of the characterized volume.

Return Value:
	int - 1 if successful, 0 otherwise.

Description:   
	This routine gets the transformation information using the BX
	command.  Remember that if you want to track outside the
	characterized volume you need to set the flag.
*****************************************************************/
int CCommandHandling::nGetBXTransforms(bool bReturn0x0800Option)
{
	int
		nReplyMode = 0x0001,
		nReplySize = 0,
		nSpot = 0,
		nNoHandles = 0,
		nHandle = 0,
		nRet  = 0;
	unsigned int
		unSystemStatus = 0,
		uTransStatus = 0,
		unHandleStatus = 0,
		uHeaderCRC = 0,
		uBodyCRC = 0,
		uCalcCRC = 0;
	char
		*pszTransformInfo = NULL;

	/* set reply mode depending on bReturnOOV */
	nReplyMode = bReturn0x0800Option ? 0x0801 : 0x0001;
	
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "BX %04X", nReplyMode );

	if(nSendMessage( m_szCommand, TRUE ))
	{
		if (!nGetBinaryResponse( ))
		{
			return 0;
		}/* if */

		if ( m_bDisplayErrorsWhileTracking )
		{
			if (!nCheckResponse(nVerifyResponse( m_szLastReply, TRUE )))
				return 0;
		}
		else
		{
			if (!nVerifyResponse( m_szLastReply, FALSE ))
				return 0;
		}/* else */

		pszTransformInfo = m_szLastReply;
		uCalcCRC = SystemGetCRC(m_szLastReply, 4 );

		/* check for preamble ( A5C4 ) */
        while(((pszTransformInfo[0]&0xff)!=0xc4))
		{
            pszTransformInfo++;
		}/* while */

		if ( (pszTransformInfo[0]&0xff)!=0xc4 || (pszTransformInfo[1]&0xff)!=0xa5 )
		{
			return REPLY_INVALID;
		}/* if */

		/* parse the header */
        nSpot+=2;
		nReplySize = nGetHex2(&pszTransformInfo[nSpot]);
        nSpot+=2;
		uHeaderCRC = nGetHex2(&pszTransformInfo[nSpot]); 
        nSpot+=2;

		if ( uCalcCRC != uHeaderCRC )
		{
			if ( m_bDisplayErrorsWhileTracking )
				nCheckResponse( REPLY_BADCRC ); /* display the Bad CRC error message */
			return REPLY_BADCRC;
		} /* if */

		nNoHandles = nGetHex1(&pszTransformInfo[nSpot]);
		nSpot++;

		ActivatedPortHandles.clear();

		for ( int i = 0; i < nNoHandles; i++ )
		{
			nHandle = nGetHex1(&pszTransformInfo[nSpot]);
			nSpot++;
			 
			// Store portHandles to simplify getting data out later
			ActivatedPortHandles.push_back(nHandle);

			uTransStatus = nGetHex1(&pszTransformInfo[nSpot]);
			nSpot++;

			if ( uTransStatus == 1 ) /* one means that the transformation was returned */
			{
				/* parse out the individual components by converting binary to floats */
				m_dtHandleInformation[nHandle].Xfrms.rotation.q0 = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.rotation.qx = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.rotation.qy = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.rotation.qz = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.translation.x = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.translation.y = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.translation.z = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.fError = fGetFloat(&pszTransformInfo[nSpot]);
				nSpot+=4;
				unHandleStatus = nGetHex4(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.ulFrameNumber = nGetHex4(&pszTransformInfo[nSpot]);
				nSpot+=4;
				m_dtHandleInformation[nHandle].Xfrms.ulFlags = TRANSFORM_VALID;
			} /* if */

			if ( uTransStatus == 2 || uTransStatus == 4 ) /* 2 means the tool is missing and */
														  /* 4 means DISABLED */
			{
				/*
				 * no transformation information is returned but the port status and time
				 * are return
				 */
				if ( uTransStatus == 2 )
				{
					unHandleStatus = nGetHex4(&pszTransformInfo[nSpot]);
					nSpot+=4;
					m_dtHandleInformation[nHandle].Xfrms.ulFrameNumber = nGetHex4(&pszTransformInfo[nSpot]);
					nSpot+=4;
					m_dtHandleInformation[nHandle].Xfrms.ulFlags = TRANSFORM_MISSING;
				} /* if */
				else
					m_dtHandleInformation[nHandle].Xfrms.ulFlags = TRANSFORM_DISABLED;

				m_dtHandleInformation[nHandle].Xfrms.rotation.q0 =
				m_dtHandleInformation[nHandle].Xfrms.rotation.qx =
				m_dtHandleInformation[nHandle].Xfrms.rotation.qy =
				m_dtHandleInformation[nHandle].Xfrms.rotation.qz =
				m_dtHandleInformation[nHandle].Xfrms.translation.x =
				m_dtHandleInformation[nHandle].Xfrms.translation.y =
				m_dtHandleInformation[nHandle].Xfrms.translation.z =
				m_dtHandleInformation[nHandle].Xfrms.fError = BAD_FLOAT;
			}/* if */

			if ( uTransStatus == 1 || uTransStatus == 2 )
			{
				m_dtHandleInformation[nHandle].HandleInfo.bToolInPort = ( unHandleStatus & 0x01 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bGPIO1 = ( unHandleStatus & 0x02 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bGPIO2 = ( unHandleStatus & 0x04 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bGPIO3 = ( unHandleStatus & 0x08 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bInitialized = ( unHandleStatus & 0x10 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bEnabled = ( unHandleStatus & 0x20 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bOutOfVolume = ( unHandleStatus & 0x40 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bPartiallyOutOfVolume = ( unHandleStatus & 0x80 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bBrokenSensor = ( unHandleStatus & 0x100 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bDisturbanceDet = ( unHandleStatus & 0x200 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bSignalTooSmall = ( unHandleStatus & 0x400 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bSignalTooBig = ( unHandleStatus & 0x800 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bProcessingException = ( unHandleStatus & 0x1000 ? 1 : 0 );
				m_dtHandleInformation[nHandle].HandleInfo.bHardwareFailure = ( unHandleStatus & 0x2000 ? 1 : 0 );
			}/* if */
		} /* for */

		unSystemStatus = nGetHex2( &pszTransformInfo[nSpot] );
		nSpot+=2;
		uBodyCRC = nGetHex2(&pszTransformInfo[nSpot]); 
		m_dtSystemInformation.bCommunicationSyncError = ( unSystemStatus & 0x01 ? 1 : 0 );
		m_dtSystemInformation.bTooMuchInterference = ( unSystemStatus & 0x02 ? 1 : 0 );
		m_dtSystemInformation.bSystemCRCError = ( unSystemStatus & 0x04 ? 1 : 0 );
		m_dtSystemInformation.bRecoverableException = ( unSystemStatus & 0x08 ? 1 : 0 );
		m_dtSystemInformation.bHardwareFailure = ( unSystemStatus & 0x10 ? 1 : 0 );
		m_dtSystemInformation.bHardwareChange = ( unSystemStatus & 0x20 ? 1 : 0 );
		m_dtSystemInformation.bPortOccupied = ( unSystemStatus & 0x40 ? 1 : 0 );
		m_dtSystemInformation.bPortUnoccupied = ( unSystemStatus & 0x80 ? 1 : 0 );

		uCalcCRC = SystemGetCRC(pszTransformInfo+=6, nSpot-6 );
		if ( uCalcCRC != uBodyCRC )
		{
			nCheckResponse( REPLY_BADCRC ); /* display the Bad CRC error message */
			return REPLY_BADCRC;
		} /* if */


	} /* if */

	/*
	 * reference tracking...apply at the end of all the parsing so that
	 * it is all the latest xfrms being applied
	 */
	//ApplyXfrms();


	return 1;
} /* nGetBXTransforms */

/*****************************************************************
Name:				nGetBinaryResponse

Inputs:
	None.

Return Value:
	int - 1 if passes, else 0

Description:   
	This routine gets the response from the system that is to be 
	poled through the com port.  The routine gets its start time 
	( the time it started polling ) and continues until one of two
	things happens.  First, if the end of response ( the number of bytes
	specified in the header is found ) the reply is complete and 1 is 
	returned.  If the time it takes to get a response exceeds the timeout 
	value, the system assumes no response is coming and timeouts.  The 
	timeout dialog 	is then displayed.  Use this response routine for 
	all calls except the BX call.
*****************************************************************/
int CCommandHandling::nGetBinaryResponse( )
{
	char
		chChar;
	bool 
		bDone = FALSE;
	int 
		nTotalBinaryLength = -1, //initialize it to a number smaller than nCount
		nCount = 0,
		nRet = 0,
		nRetry = 0,
		charInt = 0;

    memset(m_szLastReply, 0, sizeof( m_szLastReply ) );

	do
	{
		/* Check COM port */
		if( pCOMPort == NULL )
		{
			return FALSE;
		}/* if */

		charInt = pCOMPort->SerialGetChar();
		
		// Check for an error
		if( charInt ==  SERIAL_READ_ERROR ) break;

		chChar = char(charInt);
		m_szLastReply[nCount] = chChar;

		/*
			* Get the total length of the buffer
			*/
		if ( nCount == 3 )  
		{
			/* + 7 to account for header information */					
			nTotalBinaryLength = nGetHex2(&m_szLastReply[2]) + 7 + 1; 
		}/* if */

		nCount++;

		if ( nCount == nTotalBinaryLength )
		{
			bDone = TRUE;			
		}/* if */

	} while ( !bDone );

	return bDone;

} /* nGetBinaryResponse */

/*****************************************************************
Name:				nStartTracking

Inputs:
	None.

Return Value:
	int - 0 if fails, else nCheckResponse

Description:    This routine starts that System tracking.  It uses
				the TSTART command to do this.
*****************************************************************/
int CCommandHandling::nStartTracking()
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "TSTART " );

	if(nSendMessage( m_szCommand, TRUE ))
	{
		nGetResponse( );
		return nCheckResponse( nVerifyResponse(m_szLastReply, TRUE) );
	} /* if */

	return 0;
} /* nStartTracking */

/*****************************************************************
Name:				nStopTracking

Inputs:
	None.

Return Value:
	int - 0 if fails, nCheckResponse if passes

Description:   This routine stops the System's tracking by using
			   the TSTOP call.
*****************************************************************/
int CCommandHandling::nStopTracking()
{
	memset(m_szCommand, 0, sizeof(m_szCommand));
	sprintf( m_szCommand, "TSTOP " );

	if(nSendMessage( m_szCommand, TRUE ))
	{
		nGetResponse( );
		return nCheckResponse( nVerifyResponse(m_szLastReply, TRUE) );
	} /* if */

	return 0;
} /* nStopTracking */

int CCommandHandling::GetNumEnabledHandles()
{
	return m_nPortsEnabled;
}