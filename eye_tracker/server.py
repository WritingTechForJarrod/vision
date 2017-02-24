#
#   Hello World server in Python
#   Binds REP socket to tcp://*:5555
#   Expects b"Hello" from client, replies with b"World"
#

import time
import zmq

# Constants
SOCKET_PUB = 'tcp://*:5556'
SOCKET_PULL = 'tcp://*:5557'

def poll():
	events = poller.poll(1)
	if len(events) > 0:
		msg = pull.recv()
		print(msg)

context = zmq.Context()
pub = context.socket(zmq.PUB)
pull = context.socket(zmq.PULL)
pub.bind(SOCKET_PUB)
pull.bind(SOCKET_PULL)

poller = zmq.Poller()
poller.register(pull,zmq.POLLIN)

i = 0
message = "@eyetracker test" + str(i)
while True:
	poll()
	pub.send_string(message)
	if (i > 1000):
		message = "@eyetracker cmd=quit " + str(i)
	else:
		message = "@eyetracker test" + str(i)
	i = i + 1
