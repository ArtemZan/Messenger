#pragma once

template <typename T>
class Client
{
protected:
	Client()
	{

	}

	~Client()
	{
		Disconnect();

		std::cout << "Client destructed\n";
	}

public:
	bool IsConnected() const
	{
		if (m_connection)
			return m_connection->IsConnected();
		else
			return false;
	}

	bool Connect(const std::string& host, const uint16_t port)
	{
		try
		{
			asio::ip::tcp::resolver resolver(m_context);
			asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

			std::cout << "Try to connect to " << host << ": " << std::to_string(port) << "...\n";

			m_connection = std::make_unique<Connection<T>>(m_context, asio::ip::tcp::socket(m_context), m_inMsg, [this](Sent_message<T>& message) { OnMessageReceive(message.message, message.ID); });

			m_connection->ConnectToServer(endpoints);

			m_asioThread = std::thread([this]() { m_context.run(); });
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}

		return true;
	}

	void Disconnect()
	{
		if(IsConnected())
			m_connection->Disconnect();

		m_context.stop();

		if (m_asioThread.joinable())
			m_asioThread.join();

		m_connection.release();
	}

	void Update(size_t max_messages_handled = -1)
	{
		while (max_messages_handled-- && !m_inMsg.empty())
		{
			Sent_message<T> msg = m_inMsg.pop_front();


			OnMessageReceive(msg.message);
		}
	}

	inline void SendToServer(const Message<T>& msg)							{ Send({msg, SERVER}); }

	inline void SendToClient(const Message<T>& msg, uint32_t receiver_id)	{ Send({msg, receiver_id}); }

	inline void SendToAll(const Message<T>& msg)							{ Send({ msg, ALL_CLIENTS }); }


	inline tsque<Sent_message<T>>& Incoming() const { return m_inMsg; }

	//inline uint32_t GetID() const { return m_connection->GetID(); }

private:
	void Send(const Sent_message<T>& msg)
	{
		if (IsConnected())
			m_connection->Send(msg);
		else
			std::cout << "The client is not connected to server\n";
	}

protected:
	virtual void OnMessageReceive(Message<T>& msg, uint32_t sender_id) = 0;

protected:
	asio::io_context				m_context;
	std::unique_ptr<Connection<T>>	m_connection;
private:
	std::thread						m_asioThread;
	tsque<Sent_message<T>>			m_inMsg;
};