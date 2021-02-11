#pragma once

//T is enum of messages types
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

		Debug::Message("Client destructed");
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

			Debug::Message("Try to connect to ", host, ":", std::to_string(port), "...");

			m_connection = std::make_unique<Connection<T>>(m_context, asio::ip::tcp::socket(m_context), [this](Sent_message<T>& message) { OnMessageReceive(message.message, message.ID); });

			m_connection->ConnectToServer(endpoints);

			m_asioThread = std::thread([this]() { m_context.run(); });
		}
		catch (std::exception& e)
		{
			Debug::Message(e.what());
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


	inline void SendToServer(const Message<T>& msg)							{ Send({msg, SERVER}); }

	inline void SendToClient(const Message<T>& msg, uint32_t receiver_id)	{ Send({msg, receiver_id}); }

	inline void SendToAll(const Message<T>& msg)							{ Send({ msg, ALL_CLIENTS }); }
private:
	void Send(const Sent_message<T>& msg)
	{
		if (IsConnected())
			m_connection->Send(msg);
		else
			Debug::Message("The client is not connected to server");
	}

protected:
	virtual void OnMessageReceive(Message<T>& msg, uint32_t sender_id) = 0;

protected:
	asio::io_context				m_context;
	std::unique_ptr<Connection<T>>	m_connection;
private:
	std::thread						m_asioThread;
};