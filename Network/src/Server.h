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
	//m_acceptor is responsible for waiting for new connections
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
			//Actually this thread doesn't wait, instead asio thread is waiting
			WaitForConnection();

			//When asio has work to do we can run it in it's own thread
			m_asioThread = std::thread([this]() { m_context.run(); });
		}
		catch (std::exception& e)
		{
			Debug::Message("Failed to run server: \"", e.what(), '"');
			return false;
		}

		Debug::Message("Successfully ran server\n", m_acceptor.local_endpoint());
		return true;
	}

	void Stop()
	{
		m_context.stop();

		if (m_asioThread.joinable())
			m_asioThread.join();

		Debug::Message("Server stopped");
	}

	void Update(size_t max_messages_handled = -1)
	{
		//All we do here is checking whether all clients are still connected
		for (int i = 0; i < m_connections.size(); i++)
		{
			//if not...
			if (!m_connections[i]->IsConnected())
			{
				//disconnect it from server side
				OnClientDisconnect(m_connections[i]);

				m_freeIDs.push_back(m_connections[i]->m_ID);

				m_connections[i].reset();
				m_connections.erase(m_connections.begin() + i);


				i--;
			}

		}
	}

	//This funcion is called when server receive a message.
	void HandleMessage(uint32_t sender_id, Sent_message<T>& msg)
	{
		//msg.ID is receiver ID
		//IDs SERVER and ALL_CLIENTS are reserved
		switch (msg.ID)
		{
		//if the message is for server handle it
		case SERVER:		OnMessageRecieve(msg.message, sender_id); break;
		//if it is for everybody (without sender) send it to everybody
		case ALL_CLIENTS:	SendToAll({ msg.message, sender_id }, GetConnection(sender_id)); break;
		//else send it to certain client
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
			Debug::Message("Can't send message, because the client is not connected!");
		}
	}

	//Send to everybody (maybe with one client ignored) from server
	inline void SendToAll(const Message<T>& msg, std::shared_ptr<Connection<T>> ignore = nullptr) { SendToAll({ msg, SERVER }, ignore); }

protected:
	//To be overriden
	//Returns whether to accept connections
	virtual bool OnClientConnect(const std::shared_ptr<Connection<T>> client)
	{
		return false;
	}

	virtual void OnClientDisconnect(const std::shared_ptr<Connection<T>> client) = 0;

	virtual void OnMessageRecieve(Message<T>& msg, uint32_t sender_id) = 0;

private:
	//Send to everybody (maybe with one client ignored) from somebody
	void SendToAll(const Sent_message<T>& message, std::shared_ptr<Connection<T>> ignore = nullptr)
	{
		for (std::shared_ptr<Connection<T>>& client : m_connections)
		{
			if (client != ignore)
				Send(client, message);
		}
	}

	//says asio to wait for connection and returns almost immediately
	void WaitForConnection()
	{
		m_acceptor.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket)
			{
				if (ec)
				{
					Debug::Message("Couldn't connect client ", ec.message());
				}
				else
				{
					Debug::Message("New connection: ", socket.remote_endpoint(), '\n');

					
					std::shared_ptr<Connection<T>> new_connection = 
						std::make_shared<Connection<T>>(GetNextID(), m_context, std::move(socket), 
							[this](uint32_t sender_id, Sent_message<T>& msg) { HandleMessage(sender_id, msg); });

					if (OnClientConnect(new_connection))
					{
						m_connections.push_back(new_connection);

						m_connections.back()->ConnectToClient();
					}
					else
					{
						Debug::Message("Connection denied");
					}
				}

				//If asio doesn't have work it stop (but we don't want this) so
				//when new connection is handled we start 
				//waiting for another connection again so asio thread always has work to do.
				WaitForConnection();
			});
	}
	

	uint32_t GetNextID() 
	{ 
		if(m_freeIDs.empty())
			return m_connections.size() + 2;
		uint32_t r = m_freeIDs.front();
		m_freeIDs.erase(m_freeIDs.begin());
		return r;
	}


	std::shared_ptr<Connection<T>>& GetConnection(uint32_t id) 
	{ 
		for (int i = 0; i < m_connections.size(); i++)
		{
			if (m_connections[i]->m_ID == id)
				return m_connections[i];
		}
	}

private:
	std::vector<uint32_t>						m_freeIDs;
	std::vector<std::shared_ptr<Connection<T>>> m_connections;
	asio::io_context							m_context;
	std::thread									m_asioThread;
	asio::ip::tcp::acceptor						m_acceptor;
};