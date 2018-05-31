//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>

#include "APVT.h"

/// Serves stock quote information to any client that connects to it.
class server
{
public:
  /// Constructor opens the acceptor and starts waiting for the first incoming
  /// connection.
  server(boost::asio::io_service& io_service, unsigned short port)
    : acceptor_(io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
  {
    // Create the data to be sent to each client.
      // This is a test Attitude, Position Velocity and Time server
    APVT s;
      struct timeval read_time;
      s.HW_ID=1; //hardware ID
      s.H= 58.3624; //WGS84 Altitude (m)
      s.Lat= 41.2749; //WGS84 Latitude (deg)
      s.Long=1.9875; //WGS84 Longitude (deg)
      s.eul[0]=0.23; //roll (radians)
      s.eul[1]=-0.15; //pitch (radians)
      s.eul[2]=0.01; //yaw (radians)
      s.v[0]=0; //ECEF velocity X (m/s)
      s.v[1]=0; //ECEF velocity Y (m/s)
      s.v[2]=0; //ECEF velocity Z (m/s)
      s.r[0]=4.797665934700000e6;//ECEF position X (m)
      s.r[1]=0.166493057100000e6;//ECEF position Y (m)
      s.r[2]=4.185450552700000e6;//ECEF position Z (m)
      s.reliability=4; //Position and attitude reliability level (0-4)
      for(int i=0;i<10;i++) {
          gettimeofday(&read_time, NULL);
          s.time=read_time.tv_sec;
          s.sec=(float)read_time.tv_usec / 1000000.0;
          stocks_.push_back(s);
          sleep(1);
      }
    // Start an accept operation for a new connection.
    connection_ptr new_conn(new connection(acceptor_.get_io_service()));
    acceptor_.async_accept(new_conn->socket(),
        boost::bind(&server::handle_accept, this,
          boost::asio::placeholders::error, new_conn));
  }

  /// Handle completion of a accept operation.
  void handle_accept(const boost::system::error_code& e, connection_ptr conn)
  {
    if (!e)
    {
      // Successfully accepted a new connection. Send the list of stocks to the
      // client. The connection::async_write() function will automatically
      // serialize the data structure for us.
      conn->async_write(stocks_,
          boost::bind(&server::handle_write, this,
            boost::asio::placeholders::error, conn));

      // Start an accept operation for a new connection.
      connection_ptr new_conn(new connection(acceptor_.get_io_service()));
      acceptor_.async_accept(new_conn->socket(),
          boost::bind(&server::handle_accept, this,
            boost::asio::placeholders::error, new_conn));
    }
    else
    {
      // An error occurred. Log it and return. Since we are not starting a new
      // accept operation the io_service will run out of work to do and the
      // server will exit.
      std::cerr << e.message() << std::endl;
    }
  }

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e, connection_ptr conn)
  {
    // Nothing to do. The socket will be closed automatically when the last
    // reference to the connection object goes away.
  }

private:
  /// The acceptor object used to accept incoming socket connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The data to be sent to each client.
  std::vector<APVT> stocks_;
};


int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>" << std::endl;
      return 1;
    }
    unsigned short port = boost::lexical_cast<unsigned short>(argv[1]);

    boost::asio::io_service io_service;
    server server(io_service, port);
    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

