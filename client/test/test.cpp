#include "test.h"
#include <assert.h>

#include <libnet.hpp>

enum class MessageTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
};

namespace forecasting::net
{
    typedef connection<MessageTypes> Connection;
    typedef message<MessageTypes> Message;
    typedef client_interface<MessageTypes> Client;
    typedef server_interface<MessageTypes> Server;
}

namespace fnet = forecasting::net;

class SensoryNodeClient : public fnet::Client
{
public:
};

class SensoryNode : public forecasting::net::Server
{
public:
	SensoryNode(uint16_t nPort) : fnet::Server(nPort)
	{

	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<fnet::Connection> client)
    {
		fnet::Message msg;
		msg.header.id = MessageTypes::ServerAccept;
		client->Send(msg);
		return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<fnet::Connection> client)
    {
		std::cout << "Removing client [" << client->get_id() << "]\n";
    }

    virtual void OnMessage(std::shared_ptr<fnet::Connection> client, fnet::Message& msg)
    {
		switch (msg.header.id)
		{
			case MessageTypes::ServerPing:
			{
				std::cout << "[" << client->get_id() << "]: Server Ping\n";
	
				// Simply bounce message back to client
				client->Send(msg);
			}
			break;
            default:
            break;
		}
    }
};

bool bQuit = false;

void echo_ping(SensoryNode& server) {
    while(!bQuit) {
        server.Update();
    }
}

void ping_server(SensoryNodeClient& c) {
    while(!bQuit) {
		fnet::Message msg;
		msg.header.id = MessageTypes::ServerPing;

		// Caution with this...
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		msg << timeNow;
		c.Send(msg);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void countdown(bool& bQuit) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    bQuit = true;
}

void on_message(SensoryNodeClient& c) {
	while (!bQuit)
	{
		if (c.IsConnected())
		{
			while(!c.Incoming().empty())
			{
				auto msg = c.Incoming().pop_front().msg;

				switch (msg.header.id)
				{
                case MessageTypes::ServerAccept:
                {
                    // Server has responded to a ping request
                    std::cout << "Server Accepted Connection\n";
                }
                break;

                case MessageTypes::ServerDeny:
                {
                    // Server has responded to a ping request
                    std::cout << "Server Denied Connection\n";
			        bQuit = true;
                }
                break;

                case MessageTypes::ServerPing:
                {
                    // Server has responded to a ping request
                    std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timeThen;
                    msg >> timeThen;
                    std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
                }
                break;
                }
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}
	}
}




void test_client_server_communication() {
	SensoryNode server(60000);
	server.Start();

	SensoryNodeClient c;
	c.Connect("localhost", 60000);

    std::thread server_ping(echo_ping, std::ref(server));
    std::thread client_ping(ping_server, std::ref(c));
    std::thread client_on_msg(on_message, std::ref(c));
    std::thread timer(countdown, std::ref(bQuit));

    server_ping.join();
    client_ping.join();
    client_on_msg.join();
    timer.join();
}

void test_net_message()
{
    fnet::Message msg; 

    int a = 2;
    char c = 'a';
    struct data
    {
        float x, y;
    };

    data point1 = { 1.1, 2.2 };

    std::cout << "Before add to the message.\n";
    std::cout << "a: " << a << std::endl
        << "c: " << c << std::endl
        << "point1(x, y): " << point1.x << ", " << point1.y << std::endl;

    msg << a << c << point1;

    int b;
    char d;
    data point2;

    msg >> point2 >> d >> b;
    std::cout << "After extract the message.\n";
    std::cout << "b: " << b << std::endl
        << "d: " << d << std::endl
        << "point2(x, y): " << point2.x << ", " << point2.y << std::endl;

    assert(a == b && c == d && point1.x == point2.x && point1.y == point2.y);
}