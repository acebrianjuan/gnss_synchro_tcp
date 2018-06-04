//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include "connection.hpp" // Must come before boost/serialization headers.
#include <boost/serialization/vector.hpp>

#include "gnss_synchro.h"


/// Downloads stock quote information from a server.
class client {
public:
    /// Constructor starts the asynchronous connect operation.
    client(boost::asio::io_service &io_service,
           const std::string &host, const std::string &service)
            : connection_(io_service) {
        // Resolve the host name into an IP address.
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(host, service);
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator =
                resolver.resolve(query);
        boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;

        // Start an asynchronous connect operation.
        connection_.socket().async_connect(endpoint,
                                           boost::bind(&client::handle_connect, this,
                                                       boost::asio::placeholders::error, ++endpoint_iterator));
    }

    /// Handle completion of a connect operation.
    void handle_connect(const boost::system::error_code &e,
                        boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
        if (!e) {
            // Successfully established connection. Start operation to read the list
            // of stocks. The connection::async_read() function will automatically
            // decode the data that is read from the underlying socket.
            connection_.async_read(stocks_,
                                   boost::bind(&client::handle_read, this,
                                               boost::asio::placeholders::error));
        } else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
            // Try the next endpoint.
            connection_.socket().close();
            boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
            connection_.socket().async_connect(endpoint,
                                               boost::bind(&client::handle_connect, this,
                                                           boost::asio::placeholders::error, ++endpoint_iterator));
        } else {
            // An error occurred. Log it and return. Since we are not starting a new
            // operation the io_service will run out of work to do and the client will
            // exit.
            std::cerr << e.message() << std::endl;
        }
    }

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code &e) {
        struct tm tstruct;
        if (!e) {
            // Print out the data that was received.
            for (std::size_t i = 0; i < stocks_.size(); ++i) {
                std::cout << "APVT packet number " << i << "\n";
                char buf[80];
                tstruct = *gmtime(&stocks_[i].time);
                double seconds = tstruct.tm_sec + stocks_[i].sec;
                strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:", &tstruct);
                std::string str_time = std::string(buf);
                std::cout << "Position timestamp: " << str_time << seconds << std::endl;
                std::cout << " HW ID= "<<stocks_[i].HW_ID << std::endl;
                std::cout << "WGS84 Latitude= "<<stocks_[i].Lat << " [deg]"<<std::endl;
                std::cout << "WGS84 Longitude= "<<stocks_[i].Long << " [deg]"<< std::endl;
                std::cout << "WGS84 Altitude= "<<stocks_[i].H <<  " [m]"<<std::endl;
                std::cout << "ECEF Position X= "<<stocks_[i].r[0] << " [m]"<< std::endl;
                std::cout << "ECEF Position Y= "<<stocks_[i].r[1] << " [m]"<< std::endl;
                std::cout << "ECEF Position Z= "<<stocks_[i].r[2] << " [m]"<< std::endl;
                std::cout << "ECEF Velocity X= "<<stocks_[i].v[0] << " [m/s]"<< std::endl;
                std::cout << "ECEF Velocity Y= "<<stocks_[i].v[1] << " [m/s]"<< std::endl;
                std::cout << "ECEF Velocity Z= "<<stocks_[i].v[2] << " [m/s]"<< std::endl;
                std::cout << "Attitude Roll= "<<stocks_[i].eul[0] << " [rad]"<<std::endl;
                std::cout << "Attitude Pitch= "<<stocks_[i].eul[1] <<" [rad]"<< std::endl;
                std::cout << "Attitude Yaw= "<<stocks_[i].eul[2] <<" [rad]"<< std::endl;
                std::cout << "Position and Attitude reliability level= "<<stocks_[i].reliability << std::endl;
                connection_.async_read(stocks_,
                                       boost::bind(&client::handle_read, this,
                                                   boost::asio::placeholders::error));
            }
        } else {
            // An error occurred.
            std::cerr << e.message() << std::endl;
        }

        // Since we are not starting a new operation the io_service will run out of
        // work to do and the client will exit.
    }

private:
    /// The connection to the server.
    connection connection_;

    /// The data received from the server.
    std::vector<APVT> stocks_;
};


int main(int argc, char *argv[]) {
    try {
        // Check command line arguments.
        if (argc != 3) {
            std::cerr << "Usage: client <host> <port>" << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;
        client client(io_service, argv[1], argv[2]);
        io_service.run();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

