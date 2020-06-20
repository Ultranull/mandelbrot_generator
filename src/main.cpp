#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <fstream>
#include <sstream>

#include <string>

#include <float.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "TextEditor.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "tools/App.h"
#include "tools/Mesh.h"
#include "tools/ShaderProgram.h"
#include "tools/FrameBuffer.h"


using namespace glm;
using namespace std;

/* TODO:
	[~] editable shaders
	 |- [x] make vert shader internal
	 |- [x] add imgui file editor
	 |- [x] syntax highlighting
	 |- [~] interval based compiling and saving
	 |- [~] output window for compile + link issues
	[x] dock windows
	[x] file name and resolution dialogue on save
	[~] clean up everything
	[x] add icon
	[-] pallete color loop setting
	[ ] merge julia and mandelbrot shaders

	[ ] orbital trap
	[ ] Pickover stalk
	-[-]-Newton-fractal-if-i-can-figure-out-how-


*/

std::vector<std::string> split_string(const std::string& str, const std::string& delimiter)
{
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + 1;
	}

	strings.push_back(str.substr(prev));

	return strings;
}

class Game :public App {
	Mesh mesh;

	Shader mandelbrotFragment, juliaFragment, vertexShader;
	Program mandelbrotShader, juliaShader;
	FrameBuffer mandelbrotBuffer;
	FrameBuffer juliaBuffer;

	TextEditor mandelbrotEditor;
	TextEditor juliaEditor;
	bool autoCompile_mandlebrot = false;
	bool autoCompile_julia = false;

	UniformBuffer palleteBuffer;
	vector<vec4> colorPallete;

	ImVec2 mandelbrotWindowSize, juliaWindowSize;

	float positionStepSize = 1, mandelbrotRadius = .5f, juliaRadius = .5f;
	float gradientMult = 1, power = 2.f, gradientOffset = 0;
	int iterations = 300;
	bool swapped = false, burningShip = false;
	vec2 position, relative;

