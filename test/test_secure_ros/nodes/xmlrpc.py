#!/usr/bin/env python
#
# Software License Agreement (BSD License)
#
# Copyright (c) 2017, SRI International
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of Willow Garage, Inc. nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#


import argparse
import os
import sys
import argparse 
import xmlrpclib
import xml
from SimpleXMLRPCServer import SimpleXMLRPCServer
from SimpleXMLRPCServer import SimpleXMLRPCRequestHandler


class XmlRpcClient( ):
  def __init__(self):
    parser = argparse.ArgumentParser( description='ROS XmlRpc client',
        usage='''xmlrpc.py <command> [<args>]

The ROS XMLRPC commands are:

  getServiceClients       Get list of authorized subscriber clients
  getPublishedTopics      Get list of published topics

''')
    parser.add_argument('command', help='Subcommand to run')
    args = parser.parse_args(sys.argv[1:2])

    if not hasattr(self, args.command):
      print( 'Unrecognized command: %s' % args.command )
      parser.print_help()
      sys.exit(1)

    self.parser = argparse.ArgumentParser( description='Get list of authorized subscriber clients')
    self.parser.add_argument('--caller_id', "-c", type = str, default = "anon", help = 'Caller ID' )
    self.parser.add_argument('--master_uri', "-m", type = str, default = os.environ.get( "ROS_MASTER_URI" ), help = 'ROS Master URI' )

    # use dispatch pattern to invoke method with same name
    getattr(self, args.command)()
  

  def getServiceClients( self ):
    self.parser.add_argument('service', help = 'Service name' )
    args = self.parser.parse_args( sys.argv[2:] )
    client = xmlrpclib.ServerProxy( args.master_uri )
    val = client.getServiceClients( args.caller_id, args.service )
    print( 'getServiceClients returned code=%s, msg=%s, val=%s' % ( val[0], val[1], val[2] ) )


  def getPublishedTopics( self ):
    self.parser.add_argument('subgraph', help = 'Subgraph of topics', nargs = "?", default = "" )
    args = self.parser.parse_args( sys.argv[2:] )
    client = xmlrpclib.ServerProxy( args.master_uri )
    val = client.getPublishedTopics( args.caller_id, args.subgraph )
    print( 'getPublishedTopics returned code=%s, msg=%s, val=%s' % ( val[0], val[1], val[2] ) )


if __name__ == '__main__':
  XmlRpcClient()
