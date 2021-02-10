#pragma once

enum
{
	SERVER,
	ALL_CLIENTS
};

template <typename T>
class Server
{
protected:
	Server(uint16_t port)
		: m_acceptor(m_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{

	}

	~Server()
	{
		Stop();
	}
public:
	bool Run()
	{
		try
		{
			WaitForConnection();

			m_asioThread = std::thread([this]() { m_context.run(); });
		}
		catch (std::exception& e)
		{
			std::cout << "Failed to run server: \"" << e.what() <<'"'<< std::endl;
			return false;
		}

		std::cout << "Successfully run server\n" << m_acceptor.local_endpoint() << std::endl;
		return true;
	}

	void Stop()
	{
		m_context.stop();

		if (m_asioThread.joinable())
			m_asioThread.join();

		std::cout << "Server stopped\n";
	}

	void Update(size_t max_messages_handled = -1)
	{
		for (int i = 0; i < m_connections.size(); i++)
		{
			if (!m_connections[i]->IsConnected())
			{
				OnClientDisconnect(m_connections[i]);

				m_connections[i].reset();
				m_connections.erase(m_connections.begin() + i);

				i--;
			}

		}
	}

	void HandleMessage(uint32_t sender_id, Sent_message<T>& msg)
	{
		switch (msg.ID)
		{
		case SERVER:		OnMessageRecieve(msg.message, sender_id); break;
		case ALL_CLIENTS:	SendToAll({ msg.message, sender_id }, GetConnection(sender_id)); break;
		default:
		{
			std::shared_ptr<Connection<T>>& receiver = GetConnection(msg.ID);
			msg.ID = sender_id;
			Send(receiver, msg);
			break;
		}
		}
	}

	void Send(std::shared_ptr<Connection<T>> client, const Sent_message<T>& message)
	{
		if (client && client->IsConnected())
		{
			client->Send(message);
		}
		else
		{
			std::cout << "The client is not connected!\n";
		}
	}

	inline void SendToAll(const Message<T>& msg, std::shared_ptr<Connection<T>> ignore = nullptr) { SendToAll({ msg, SERVER }, ignore); }

protected:
	virtual bool OnClientConnect(const std::shared_ptr<Connection<T>> client)
	{
		return false;
	}

	virtual void OnClientDisconnect(const std::shared_ptr<Connection<T>> client) = 0;

	virtual void OnMessageRecieve(Message<T>& msg, uint32_t sender_id) = 0;

private:
	void SendToAll(const Sent_message<T>& message, std::shared_ptr<Connection<T>> ignore = nullptr)
	{
		for (std::shared_ptr<Connection<T>>& client : m_connections)
		{
			if (client != ignore)
				Send(client, message);
		}
	}

	void WaitForConnection()
	{
		m_acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket)
			{
				if (ec)
				{
					std::cout << "Couldn't connect client " << ec.message();
				}
				else
				{
					Errors::Error("New connection: ").Add(socket.remote_endpoint()).Add('\n');

					std::shared_ptr<Connection<T>> new_connection = 
						std::make_shared<Connection<T>>(GetNextID(), m_context, std::move(socket), m_inMsg, 
							[this](uint32_t sender_id, Sent_message<T>& msg) { HandleMessage(sender_id, msg); });

					if (OnClientConnect(new_connection))
					{
						m_connections.push_back(new_connection);

						m_connections.back()->ConnectToClient();
					}
					else
					{
						std::cout << "Connection denied\n";
					}
				}

				WaitForConnection();
			});
	}
	
	inline uint32_t GetNextID() const { return m_connections.size() + 2; }
	inline std::shared_ptr<Connection<T>>& GetConnection(uint32_t id) const { return (std::shared_ptr<Connection<T>>&)m_connections[id - 2]; }

private:
	tsque<Sent_message<T>>						m_inMsg;
	std::vector<std::shared_ptr<Connection<T>>> m_connections;
	asio::io_context							m_context;
	std::thread									m_asioThread;
	asio::ip::tcp::acceptor						m_acceptor;
};