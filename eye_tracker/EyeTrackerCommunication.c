/*
 * This code is a modified version of an the MinimalGazeDataStream example found in the Tobii EyeX SDK. Original header below.
 *
 * This is an example that demonstrates how to connect to the EyeX Engine and subscribe to the lightly filtered gaze data stream.
 * Copyright 2013-2014 Tobii Technology AB. All rights reserved.
 */
#define _WINSOCKAPI_
#define _CRT_SECURE_NO_WARNINGS

#define SOCKET_SUB  "tcp://localhost:5556" // Eye tracker module listens to this socket for commands
#define SOCKET_PUSH "tcp://localhost:5557" // Eye tracker module pushes updates on this socket
#define TOPIC_FILTER "eyetracker"
#define TIMEOUT_THRESHOLD 10 // Timeout before marco command does not reply with polo.

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
TX_CONTEXTHANDLE g_hContext = TX_EMPTY_HANDLE;
static void *g_zmqcontext;
static void *g_subscription_socket;
static void *g_push_socket;
static int g_start_message_sent = 0;
static int g_marco_blocking = 0;
static int g_connection_successful = 0;
static time_t g_last_data_sent_time;
static double g_screen_bound_x = 0;
static double g_screen_bound_y = 0;
static int g_screen_bounds_found = 0;

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
	zmq_send(g_push_socket, buffer, strlen(buffer), 0);
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
 * Handles a state-changed notification, or the response from a get-state operation.
 */
void OnStateReceived(TX_HANDLE hStateBag)
{
	TX_BOOL success;
	TX_INTEGER eyeTrackingState;
	TX_SIZE2 screenBounds;

	success = (txGetStateValueAsInteger(hStateBag, TX_STATEPATH_EYETRACKINGSTATE, &eyeTrackingState) == TX_RESULT_OK);
	success = (txGetStateValueAsSize2(hStateBag, TX_STATEPATH_EYETRACKINGSCREENBOUNDS, &screenBounds) == TX_RESULT_OK);
	if (success) {
		g_screen_bound_x = screenBounds.Width;
		g_screen_bound_y = screenBounds.Height;
		g_screen_bounds_found = 1;
	}
}

/*
 * Handles engine state change notifications.
 */
void TX_CALLCONVENTION OnEngineStateChanged(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_RESULT result = TX_RESULT_UNKNOWN;
	TX_HANDLE hStateBag = TX_EMPTY_HANDLE;

	if (txGetAsyncDataResultCode(hAsyncData, &result) == TX_RESULT_OK && 
		txGetAsyncDataContent(hAsyncData, &hStateBag) == TX_RESULT_OK) {
		OnStateReceived(hStateBag);
		txReleaseObject(&hStateBag);
	}
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
		g_connection_successful = 0;
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		g_connection_successful = 0;
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		g_connection_successful = 0;
		send_zmq_message(ET_SERVER_LOW);
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		g_connection_successful = 0;
		send_zmq_message(ET_SERVER_HIGH);
		break;
	}
	txGetStateAsync(g_hContext, TX_STATEPATH_EYETRACKING, OnEngineStateChanged, NULL);
}

/*
 * Handles an event from the Gaze Point data stream.
 */
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	char send_message[50]; // Max message size is 50 bytes
	float x_coord;
	float y_coord;
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		g_connection_successful = 1;
		if (g_start_message_sent == 1 && g_marco_blocking == 0 && g_screen_bounds_found == 1) {
			g_last_data_sent_time = time(NULL);
			x_coord = eventParams.X/g_screen_bound_x;
			y_coord = eventParams.Y/g_screen_bound_y;
			if (x_coord > 0 && x_coord < 1 && y_coord > 0 && y_coord < 1) {
				_snprintf(send_message, 50, "%s gaze %.3f,%.3f",TOPIC_FILTER, x_coord, y_coord);
				zmq_send(g_push_socket, send_message, strlen(send_message), 0);
			}
		}
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
BOOL _on_start(char* subscription_filter, TX_TICKET* hConnectionStateChangedTicket,
			   TX_TICKET* hEventHandlerTicket) {
	BOOL success = 1;
	// Create and initialize 0mq sockets
	g_zmqcontext = zmq_ctx_new();
	g_subscription_socket = zmq_socket(g_zmqcontext, ZMQ_SUB);
	g_push_socket = zmq_socket(g_zmqcontext,ZMQ_PUSH);
	zmq_connect(g_subscription_socket, SOCKET_SUB);
	zmq_connect(g_push_socket, SOCKET_PUSH);
	zmq_setsockopt(g_subscription_socket, ZMQ_SUBSCRIBE, subscription_filter, strlen(subscription_filter));

	send_zmq_message(ET_STARTING);
	g_start_message_sent = 1;

	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&g_hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(g_hContext);
	success &= txRegisterConnectionStateChangedHandler(g_hContext, hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(g_hContext, hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;

	// enables connection to EyeX Engine, connection alive
	// until it is desabled or context is destroyed
	success &= txEnableConnection(g_hContext) == TX_RESULT_OK;

	return success;
}

/*
* After receiving "marco" command, responds with "eyetracker polo".
*/
void _on_marco() {
	time_t current_time = time(NULL);
	g_marco_blocking = 1;
	if (g_connection_successful == 1 && (current_time - g_last_data_sent_time) < TIMEOUT_THRESHOLD) {
		send_zmq_message(ET_MARCO);
	}
	else
		g_connection_successful = 0;
	g_marco_blocking = 0;
	return;
}

/*
* After receiving stop command from server, close zmq sockets, destroy zmq context and disconnect from Tobii engine.
*/
void _on_stop(TX_CONTEXTHANDLE hContext) {
	boolean success;
	send_zmq_message(ET_STOPPING);
	// zmq clean up
	zmq_close(g_subscription_socket);
	zmq_close(g_push_socket);
	zmq_ctx_destroy(g_zmqcontext);

	// Tobii cleanup
	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	success = txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	success &= txReleaseContext(&hContext) == TX_RESULT_OK;
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
	zmq_recv(g_subscription_socket,buffer,100,ZMQ_DONTWAIT);
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
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;
	// zmq variables
	char* subscription_filter = "@eyetracker";
	int is_alive = 1;
	Command received = CONSOLE_NO_CMD;

	success = _on_start(subscription_filter,&hConnectionStateChangedTicket,&hEventHandlerTicket);

	if (!success) {
		send_zmq_message(ET_INIT_FAILED);
		_on_stop(g_hContext);
	}

	while (is_alive == 1) {
		received = read_message();
		if (received == CONSOLE_STOP) {
			is_alive = 0;
			_on_stop(g_hContext);
		}
		else if (received == CONSOLE_MARCO) {
			_on_marco();
		}
	}

	return 0;
}
