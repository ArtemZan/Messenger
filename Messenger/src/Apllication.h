#pragma once

enum class MSG_TYPES
{
	TEXT
};

class Application : public Client<MSG_TYPES>
{
private:
	Application();


	void Init(const char* title, size_t width, size_t height);

	void Run();

	void Shutdown();

	void OnUpdate(float dTime);

	void OnWindowClose();
	void OnWindowResize(int new_width, int new_height);

	void OnMouseMove(double x, double y);
	void OnMouseClick(int button, int action, int mods);

	void OnMessageReceive(Message<MSG_TYPES>& msg, uint32_t sender_id) override;
public:
	static inline Application* Create() { return &Instance; }
private:
	static Application Instance;

	GLFWwindow* m_window;

	const char* m_title;
	bool		m_running = true;
	int			m_width;
	int			m_height;

	std::string					m_message;		 //TEMPORARY
	uint32_t					m_senderId;		 //TEMPORARY
	std::vector<std::string>	m_debugMessages;
};