	void initGL() {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClearColor(.1, .1, .1, 1);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_CULL_FACE);
	}

	void init() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.IniFilename = NULL;
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 130");
		ImGui::StyleColorsDark();

		mandelbrotFragment = Shader("mandelbrot.frag", GL_FRAGMENT_SHADER);
		juliaFragment = Shader("julia.frag", GL_FRAGMENT_SHADER);

		vertexShader = Shader(GL_VERTEX_SHADER,
			"#version 430 core\n"
			"precision highp float;"
			"layout(location = 0) in vec3 vertpos;"
			"layout(location = 1) in vec2 vertUV;"
			"out vec2 uv;"
			"void main() {"
			"	gl_Position = vec4(vertpos, 1);"
			"	uv = vertUV;"
			"}");

		printf("Compiling %s\n%s", 
			mandelbrotFragment.filename.c_str(),
			mandelbrotFragment.Compile().c_str());

		printf("Compiling %s\n%s",
			juliaFragment.filename.c_str(),
			juliaFragment.Compile().c_str());

		vertexShader.Compile();

		mandelbrotShader = Program(&vertexShader, &mandelbrotFragment);
		juliaShader = Program(&vertexShader, &juliaFragment);

		printf("linking mandelbrot %s", mandelbrotShader.Link().c_str());
		printf("linking julia %s", juliaShader.Link().c_str());

		mesh = Mesh({
			Vertex{vec3(-1,-1,0), vec2(0,1)},
			Vertex{vec3( 1, 1,0), vec2(1,0)},
			Vertex{vec3(-1, 1,0), vec2(0,0)},

			Vertex{vec3(-1,-1,0), vec2(0,1)},
			Vertex{vec3( 1,-1,0), vec2(1,1)},
			Vertex{vec3( 1, 1,0), vec2(1,0)},
		});


		mandelbrotEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
		mandelbrotEditor.SetText(mandelbrotFragment.content);

		juliaEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
		juliaEditor.SetText(juliaFragment.content);

		mandelbrotWindowSize = ImVec2(800, 800);
		juliaWindowSize = ImVec2(800, 800);

		mandelbrotBuffer = FrameBuffer(2500, 2500);
		mandelbrotBuffer.addTexture2D("color", GL_RGBA, GL_RGBA, GL_COLOR_ATTACHMENT0);
		mandelbrotBuffer.addDepth();
		mandelbrotBuffer.drawBuffers();

		juliaBuffer = FrameBuffer(2500, 2500);
		juliaBuffer.addTexture2D("color", GL_RGBA, GL_RGBA, GL_COLOR_ATTACHMENT0);
		juliaBuffer.addDepth();
		juliaBuffer.drawBuffers();

		//colorPallete = vector<vec4>(10, vec4(0));
		colorPallete.push_back(vec4(1, 0, 0, 1));
		colorPallete.push_back(vec4(0, 1, 0, 1));
		colorPallete.push_back(vec4(0, 0, 1, 1));
		colorPallete.push_back(vec4(0, 1, 1, 1));
		colorPallete.push_back(vec4(1, 0, 1, 1));
		colorPallete.push_back(vec4(1, 1, 0, 1));
	
		palleteBuffer = UniformBuffer();
		palleteBuffer.bind();
		palleteBuffer.setData<vec4>(colorPallete, GL_DYNAMIC_DRAW);
		palleteBuffer.blockBinding(mandelbrotShader.getProgramID(), 0, "pallete");
		palleteBuffer.unbind();
	}

	void onClose() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void update(float delta) {

		glfwSetWindowTitle(window, to_string(fps).c_str());
		
	}

	void FpsWindow() {
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoScrollbar;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
		ImGui::SetNextWindowBgAlpha(0.5f);
		ImGui::SetNextWindowPos(ImVec2(0, 0),ImGuiCond_Once);
		ImGui::Begin("w1", nullptr, window_flags);
		ImGui::Text(
			"Application average %.3f ms/frame (%.1f FPS, %i engine FPS)",
			1000.0f / ImGui::GetIO().Framerate,
			ImGui::GetIO().Framerate,
			fps);
		ImGui::End();
	}

	void ColorSettingsWindow() {
		ImGui::Begin("Color settings"); {

			ImGui::SliderFloat("gradient multiplier", &gradientMult, 1, 500, "%f", 10.f);
			ImGui::DragFloat("Gradient Offset", &gradientOffset, 0.001);
			ImGui::Text("Gradient Pallete");
			ImGui::Indent();

			if (ImGui::Button("Add color")) {
				colorPallete.push_back(vec4(0));
			}

			int move_from = -1, move_to = -1;
			for (int i = 0; i < colorPallete.size(); i++) {
				std::string colorLebel = "##pallete" + std::to_string(i);
				ImGui::ColorEdit3(colorLebel.c_str(), reinterpret_cast<float*>(&colorPallete[i]));
				ImGui::SameLine();
				if (ImGui::Button((std::string("X") + colorLebel).c_str())) {
					colorPallete.erase(colorPallete.begin() + i);
					i--;
					continue;
				}

				ImGui::SameLine();
				ImGui::Selectable((std::string("=") + colorLebel).c_str());

				ImGuiDragDropFlags src_flags = 0;
				src_flags |= ImGuiDragDropFlags_SourceNoDisableHover;
				src_flags |= ImGuiDragDropFlags_SourceNoHoldToOpenOthers;
				if (ImGui::BeginDragDropSource(src_flags))
				{
					ImVec4 imcolor(colorPallete[i].x, colorPallete[i].y, colorPallete[i].z, 1);
					ImGui::ColorButton("test", imcolor);
					ImGui::SameLine();
					ImGui::Text("(%f, %f, %f)", imcolor.x, imcolor.y, imcolor.z);
					ImGui::SetDragDropPayload("colorIndex", &i, sizeof(int));
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					ImGuiDragDropFlags target_flags = 0;
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("colorIndex", target_flags))
					{
						move_from = *(const int*)payload->Data;
						move_to = i;
					}
					ImGui::EndDragDropTarget();
				}

			}

			if (move_from != -1 && move_to != -1)
			{
				vec4 tmp = colorPallete[move_from];
				colorPallete.erase(colorPallete.begin() + move_from);

				auto it = colorPallete.begin();
				colorPallete.insert(it + move_to, tmp);

				ImGui::SetDragDropPayload("colorIndex", &move_to, sizeof(int));
			}


		}ImGui::End();

		palleteBuffer.bind();
		palleteBuffer.setData<vec4>(colorPallete, GL_DYNAMIC_DRAW);
		palleteBuffer.unbind();
	}

	void SettingsWindow() {
		ImGuiWindowFlags window_flags = 0;
		//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
		ImGui::Begin("Settings", nullptr, window_flags); {
			ImGui::SliderFloat("Step size", &positionStepSize, FLT_EPSILON, .0005, "%.10f", 10.f);

			ImGui::DragFloat2("Position", reinterpret_cast<float*>(&position),positionStepSize, -4, 4, "%.10f", 10.f);

			ImGui::DragFloat2("Relative", reinterpret_cast<float*>(&relative), positionStepSize, -4, 4, "%.10f", 10.f);

			ImGui::SliderFloat("Mandelbrot Zoom", &mandelbrotRadius, 0.00000001, 1, "%.10f", 10.f);
			ImGui::SliderFloat("Julia Zoom", &juliaRadius, 0.00000001, 1, "%.10f", 10.f);

			ImGui::SliderInt("Iterations", &iterations, 1, 2048);
			ImGui::DragFloat("Power", &power, .01f, 1, 10);


			ImGui::Checkbox("Swapped", &swapped);
			ImGui::Checkbox("Absolute", &burningShip);
			ImGui::SameLine();
			if (ImGui::Button("Reset")) {
				positionStepSize = 1; 
				mandelbrotRadius = .5f;
				juliaRadius = .5f;
				gradientMult = 1;
				iterations = 300;
				swapped = false;
				burningShip = false;
				position = vec4(0);
				relative = vec4(0);
				power = 2;
			}

		}ImGui::End();
	}

	TextEditor::ErrorMarkers parseError(string errors) {
		TextEditor::ErrorMarkers markers;
		vector<string> lines = split_string(errors, "\n");
		for (string& line: lines) {
			if (line.empty())break;
			vector<string> chunk = split_string(line, ":");
			int lineNumber = stoi(chunk[0].substr(2, chunk[0].find(")")-1));

			markers.insert(std::make_pair(lineNumber, chunk[2]));
		}
		return markers;
	}

	void compileShader(TextEditor& editor, Shader& shader, Program& program) {
		editor.SetErrorMarkers(TextEditor::ErrorMarkers());
		shader.content = editor.GetText();
		string error = shader.Compile();
		if (error != "") {
			editor.SetErrorMarkers(parseError(error));
		}
		else {
			shader.Save();
			program.Link();
		}
	}

	void editorWindow(std::string name, TextEditor& editor, Shader& shader, Program& program) {

		ImGui::Begin(name.c_str(),nullptr, ImGuiWindowFlags_MenuBar); {
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Save"))
					{
						auto textToSave = editor.GetText();
						shader.Save();
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					bool ro = editor.IsReadOnly();
					if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
						editor.SetReadOnly(ro);
					ImGui::Separator();

					if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
						editor.Undo();
					if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
						editor.Redo();

					ImGui::Separator();

					if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
						editor.Copy();
					if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
						editor.Cut();
					if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
						editor.Delete();
					if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
						editor.Paste();

					ImGui::Separator();

					if (ImGui::MenuItem("Select all", nullptr, nullptr))
						editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem("Dark palette"))
						editor.SetPalette(TextEditor::GetDarkPalette());
					if (ImGui::MenuItem("Light palette"))
						editor.SetPalette(TextEditor::GetLightPalette());
					if (ImGui::MenuItem("Retro blue palette"))
						editor.SetPalette(TextEditor::GetRetroBluePalette());
					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Compile")) {
					compileShader(editor, shader, program);
				}
				//if (ImGui::MenuItem("Toggle auto-compile")) {

				//}

				ImGui::EndMenuBar();
			}
			editor.Render(name.c_str());
		}ImGui::End();

		if (editor.IsTextChanged()) {
			//compileShader(editor, shader, program);
		}
	}

	void mandelbrotWindow() {
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_MenuBar;
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
		ImGui::Begin("mandelbrot", nullptr, window_flags);
		{
			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();
			ImVec2 pos = ImGui::GetWindowPos();
			mandelbrotWindowSize = ImGui::GetWindowSize();


			ImGui::GetWindowDrawList()->AddImage(
				(void*)mandelbrotBuffer.getTexture("color").id,
				ImVec2(vMin.x + pos.x, vMin.y + pos.y),
				ImVec2(vMax.x + pos.x, vMax.y + pos.y));

			if (ImGui::BeginMenuBar())
			{

				if (ImGui::MenuItem("Save"))
				{
					ivec2 size(2500, 2500);
					renderFractal(
						size.x,
						size.y,
						mandelbrotRadius,
						mandelbrotBuffer,
						mandelbrotShader);
					unsigned char* data = new unsigned char[size.x * size.y * 4];
					glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
					stbi_write_png("Mandelbrot.png", size.x, size.y, 4, data, size.x * 4);
					delete[] data;
				}

				if (ImGui::MenuItem("Save as..."))
					ImGui::OpenPopup("save as");

				if (ImGui::BeginPopupModal("save as", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Note: save will be in PNG format!");
					ImGui::Separator();

					static char textinput[128] = "image.png";
					static int resolution[2] = { 2500, 2500 };

					ImGui::InputText("File Name", textinput, 128);
					ImGui::InputInt2("Resolution", resolution);

					if (ImGui::Button("OK", ImVec2(120, 0))) {
						ivec2 size(resolution[0], resolution[1]);
						renderFractal(
							size.x,
							size.y,
							mandelbrotRadius,
							mandelbrotBuffer,
							mandelbrotShader);
						unsigned char* data = new unsigned char[size.x * size.y * 4];
						glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
						stbi_write_png(textinput, size.x, size.y, 4, data, size.x * 4);
						delete[] data;
						ImGui::CloseCurrentPopup();
					}
					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}

				ImGui::EndMenuBar();
			}
		}
		ImGui::End();
	}

	void juliaWindow() {
		ImGuiWindowFlags window_flags = 0;
		//window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		window_flags |= ImGuiWindowFlags_MenuBar;
		ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
		ImGui::Begin("julia", nullptr, window_flags);
		{
			ImVec2 vMin = ImGui::GetWindowContentRegionMin();
			ImVec2 vMax = ImGui::GetWindowContentRegionMax();
			ImVec2 pos = ImGui::GetWindowPos();
			juliaWindowSize = ImGui::GetWindowSize();

			ImGui::GetWindowDrawList()->AddImage(
				(void*)juliaBuffer.getTexture("color").id,
				ImVec2(vMin.x + pos.x, vMin.y + pos.y),
				ImVec2(vMax.x + pos.x, vMax.y + pos.y));

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::MenuItem("Save"))
				{
					ivec2 size(2500, 2500);
					renderFractal(
						size.x,
						size.y,
						juliaRadius,
						juliaBuffer,
						juliaShader);
					unsigned char* data = new unsigned char[size.x * size.y * 4];
					glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
					stbi_write_png("Julia.png", size.x, size.y, 4, data, size.x * 4);
					delete[] data;
				}

				if (ImGui::MenuItem("Save as..."))
					ImGui::OpenPopup("save as");

				if (ImGui::BeginPopupModal("save as", NULL, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Text("Note: save will be in PNG format!");
					ImGui::Separator();

					static char textinput[128]= "image.png";
					static int resolution[2] = { 2500, 2500 };

					ImGui::InputText("File Name", textinput, 128);
					ImGui::InputInt2("Resolution", resolution);

					if (ImGui::Button("OK", ImVec2(120, 0))) { 
						ivec2 size(resolution[0], resolution[1]);
						renderFractal(
							size.x,
							size.y,
							juliaRadius,
							juliaBuffer,
							juliaShader);
						unsigned char* data = new unsigned char[size.x * size.y * 4];
						glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, data);
						stbi_write_png(textinput, size.x, size.y, 4, data, size.x * 4);
						delete[] data;
						ImGui::CloseCurrentPopup(); 
					}
					ImGui::SetItemDefaultFocus();
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
					ImGui::EndPopup();
				}

				ImGui::EndMenuBar();
			}
		}
		ImGui::End();
	}

	void renderFractal(int width, int height, float radius, FrameBuffer& buffer, Program& shader) {
		buffer.bind();
		buffer.ResizeTexture("color", width, height);
		glViewport(0, 0, width, height);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		shader.bind();
		shader.setUniform("time", ticks);
		shader.setUniform("relative", &relative);
		shader.setUniform("position", &position);
		shader.setUniform("radius", radius);
		shader.setUniform("swapped", (int)swapped);
		shader.setUniform("iterations", iterations);
		shader.setUniform("resolution", &vec2(width, height));
		shader.setUniform("palleteSize", (int)colorPallete.size());
		shader.setUniform("gradientMult", gradientMult);
		shader.setUniform("burningShip", (int)burningShip);
		shader.setUniform("power", power);
		shader.setUniform("gradientOffset", gradientOffset);

		mesh.renderVertices(GL_TRIANGLES);

	}

	void render(float delta) {

		renderFractal(
			mandelbrotWindowSize.x,
			mandelbrotWindowSize.y,
			mandelbrotRadius,
			mandelbrotBuffer,
			mandelbrotShader);

		renderFractal(
			juliaWindowSize.x,
			juliaWindowSize.y,
			juliaRadius,
			juliaBuffer,
			juliaShader);

		mesh.renderVertices(GL_TRIANGLES);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		FrameBuffer::bindDefualt();
		viewportinit(window);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		mandelbrotWindow();
		juliaWindow();
		
		FpsWindow(); 
		SettingsWindow();
		ColorSettingsWindow();

		editorWindow("mandelbrot shader", mandelbrotEditor, mandelbrotFragment, mandelbrotShader);
		editorWindow("julia shader", juliaEditor, juliaFragment, juliaShader);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void inputListener(float delta) {
		running = glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS;

	}
public:

	Game(int width, int height, const char *title) :App(width, height, title) {

	}
};

int main() {
	Game game(700, 700, "");
	game.start();
	return 0;
}
