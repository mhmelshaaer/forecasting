#include <iostream>

// For listening for Ctrl + c
#include <cstdio>

#include <libnet.hpp>

enum class MessageTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	ServerMessage,
};

namespace forecasting::net
{
    typedef connection<MessageTypes> Connection;
    typedef message<MessageTypes> Message;
    typedef server_interface<MessageTypes> Server;
}

namespace fnet = forecasting::net;

class CustomServer : public forecasting::net::Server
{
public:
	CustomServer(uint16_t nPort) : fnet::Server(nPort)
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

/**
 * Signal handler.
 *
 * @param s signal
 */
void handle_exit_signal(int s){
  std::cout << "[SERVER] terminating...\n";
  bQuit = true;
}

/**
 * When hitting ctrl + c while running the client, the program will exist
 * expectedly.
 */
void listen_for_ctrl_c() {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = handle_exit_signal;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}

int main() {
	CustomServer server(60000);
	server.Start();

    while(!bQuit) {
        server.Update();
    }

	return 0;
}
