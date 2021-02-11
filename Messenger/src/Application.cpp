#include "pch.h"
#include "Network.h"
#include "Apllication.h"

Application Application::Instance;


Application::Application()
	:m_title("Messenger"), m_width(960), m_height(540)
{
	Init(m_title, m_width, m_height);
	Connect("127.0.0.1", 5500);
	Run();
}

//many things will change here
void Application::OnUpdate(float dTime)
{
	ImGuiIO& io = ImGui::GetIO();

	{
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= !ImGuiWindowFlags_MenuBar;

		ImGui::SetNextWindowPos({0, 0});
		ImGui::SetNextWindowSize({ io.DisplaySize.x / 2.5f , io.DisplaySize.y });

		ImGui::Begin(" ", 0, window_flags);

		static char msg_text[100] = "Message";

		ImGui::InputText("Message", msg_text, 100);

		if (ImGui::Button("Send", { 50, 20 }))
		{
			Message<MSG_TYPES> msg(MSG_TYPES::TEXT);
			msg.Write(msg_text, strlen(msg_text) + 1);
			SendToServer(msg);
			SendToAll(msg);
		}

		if (!m_message.empty())
		{
			ImGui::PushStyleColor(0, { 0.8f, 1.0f, 0.6f, 1.0f });
			ImGui::Text("Message from %d: ", m_senderId);
			ImGui::SameLine();
			ImGui::TextWrapped(m_message.c_str());
			ImGui::PopStyleColor();
		}


		ImGui::End();
	}
	
	{
		std::string message;
		while (Debug::GetNextMessage(message))
		{
			m_debugMessages.push_back(message);
		}
		ImGui::SetNextWindowPos({io.DisplaySize. x / 2.0f, 0.0f});
		ImGui::SetNextWindowSize({ io.DisplaySize.x / 2.5f, io.DisplaySize.y / 2.0f });
		ImGui::Begin("Debug");
		for (const std::string& message : m_debugMessages)
		{	
			ImGui::PushStyleColor(0, { 1.0f, 1.0f, 0.3f, 1.0f });
			ImGui::TextWrapped(message.c_str());
			ImGui::PopStyleColor();
		}
		ImGui::End();
	}

	ImGui::ShowDemoWindow();
}

void Application::OnWindowClose()
{
	m_running = false;
}

void Application::OnWindowResize(int new_width, int new_height)
{
	m_width = new_width;
	m_height = new_height;
}

void Application::OnMouseMove(double x, double y)
{
}

void Application::OnMouseClick(int button, int action, int mods)
{
}


void Application::OnMessageReceive(Message<MSG_TYPES>& msg, uint32_t sender_id)
{
	m_message.resize(msg.header.size);
	msg.Read(m_message.data(), msg.header.size);
	m_senderId = sender_id;
}


void Application::Shutdown()
{
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();

	glfwTerminate();
}

void Application::Init(const char* title, size_t width, size_t height)
{
	if (glfwInit() == GLFW_FALSE)
	{
		assert(0);
	};

	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);

	glfwMakeContextCurrent(m_window);

	glewInit();

	glfwSetWindowUserPointer(m_window, this);


	glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			app->OnWindowClose();
		});


	glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			app->OnWindowResize(width, height);
		});



	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double width, double height)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			app->OnMouseMove(width, height);
		});


	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
		{
			Application* app = (Application*)glfwGetWindowUserPointer(window);
			app->OnMouseClick(button, action, mods);
		});



	ImGuiContext* ImGuiContext = ImGui::CreateContext();

	ImGui::SetCurrentContext(ImGuiContext);

	ImGui_ImplGlfw_InitForOpenGL(m_window, true);
	ImGui_ImplOpenGL3_Init();
}

void Application::Run()
{
	while (m_running)
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		OnUpdate(0);

		ImGui::Render();

		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	Shutdown();
}