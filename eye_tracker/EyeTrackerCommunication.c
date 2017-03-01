/*
 * This is an example that demonstrates how to connect to the EyeX Engine and subscribe to the lightly filtered gaze data stream.
 *
 * Copyright 2013-2014 Tobii Technology AB. All rights reserved.
 */
#define _WINSOCKAPI_
#define _CRT_SECURE_NO_WARNINGS

#define SOCKET_SUB  "tcp://localhost:5556" // Eye tracker module listens to this socket for commands
#define SOCKET_PUSH "tcp://localhost:5557" // Eye tracker module pushes updates on this socket
#define TOPIC_FILTER "eyetracker"

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <assert.h>
#include <time.h>
#include <zmq.h>
#include "eyex/EyeX.h"

#pragma comment (lib, "Tobii.EyeX.Client.lib")
// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Twilight Sparkle";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;
static void *context;
static void *subscription_socket;
static void *push_socket;
static int start_message_sent = 0;
static int marco_blocking = 0;

/*
* Array of valid messages eyetracker can send. Maintains one to one correspondence with "message_type" enumeration.
*/
char* eyetracker_messages[] = { "eyetracker started",
								"eyetracker stopping",
								"eyetracker polo",
								"eyetracker err Tobii engine initialization failed",
								"eyetracker err Failed to initialize the data stream.",
								"eyetracker err The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.",
								"eyetracker err The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.s",
								"eyetracker err Failed to interpret gaze data event packet.",
								"EyeX could not be shut down cleanly."
								};

/*
* Message types used to index "eyetracker_messages" array.
*/
typedef enum message_type
{
	ET_STARTING,
	ET_STOPPING,
	ET_MARCO,
	ET_INIT_FAILED,
	ET_DATA_STREAM_FAILED,
	ET_SERVER_LOW,
	ET_SERVER_HIGH,
	ET_DATA_PACKET,
	ET_STOP_FAIL
} ET_Message;

/*
* Commands issued from console
*/
typedef enum command
{
	CONSOLE_MARCO,
	CONSOLE_NO_CMD,
	CONSOLE_STOP
} Command;

/*
* Function to send ZMQ message from select Eyetracker messages
*/
void send_zmq_message(ET_Message send_message) {
	char buffer[100];
	_snprintf(buffer, 50, "%s",eyetracker_messages[send_message]);
	zmq_send(push_socket, buffer, strlen(buffer), 0);
}

/*
 * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
 */
BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
			BOOL success;
			// commit the snapshot with the global interactor as soon as the connection to the engine is established.
			// (it cannot be done earlier because committing means "send to the engine".)
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
			if (!success) {
				send_zmq_message(ET_DATA_STREAM_FAILED);
			}
		}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		//printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		//printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		send_zmq_message(ET_SERVER_LOW);
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		send_zmq_message(ET_SERVER_HIGH);
		break;
	}
}

/*
 * Handles an event from the Gaze Point data stream.
 */
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	char send_message[30]; // Max message size is 30 bytes
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		_snprintf(send_message, 50, "%s gaze %5.1d,%5.1d",TOPIC_FILTER, (int)eventParams.X, (int)eventParams.Y);
		if (start_message_sent == 1 && marco_blocking == 0)
			zmq_send(push_socket, send_message, strlen(send_message), 0);
	} else {
		send_zmq_message(ET_DATA_PACKET);
	}
}

/*
 * Callback function invoked when an event has been received from the EyeX Engine.
 */
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
 		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}

/*
* Eyetracker start-up function. Sets up 0mq communication, sends eyetracker starting message and connects to Tobii Engine.
*/
BOOL _on_start(char* subscription_filter, TX_CONTEXTHANDLE* hContext, TX_TICKET* hConnectionStateChangedTicket,
			   TX_TICKET* hEventHandlerTicket) {
	BOOL success;
	// Create and initialize 0mq sockets
	context = zmq_ctx_new();
	subscription_socket = zmq_socket(context, ZMQ_SUB);
	push_socket = zmq_socket(context,ZMQ_PUSH);
	zmq_connect(subscription_socket, SOCKET_SUB);
	zmq_connect(push_socket, SOCKET_PUSH);
	zmq_setsockopt(subscription_socket, ZMQ_SUBSCRIBE, subscription_filter, strlen(subscription_filter));

	send_zmq_message(ET_STARTING);
	start_message_sent = 1;

	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(*hContext);
	success &= txRegisterConnectionStateChangedHandler(*hContext, hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(*hContext, hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;

	// enables connection to EyeX Engine, connection alive
	// until it is desabled or context is destroyed
	success &= txEnableConnection(*hContext) == TX_RESULT_OK;

	return success;
}

/*
* After receiving "marco" command, responds with "eyetracker polo".
*/
void _on_marco() {
	marco_blocking = 1;
	send_zmq_message(ET_MARCO);
	marco_blocking = 0;
	return;
}

/*
* After receiving stop command from server, close zmq sockets, destroy zmq context and disconnect from Tobii engine.
*/
void _on_stop(TX_CONTEXTHANDLE* hContext) {
	boolean success;
	send_zmq_message(ET_STOPPING);
	// zmq clean up
	zmq_close(subscription_socket);
	zmq_close(push_socket);
	zmq_ctx_destroy(context);

	// Tobii cleanup
	// disable and delete the context.
	txDisableConnection(*hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	success = txShutdownContext(*hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	success &= txReleaseContext(hContext) == TX_RESULT_OK;
	success &= txUninitializeEyeX() == TX_RESULT_OK;

	if (!success) {
		send_zmq_message(ET_STOP_FAIL);
	}
}

/*
* Processes incoming message from console and returns the command that was received.
*/
Command read_message() {
	Command received = CONSOLE_NO_CMD;
	char buffer[100];
	zmq_recv(subscription_socket,buffer,100,ZMQ_DONTWAIT);
	if (strstr(buffer, "marco") != NULL) {
		received = CONSOLE_MARCO;
	}
	else if(strstr(buffer, "stop") != NULL) {
		received = CONSOLE_STOP;
	}
	return received;
}

/*
 * Application entry point.
 */
int main(int argc, char* argv[])
{
	// Eye tracker variables
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;
	// zmq variables
	char* subscription_filter = "@eyetracker";
	int is_alive = 1;
	Command received = CONSOLE_NO_CMD;

	success = _on_start(subscription_filter, &hContext, &hConnectionStateChangedTicket, &hEventHandlerTicket);

	if (!success) {
		send_zmq_message(ET_INIT_FAILED);
		_on_stop(&hContext);
	}

	while (is_alive == 1) {
		received = read_message();
		if (received == CONSOLE_STOP) {
			is_alive = 0;
			_on_stop(&hContext);
		}
		else if (received == CONSOLE_MARCO) {
			_on_marco();
		}
	}

	return 0;
}
