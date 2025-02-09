#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Dan Klein, Itay Marom
Cisco Systems, Inc.

Copyright (c) 2015-2015 Cisco Systems, Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""


import cmd
import json
import ast
import argparse
import random
import readline
import string
import os
import sys
import tty, termios
import trex_root_path
from common.trex_streams import *
from client.trex_stateless_client import CTRexStatelessClient
from common.text_opts import *
from client_utils.general_utils import user_input, get_current_user
import trex_status
import parsing_opts


__version__ = "1.1"


class TRexGeneralCmd(cmd.Cmd):
    def __init__(self):
        cmd.Cmd.__init__(self)
        # configure history behaviour
        self._history_file_dir = "/tmp/trex/console/"
        self._history_file = self.get_history_file_full_path()
        readline.set_history_length(100)
        # load history, if any
        self.load_console_history()


    def get_console_identifier(self):
        return self.__class__.__name__

    def get_history_file_full_path(self):
        return "{dir}{filename}.hist".format(dir=self._history_file_dir,
                                             filename=self.get_console_identifier())

    def load_console_history(self):
        if os.path.exists(self._history_file):
            readline.read_history_file(self._history_file)
        return

    def save_console_history(self):
        if not os.path.exists(self._history_file_dir):
            os.makedirs(self._history_file_dir)
        # os.mknod(self._history_file)
        readline.write_history_file(self._history_file)
        return

    def emptyline(self):
        """Called when an empty line is entered in response to the prompt.

        This overriding is such that when empty line is passed, **nothing happens**.
        """
        return

    def completenames(self, text, *ignored):
        """
        This overriding is such that a space is added to name completion.
        """
        dotext = 'do_'+text
        return [a[3:]+' ' for a in self.get_names() if a.startswith(dotext)]

    def precmd(self, line):
        # before doing anything, save history snapshot of the console
        # this is done before executing the command in case of ungraceful application exit
        self.save_console_history()
        return line


#
# main console object
class TRexConsole(TRexGeneralCmd):
    """Trex Console"""

    def __init__(self, stateless_client, acquire_all_ports=True, verbose=False):
        self.stateless_client = stateless_client
        TRexGeneralCmd.__init__(self)


        self.verbose = verbose
        self.acquire_all_ports = acquire_all_ports

        self.intro  = "\n-=TRex Console v{ver}=-\n".format(ver=__version__)
        self.intro += "\nType 'help' or '?' for supported actions\n"

        self.postcmd(False, "")


    ################### internal section ########################

    def get_console_identifier(self):
        return "{context}_{server}".format(context=self.__class__.__name__,
                                           server=self.stateless_client.get_system_info()['hostname'])
    
    def register_main_console_methods(self):
        main_names = set(self.trex_console.get_names()).difference(set(dir(self.__class__)))
        for name in main_names:
            for prefix in 'do_', 'help_', 'complete_':
                if name.startswith(prefix):
                    self.__dict__[name] = getattr(self.trex_console, name)

    def postcmd(self, stop, line):
        if self.stateless_client.is_connected():
            self.prompt = "TRex > "
        else:
            self.supported_rpc = None
            self.prompt = "TRex (offline) > "

        return stop

    def default(self, line):
        print "'{0}' is an unrecognized command. type 'help' or '?' for a list\n".format(line)

    @staticmethod
    def tree_autocomplete(text):
        dir = os.path.dirname(text)
        if dir:
            path = dir
        else:
            path = "."


        start_string = os.path.basename(text)
        
        targets = []

        for x in os.listdir(path):
            if x.startswith(start_string):
                y = os.path.join(path, x)
                if os.path.isfile(y):
                    targets.append(x + ' ')
                elif os.path.isdir(y):
                    targets.append(x + '/')

        return targets

    # annotation method
    @staticmethod
    def annotate (desc, rc = None, err_log = None, ext_err_msg = None):
        print format_text('\n{:<40}'.format(desc), 'bold'),
        if rc == None:
            print "\n"
            return

        if rc == False:
            # do we have a complex log object ?
            if isinstance(err_log, list):
                print ""
                for func in err_log:
                    if func:
                        print func
                print ""

            elif isinstance(err_log, str):
                print "\n" + err_log + "\n"

            print format_text("[FAILED]\n", 'red', 'bold')
            if ext_err_msg:
                print format_text(ext_err_msg + "\n", 'blue', 'bold')

            return False

        else:
            print format_text("[SUCCESS]\n", 'green', 'bold')
            return True


    ####################### shell commands #######################
    def do_ping (self, line):
        '''Ping the server\n'''

        rc = self.stateless_client.cmd_ping()
        if rc.bad():
            return

    def do_test (self, line):
        print self.stateless_client.get_acquired_ports()

    # set verbose on / off
    def do_verbose(self, line):
        '''Shows or set verbose mode\n'''
        if line == "":
            print "\nverbose is " + ("on\n" if self.verbose else "off\n")

        elif line == "on":
            self.verbose = True
            self.stateless_client.set_verbose(True)
            print format_text("\nverbose set to on\n", 'green', 'bold')

        elif line == "off":
            self.verbose = False
            self.stateless_client.set_verbose(False)
            print format_text("\nverbose set to off\n", 'green', 'bold')

        else:
            print format_text("\nplease specify 'on' or 'off'\n", 'bold')


    ############### connect
    def do_connect (self, line):
        '''Connects to the server\n'''

        rc = self.stateless_client.cmd_connect()
        if rc.bad():
            return


    def do_disconnect (self, line):
        '''Disconnect from the server\n'''

        rc = self.stateless_client.cmd_disconnect()
        if rc.bad():
            return

 
    ############### start

    def complete_start(self, text, line, begidx, endidx):
        s = line.split()
        l = len(s)

        file_flags = parsing_opts.get_flags(parsing_opts.FILE_PATH)

        if (l > 1) and (s[l - 1] in file_flags):
            return TRexConsole.tree_autocomplete("")

        if (l > 2) and (s[l - 2] in file_flags):
            return TRexConsole.tree_autocomplete(s[l - 1])

    def do_start(self, line):
        '''Start selected traffic in specified ports on TRex\n'''

        self.stateless_client.cmd_start_line(line)


    def help_start(self):
        self.do_start("-h")

    ############# stop
    def do_stop(self, line):
        self.stateless_client.cmd_stop_line(line)

        

    def help_stop(self):
        self.do_stop("-h")

    ########## reset
    def do_reset (self, line):
        '''force stop all ports\n'''
        self.stateless_client.cmd_reset()

  
    # tui
    def do_tui (self, line):
        '''Shows a graphical console\n'''

        if not self.stateless_client.is_connected():
            print format_text("\nNot connected to server\n", 'bold')
            return

        self.do_verbose('off')
        trex_status.show_trex_status(self.stateless_client)

    # quit function
    def do_quit(self, line):
        '''Exit the client\n'''
        return True

    
    def do_help (self, line):
         '''Shows This Help Screen\n'''
         if line:
             try:
                 func = getattr(self, 'help_' + line)
             except AttributeError:
                 try:
                     doc = getattr(self, 'do_' + line).__doc__
                     if doc:
                         self.stdout.write("%s\n"%str(doc))
                         return
                 except AttributeError:
                     pass
                 self.stdout.write("%s\n"%str(self.nohelp % (line,)))
                 return
             func()
             return
    
         print "\nSupported Console Commands:"
         print "----------------------------\n"
    
         cmds =  [x[3:] for x in self.get_names() if x.startswith("do_")]
         for cmd in cmds:
             if ( (cmd == "EOF") or (cmd == "q") or (cmd == "exit")):
                 continue
    
             try:
                 doc = getattr(self, 'do_' + cmd).__doc__
                 if doc:
                     help = str(doc)
                 else:
                     help = "*** Undocumented Function ***\n"
             except AttributeError:
                 help = "*** Undocumented Function ***\n"
    
             print "{:<30} {:<30}".format(cmd + " - ", help)

    do_exit = do_EOF = do_q = do_quit


