#!/router/bin/python

import external_packages
import zmq
import json
import general_utils
import re
from time import sleep
from collections import namedtuple

CmdResponse = namedtuple('CmdResponse', ['success', 'data'])

class bcolors:
    BLUE = '\033[94m'
    GREEN = '\033[32m'
    YELLOW = '\033[93m'
    RED = '\033[31m'
    MAGENTA = '\033[35m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

# sub class to describe a batch
class BatchMessage(object):
    def __init__ (self, rpc_client):
        self.rpc_client = rpc_client
        self.batch_list = []

    def add (self, method_name, params={}):

        id, msg = self.rpc_client.create_jsonrpc_v2(method_name, params, encode = False)
        self.batch_list.append(msg)

    def invoke(self, block = False):
        if not self.rpc_client.connected:
            return False, "Not connected to server"

        msg = json.dumps(self.batch_list)

        rc, resp_list = self.rpc_client.send_raw_msg(msg, block = False)
        if len(self.batch_list) == 1:
            return CmdResponse(True, [CmdResponse(rc, resp_list)])
        else:
            return CmdResponse(rc, resp_list)


# JSON RPC v2.0 client
class JsonRpcClient(object):

    def __init__ (self, default_server, default_port):
        self.verbose = False
        self.connected = False

        # default values
        self.port   = default_port
        self.server = default_server
        self.id_gen = general_utils.random_id_gen()

    def get_connection_details (self):
        rc = {}
        rc['server'] = self.server
        rc['port']   = self.port

        return rc

    # pretty print for JSON
    def pretty_json (self, json_str, use_colors = True):
        pretty_str = json.dumps(json.loads(json_str), indent = 4, separators=(',', ': '), sort_keys = True)

        if not use_colors:
            return pretty_str

        try:
            # int numbers
            pretty_str = re.sub(r'([ ]*:[ ]+)(\-?[1-9][0-9]*[^.])',r'\1{0}\2{1}'.format(bcolors.BLUE, bcolors.ENDC), pretty_str)
            # float
            pretty_str = re.sub(r'([ ]*:[ ]+)(\-?[1-9][0-9]*\.[0-9]+)',r'\1{0}\2{1}'.format(bcolors.MAGENTA, bcolors.ENDC), pretty_str)
            # strings

            pretty_str = re.sub(r'([ ]*:[ ]+)("[^"]*")',r'\1{0}\2{1}'.format(bcolors.RED, bcolors.ENDC), pretty_str)
            pretty_str = re.sub(r"('[^']*')", r'{0}\1{1}'.format(bcolors.MAGENTA, bcolors.RED), pretty_str)
        except :
            pass

        return pretty_str

    def verbose_msg (self, msg):
        if not self.verbose:
            return

        print "[verbose] " + msg


    # batch messages
    def create_batch (self):
        return BatchMessage(self)

    def create_jsonrpc_v2 (self, method_name, params = {}, encode = True):
        msg = {}
        msg["jsonrpc"] = "2.0"
        msg["method"]  = method_name

        msg["params"] = params

        msg["id"] = self.id_gen.next()

        if encode:
            return id, json.dumps(msg)
        else:
            return id, msg


    def invoke_rpc_method (self, method_name, params = {}):
        if not self.connected:
            return False, "Not connected to server"

        id, msg = self.create_jsonrpc_v2(method_name, params)

        return self.send_raw_msg(msg)


    # low level send of string message
    def send_raw_msg (self, msg):

        self.verbose_msg("Sending Request To Server:\n\n" + self.pretty_json(msg) + "\n")

        tries = 0
        while True:
            try:
                self.socket.send(msg)
                break
            except zmq.Again:
                sleep(0.1)
                tries += 1
                if tries > 10:
                    self.disconnect()
                    return CmdResponse(False, "Failed to send message to server")


        tries = 0
        while True:
            try:
                response = self.socket.recv()
                break
            except zmq.Again:
                sleep(0.1)
                tries += 1
                if tries > 10:
                    self.disconnect()
                    return CmdResponse(False, "Failed to get server response")


        self.verbose_msg("Server Response:\n\n" + self.pretty_json(response) + "\n")

        # decode

        # batch ?
        response_json = json.loads(response)

        if isinstance(response_json, list):
            rc_list = []

            for single_response in response_json:
                rc, msg = self.process_single_response(single_response)
                rc_list.append( CmdResponse(rc, msg) )

            return CmdResponse(True, rc_list)

        else:
            rc, msg = self.process_single_response(response_json)
            return CmdResponse(rc, msg)


    def process_single_response (self, response_json):

        if (response_json.get("jsonrpc") != "2.0"):
            return False, "Malfromed Response ({0})".format(str(response))

        # error reported by server
        if ("error" in response_json):
            if "specific_err" in response_json["error"]:
                return False, response_json["error"]["specific_err"]
            else:
                return False, response_json["error"]["message"]

        # if no error there should be a result
        if ("result" not in response_json):
            return False, "Malformed Response ({0})".format(str(response))

        return True, response_json["result"]


  
    def set_verbose(self, mode):
        self.verbose = mode

    def disconnect (self):
        if self.connected:
            self.socket.close(linger = 0)
            self.context.destroy(linger = 0)
            self.connected = False
            return True, ""
        else:
            return False, "Not connected to server"

    def connect(self, server=None, port=None):
        if self.connected:
            self.disconnect()

        self.context = zmq.Context()

        self.server = (server if server else self.server)
        self.port = (port if port else self.port)

        #  Socket to talk to server
        self.transport = "tcp://{0}:{1}".format(self.server, self.port)

        print "\nConnecting To RPC Server On {0}".format(self.transport)

        self.socket = self.context.socket(zmq.REQ)
        try:
            self.socket.connect(self.transport)
        except zmq.error.ZMQError as e:
            return False, "ZMQ Error: Bad server or port name: " + str(e)

        self.socket.setsockopt(zmq.SNDTIMEO, 5)
        self.socket.setsockopt(zmq.RCVTIMEO, 5)

        self.connected = True

        return True, ""


    def reconnect(self):
        # connect using current values
        return self.connect()

        if not self.connected:
            return False, "Not connected to server"

        # reconnect
        return self.connect(self.server, self.port)


    def is_connected(self):
        return self.connected

    def __del__(self):
        print "Shutting down RPC client\n"
        if hasattr(self, "context"):
            self.context.destroy(linger=0)

