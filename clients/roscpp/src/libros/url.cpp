/*
 * Copyright (C) 2017, SRI International.
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

#include <string>
#include <algorithm>
#include <iostream>
#include <functional>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ros/url.h"

using namespace std;

std::string uri_to_ip_address( const std::string uri ) {
  URLParser uri_parser( uri );
  std::string ip_address = uri_parser.get_ip_address();
  return ip_address;
}

bool is_uri_match( const std::string uri, const std::string ip_address ) {
  URLParser uri_parser( uri );
  return uri_parser.match( ip_address );
}

URLParser::URLParser(const string url_s) 
  : url_( url_s )
{
  //cout << "parsing " << url_s << std::endl;
  this->parse(url_s);
}

URLParser::~URLParser()
{
}

std::string URLParser::get_host( ) {
  return host_;
}

bool is_local( const std::string ip_address ) {
  struct sockaddr_in addr;
  inet_aton( ip_address.c_str(), &addr.sin_addr );
  //int addr_3 = ( addr.sin_addr.s_addr & 0xff000000 ) >> 24;
  //int addr_2 = ( addr.sin_addr.s_addr & 0x00ff0000 ) >> 16;
  int addr_1 = ( addr.sin_addr.s_addr & 0x0000ff00 ) >> 8;
  int addr_0 = ( addr.sin_addr.s_addr & 0x000000ff );
  if ( addr_0 == 127 || ( addr_0 == 169 && addr_1 == 254 ) ) {
    //std::cout << ip_address << ": " << addr_0 << "." << addr_1 << "." << addr_2 << "." << addr_3 << std::endl;
    //std::cout << ip_address << " is local address!" << std::endl;
    return true;
  }
  //std::cout << ip_address << " is not local address!" << std::endl;
  return false;
}

bool URLParser::match( const string ip_address ) {
  if ( ip_address_ == ip_address ) {
    return true;
  }
  if ( is_local( ip_address ) ) {
    return is_local( ip_address_ );
  }
  return false;
}

void URLParser::parse(const string& url_s)
{
    const string prot_end("://");
    string::const_iterator prot_i = search(url_s.begin(), url_s.end(),
                                           prot_end.begin(), prot_end.end());
    if ( prot_i == url_s.end() ) {
      prot_i = url_s.begin();
    }
    else {
      protocol_.reserve(distance(url_s.begin(), prot_i));
      transform(url_s.begin(), prot_i,
          back_inserter(protocol_),
          ptr_fun<int,int>(tolower)); // protocol is icase
      advance(prot_i, prot_end.length());
    }
    string::const_iterator port_i = find(prot_i, url_s.end(), ':');
    string::const_iterator path_i = find(prot_i, url_s.end(), '/');
    if ( port_i != url_s.end() ) {
      // port number
      host_.reserve(distance(prot_i, port_i));
      transform(prot_i, port_i,
          back_inserter(host_),
          ptr_fun<int,int>(tolower)); // host is icase
      port_.assign(port_i+1, path_i);
    }
    else {
      host_.reserve(distance(prot_i, path_i));
      transform(prot_i, path_i, back_inserter(host_), ptr_fun<int,int>(tolower)); 
    }
    string::const_iterator query_i = find(path_i, url_s.end(), '?');
    path_.assign(path_i, query_i);
    if( query_i != url_s.end() ) {
      ++query_i;
    }
    query_.assign(query_i, url_s.end());
    // convert host to ip address
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host_.c_str(), NULL, &hints, &res);
    if (err != 0) {
      printf("getaddrinfo: %s\n", gai_strerror(err));
    }
    else {
      char addrstr[100];
      void *ptr;
      while (res) {
        inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

        switch (res->ai_family) {
          case AF_INET:
            ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
            break;
          case AF_INET6:
            ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
            break;
        }
        inet_ntop (res->ai_family, ptr, addrstr, 100);
        ip_address_ = addrstr;
        res = res->ai_next;
      }
    }
}