#
def is_valid_file(filename):
    if not os.path.isfile(filename):
        raise argparse.ArgumentTypeError("The file '%s' does not exist" % filename)

    return filename


def setParserOptions():
    parser = argparse.ArgumentParser(prog="trex_console.py")

    parser.add_argument("-s", "--server", help = "TRex Server [default is localhost]",
                        default = "localhost",
                        type = str)

    parser.add_argument("-p", "--port", help = "TRex Server Port  [default is 4501]\n",
                        default = 4501,
                        type = int)

    parser.add_argument("--async_port", help = "TRex ASync Publisher Port [default is 4500]\n",
                        default = 4500,
                        dest='pub',
                        type = int)

    parser.add_argument("-u", "--user", help = "User Name  [default is currently logged in user]\n",
                        default = get_current_user(),
                        type = str)

    parser.add_argument("--verbose", dest="verbose",
                        action="store_true", help="Switch ON verbose option. Default is: OFF.",
                        default = False)


    parser.add_argument("--no_acquire", dest="acquire",
                        action="store_false", help="Acquire all ports on connect. Default is: ON.",
                        default = True)

    parser.add_argument("--batch", dest="batch",
                        nargs = 1,
                        type = is_valid_file,
                        help = "Run the console in a batch mode with file",
                        default = None)

    return parser


def main():
    parser = setParserOptions()
    options = parser.parse_args()

    # Stateless client connection
    stateless_client = CTRexStatelessClient(options.user, options.server, options.port, options.pub)
    rc = stateless_client.cmd_connect()
    if rc.bad():
        return

    if options.batch:
        cont = stateless_client.run_script_file(options.batch[0])
        if not cont:
            return
        
    # console
    try:
        console = TRexConsole(stateless_client, options.acquire, options.verbose)
        console.cmdloop()
    except KeyboardInterrupt as e:
        print "\n\n*** Caught Ctrl + C... Exiting...\n\n"
        return

if __name__ == '__main__':
    main()

