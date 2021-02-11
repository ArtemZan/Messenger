#include "Network.h"
#include <Windows.h>

enum class MSG_TYPES
{
	TEXT
};

class MyServer : public Server<MSG_TYPES>
{
public:
	MyServer(uint16_t port)
		: Server(port)
	{

	}

	bool OnClientConnect(const std::shared_ptr<Connection<MSG_TYPES>> client) override
	{
		Debug::Message("Client connected to my server\n");
		return true;
	}

	void OnClientDisconnect(const std::shared_ptr<Connection<MSG_TYPES>> client) override
	{
		Debug::Message("Client disconnected from my server");
	}

	void OnMessageRecieve(Message<MSG_TYPES>& msg, uint32_t sender_id) override
	{
		switch (msg.header.type)
		{
		case MSG_TYPES::TEXT: 
		{
			char buf[100];
			msg.Read(buf, msg.header.size);
			Debug::Message("My server recieved a message: ").Add(buf);
			break;
		}
		default:
			break;
		}
	}

};

int main()
{
	MyServer server(5500);

	server.Run();

	for (int i = 0; i < 1000; i++)
	{
		server.Update();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (i == 700)
			Debug::Message("Server turns off in 30 seconds");
	}

	server.Stop();

	std::cin.get();
}

