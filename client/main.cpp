#include <iostream>

#include <libnet.hpp>

enum class MessageTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing
};

namespace forecasting::net
{
    typedef connection<MessageTypes> Connection;
    typedef message<MessageTypes> Message;
    typedef client_interface<MessageTypes> Client;
}

namespace fnet = forecasting::net;

class CustomClient : public fnet::Client
{
public:
};

bool bQuit = false;

/**
 * Signal handler.
 *
 * @param s signal
 */
void handle_exit_signal(int s){
  std::cout << "[CLIENT] terminating...\n";
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

void PingServer(fnet::Client& client)
{

    while(!bQuit) {
        fnet::Message msg;
        msg.header.id = MessageTypes::ServerPing;

        // Caution with this...
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        client.Send(msg);
        
		std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    listen_for_ctrl_c();

	CustomClient c;
	c.Connect("localhost", 60000);
 
    std::thread ping(PingServer, std::ref(c));

	while (!bQuit)
	{
		if (c.IsConnected())
		{
			while (!c.Incoming().empty())
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
  
    std::cout << "[CLIENT] Stopped successfully.\n";

    return 0;
}