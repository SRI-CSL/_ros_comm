/*
 * Copyright (C) 2009, Willow Garage, Inc.
 * Copyright (c) 2017, SRI International
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ros/master.h"
#include "ros/xmlrpc_manager.h"
#include "ros/this_node.h"
#include "ros/init.h"
#include "ros/network.h"

#include <ros/console.h>
#include <ros/assert.h>

#include "xmlrpcpp/XmlRpc.h"
#include <iostream>
#include <sstream> 

namespace ros
{

namespace master
{

uint32_t g_port = 0;
std::string g_host;
std::string g_uri;
ros::WallDuration g_retry_timeout;

void init(const M_string& remappings)
{
  M_string::const_iterator it = remappings.find("__master");
  if (it != remappings.end())
  {
    g_uri = it->second;
  }

  if (g_uri.empty())
  {
    char *master_uri_env = NULL;
    #ifdef _MSC_VER
      _dupenv_s(&master_uri_env, NULL, "ROS_MASTER_URI");
    #else
      master_uri_env = getenv("ROS_MASTER_URI");
    #endif
    if (!master_uri_env)
    {
      ROS_FATAL( "ROS_MASTER_URI is not defined in the environment. Either " \
                 "type the following or (preferrably) add this to your " \
                 "~/.bashrc file in order set up your " \
                 "local machine as a ROS master:\n\n" \
                 "export ROS_MASTER_URI=http://localhost:11311\n\n" \
                 "then, type 'roscore' in another shell to actually launch " \
                 "the master program.");
      ROS_BREAK();
    }

    g_uri = master_uri_env;

#ifdef _MSC_VER
    // http://msdn.microsoft.com/en-us/library/ms175774(v=vs.80).aspx
    free(master_uri_env);
#endif
  }

  // Split URI into
  if (!network::splitURI(g_uri, g_host, g_port))
  {
    ROS_FATAL( "Couldn't parse the master URI [%s] into a host:port pair.", g_uri.c_str());
    ROS_BREAK();
  }
}

const std::string& getHost()
{
  return g_host;
}

uint32_t getPort()
{
  return g_port;
}

const std::string& getURI()
{
  return g_uri;
}

void setRetryTimeout(ros::WallDuration timeout)
{
  if (timeout < ros::WallDuration(0))
  {
    ROS_FATAL("retry timeout must not be negative.");
    ROS_BREAK();
  }
  g_retry_timeout = timeout;
}

bool check()
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = this_node::getName();
  return execute("getPid", args, result, payload, false);
}

bool getTopics(V_TopicInfo& topics)
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = this_node::getName();
  args[1] = ""; //TODO: Fix this

  if (!execute("getPublishedTopics", args, result, payload, true))
  {
    return false;
  }

  topics.clear();
  for (int i = 0; i < payload.size(); i++)
  {
    topics.push_back(TopicInfo(std::string(payload[i][0]), std::string(payload[i][1])));
  }

  return true;
}

bool getNodes(V_string& nodes)
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = this_node::getName();

  if (!execute("getSystemState", args, result, payload, true))
  {
    return false;
  }

  S_string node_set;
  for (int i = 0; i < payload.size(); ++i)
  {
    for (int j = 0; j < payload[i].size(); ++j)
    {
      XmlRpc::XmlRpcValue val = payload[i][j][1];
      for (int k = 0; k < val.size(); ++k)
      {
        std::string name = payload[i][j][1][k];
        node_set.insert(name);
      }
    }
  }

  nodes.insert(nodes.end(), node_set.begin(), node_set.end());

  return true;
}

std::string XmlRpcValueType( XmlRpc::XmlRpcValue& value ) {
  std::stringstream ss;
  if ( value.getType() == XmlRpc::XmlRpcValue::TypeBoolean ) {
    bool v = value;
    ss << "Boolean (" << v << ")";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeInt ) {
    int v = value;
    ss << "Int (" << v << ")";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeDouble ) {
    double v = value;
    ss << "Double (" << v << ")";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeString ) {
    std::string v = value;
    ss << "String (" << v << ")";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeDateTime ) {
    ss << "DateTime";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeBase64 ) {
    ss << "Base64";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeArray ) {
    ss << "Array";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeStruct ) {
    ss << "Struct";
  }
  else if ( value.getType() == XmlRpc::XmlRpcValue::TypeInvalid ) {
    ss << "Invalid";
  }
  else {
    ss << "Other";
  }
  return ss.str();
}

bool getSubscriberNodes(V_string& sub_nodes, const std::string topic)
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = this_node::getName();

  if (!execute("getSystemState", args, result, payload, true))
  {
    return false;
  }

  S_string node_set;
  {
    if ( payload.size() != 3 ) {
      printf( "Expected payload size = 3 (actual is %d)\n", payload.size() );
      std::cout << "  type: " << XmlRpcValueType( payload ) << std::endl << std::flush;
      return false;
    }
    for (int j = 0; j < payload[1].size(); ++j)
    {
      std::string t = payload[1][j][0];
      if ( t == topic ) {
        XmlRpc::XmlRpcValue val = payload[1][j][1];
        for (int k = 0; k < val.size(); ++k)
        {
          std::string name = payload[1][j][1][k];
          node_set.insert(name);
        }
      }
    }
  }

  sub_nodes.insert(sub_nodes.end(), node_set.begin(), node_set.end());

  return true;
}

bool getSubscriberHosts(V_string& sub_hosts, const std::string topic) 
{
  V_string sub_nodes;
  getSubscriberNodes( sub_nodes, topic );

  S_string host_set;
  for ( size_t i = 0; i < sub_nodes.size(); i++ ) {
    XmlRpc::XmlRpcValue args, result, payload;
    args[0] = this_node::getName();
    args[1] = sub_nodes[i];

    if (!execute("lookupNode", args, result, payload, true)) {
      return false;
    }
    if ( payload.getType() != XmlRpc::XmlRpcValue::TypeString ) {
      printf( "Expected payload to be string\n" );
      std::cout << "  type: " << XmlRpcValueType( payload ) << std::endl << std::flush;
      return false;
    }
    std::string uri = payload;
    std::string host;
    uint32_t port;
    // Split URI into
    if (!network::splitURI(uri, host, port)) {
      ROS_FATAL( "Couldn't parse the URI [%s] into a host:port pair.", uri.c_str());
      ROS_BREAK();
    }
    //else { 
    //  printf( "[xmlrpc_cpp]    %s -> %s : %d\n", uri.c_str(), host.c_str(), port );
    //}
    host_set.insert( host );
  }
  sub_hosts.insert( sub_hosts.end(), host_set.begin(), host_set.end() );
  return true;
}


bool getServiceClients(V_string& sub_hosts, const std::string service) 
{
  XmlRpc::XmlRpcValue args, result, payload;
  args[0] = this_node::getName();
  args[1] = service;

  if (!execute("getServiceClients", args, result, payload, true)) {
    return false;
  }

  for ( int j = 0; j < payload.size(); ++j ) {
    std::string ip_address = payload[j];
    sub_hosts.push_back( ip_address );
  }
  return true;
}

#if defined(__APPLE__)
boost::mutex g_xmlrpc_call_mutex;
#endif

bool execute(const std::string& method, const XmlRpc::XmlRpcValue& request, XmlRpc::XmlRpcValue& response, XmlRpc::XmlRpcValue& payload, bool wait_for_master)
{
  ros::WallTime start_time = ros::WallTime::now();

  std::string master_host = getHost();
  uint32_t master_port = getPort();
  XmlRpc::XmlRpcClient *c = XMLRPCManager::instance()->getXMLRPCClient(master_host, master_port, "/");
  bool printed = false;
  bool slept = false;
  bool ok = true;
  bool b = false;
  do
  {
    {
#if defined(__APPLE__)
      boost::mutex::scoped_lock lock(g_xmlrpc_call_mutex);
#endif

      b = c->execute(method.c_str(), request, response);
    }

    ok = !ros::isShuttingDown() && !XMLRPCManager::instance()->isShuttingDown();

    if (!b && ok)
    {
      if (!printed && wait_for_master)
      {
        ROS_ERROR("[%s] Failed to contact master at [%s:%d].  %s", method.c_str(), master_host.c_str(), master_port, wait_for_master ? "Retrying..." : "");
        printed = true;
      }

      if (!wait_for_master)
      {
        XMLRPCManager::instance()->releaseXMLRPCClient(c);
        return false;
      }

      if (!g_retry_timeout.isZero() && (ros::WallTime::now() - start_time) >= g_retry_timeout)
      {
        ROS_ERROR("[%s] Timed out trying to connect to the master after [%f] seconds", method.c_str(), g_retry_timeout.toSec());
        XMLRPCManager::instance()->releaseXMLRPCClient(c);
        return false;
      }

      ros::WallDuration(0.05).sleep();
      slept = true;
    }
    else
    {
      if (!XMLRPCManager::instance()->validateXmlrpcResponse(method, response, payload))
      {
        XMLRPCManager::instance()->releaseXMLRPCClient(c);

        return false;
      }

      break;
    }

    ok = !ros::isShuttingDown() && !XMLRPCManager::instance()->isShuttingDown();
  } while(ok);

  if (ok && slept)
  {
    ROS_INFO("Connected to master at [%s:%d]", master_host.c_str(), master_port);
  }

  XMLRPCManager::instance()->releaseXMLRPCClient(c);

  return b;
}

} // namespace master

} // namespace ros
