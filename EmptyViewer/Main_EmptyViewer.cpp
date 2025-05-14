#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

int Width = 512;
int Height = 512;
std::vector<float> OutputImage;

int gNumVertices = 0;
int gNumTriangles = 0;
int* gIndexBuffer = nullptr;
std::vector<vec4> vertices;
std::vector<float> depthBuffer;

void create_scene() {
	int width = 32, height = 16;
	gNumVertices = (height - 2) * width + 2;
	gNumTriangles = (height - 2) * (width - 1) * 2;

	vertices.resize(gNumVertices);
	gIndexBuffer = new int[3 * gNumTriangles];

	// 위도/경도 기반 정점 생성
	int t = 0;
	for (int j = 1; j < height - 1; ++j) {
		for (int i = 0; i < width; ++i) {
			float theta = float(j) / (height - 1) * pi<float>();
			float phi = float(i) / (width - 1) * pi<float>() * 2;
			vertices[t++] = vec4(sinf(theta) * cosf(phi), cosf(theta), -sinf(theta) * sinf(phi), 1.0f);
		}
	}

	// 극점 추가
	vertices[t++] = vec4(0, 1, 0, 1);  // 북극
	vertices[t++] = vec4(0, -1, 0, 1); // 남극

	// 삼각형 인덱스 설정 (퀘드 2개로 분할)
	t = 0;
	for (int j = 0; j < height - 3; ++j) {
		for (int i = 0; i < width - 1; ++i) {
			int curr = j * width + i;
			gIndexBuffer[t++] = curr;
			gIndexBuffer[t++] = curr + width + 1;
			gIndexBuffer[t++] = curr + 1;

			gIndexBuffer[t++] = curr;
			gIndexBuffer[t++] = curr + width;
			gIndexBuffer[t++] = curr + width + 1;
		}
	}

	// 북극/남극 삼각형 연결
	for (int i = 0; i < width - 1; ++i) {
		gIndexBuffer[t++] = (height - 2) * width;     // 북극
		gIndexBuffer[t++] = i;
		gIndexBuffer[t++] = i + 1;

		gIndexBuffer[t++] = (height - 2) * width + 1; // 남극
		gIndexBuffer[t++] = (height - 3) * width + (i + 1);
		gIndexBuffer[t++] = (height - 3) * width + i;
	}
}



vec3 viewport_transform(vec4 v) {
	float x = (v.x / v.w * 0.5f + 0.5f) * Width;
	float y = (v.y / v.w * 0.5f + 0.5f) * Height;
	float z = v.z / v.w;
	return vec3(x, y, z);
}

//Q1
/*
void render() {
	if (vertices.empty()) create_scene();

	mat4 model = scale(translate(mat4(1), vec3(0, 0, -7)), vec3(2));
	mat4 view = lookAt(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0));
	mat4 projection = frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.f);

	depthBuffer.assign(Width * Height, std::numeric_limits<float>::infinity());
	OutputImage.assign(Width * Height * 3, 0.0f);

	for (int i = 0; i < gNumTriangles; ++i) {
		vec4 clipCoords[3];
		vec3 screenCoords[3];
		vec4 worldCoords[3];

		for (int j = 0; j < 3; ++j) {
			vec4 local = vertices[gIndexBuffer[i * 3 + j]];
			vec4 w = model * local;
			vec4 v = view * w;
			vec4 c = projection * v;
			clipCoords[j] = c;
			screenCoords[j] = viewport_transform(c);
			worldCoords[j] = w;
		}

		// 삼각형 중심 계산 (world space)
		vec3 p0 = vec3(worldCoords[0]);
		vec3 p1 = vec3(worldCoords[1]);
		vec3 p2 = vec3(worldCoords[2]);
		vec3 centroid = (p0 + p1 + p2) / 3.0f;

		// normal 벡터 계산
		vec3 normal = normalize(cross(p1 - p0, p2 - p0));

		// 광원 정보
		vec3 light_pos = vec3(-4, 4, -3);
		vec3 light_dir = normalize(light_pos - centroid);
		vec3 view_dir = normalize(-centroid);  // camera at origin
		vec3 reflect_dir = reflect(-light_dir, normal);

		// 조명 파라미터
		vec3 ka(0, 1, 0);
		vec3 kd(0, 0.5, 0);
		vec3 ks(0.5, 0.5, 0.5);
		float Ia = 0.2f;
		float diff = std::max(dot(normal, light_dir), 0.0f);
		float spec = pow(std::max(dot(view_dir, reflect_dir), 0.0f), 32.0f);
		vec3 color = ka * Ia + kd * diff + ks * spec;

		// 감마 보정
		color = pow(color, vec3(1.0f / 2.2f));

		// bounding box 설정
		int minX = glm::clamp(int(floor(min(screenCoords[0].x, min(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int maxX = glm::clamp(int(ceil(max(screenCoords[0].x, max(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int minY = glm::clamp(int(floor(min(screenCoords[0].y, min(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);
		int maxY = glm::clamp(int(ceil(max(screenCoords[0].y, max(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);

		for (int y = minY; y <= maxY; ++y) {
			for (int x = minX; x <= maxX; ++x) {
				vec3 p = vec3(x + 0.5f, y + 0.5f, 0);

				vec3 v0 = screenCoords[1] - screenCoords[0];
				vec3 v1 = screenCoords[2] - screenCoords[0];
				vec3 v2 = p - screenCoords[0];

				float d00 = dot(v0, v0), d01 = dot(v0, v1), d11 = dot(v1, v1);
				float d20 = dot(v2, v0), d21 = dot(v2, v1);
				float denom = d00 * d11 - d01 * d01;

				float v = (d11 * d20 - d01 * d21) / denom;
				float w = (d00 * d21 - d01 * d20) / denom;
				float u = 1.0f - v - w;

				if (u >= 0 && v >= 0 && w >= 0) {
					float depth = u * screenCoords[0].z + v * screenCoords[1].z + w * screenCoords[2].z;
					int idx = y * Width + x;
					if (depth < depthBuffer[idx]) {
						depthBuffer[idx] = depth;
						OutputImage[3 * idx + 0] = color.r;
						OutputImage[3 * idx + 1] = color.g;
						OutputImage[3 * idx + 2] = color.b;
					}
				}
			}
		}
	}
}
*/
//Q2
/*
void render() {
	if (vertices.empty()) create_scene();

	mat4 model = scale(translate(mat4(1), vec3(0, 0, -7)), vec3(2));
	mat4 view = lookAt(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0));
	mat4 projection = frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.f);

	depthBuffer.assign(Width * Height, std::numeric_limits<float>::infinity());
	OutputImage.assign(Width * Height * 3, 0.0f);

	for (int i = 0; i < gNumTriangles; ++i) {
		vec4 clipCoords[3];
		vec3 screenCoords[3];
		vec3 worldPos[3];
		vec3 colors[3];

		for (int j = 0; j < 3; ++j) {
			vec4 local = vertices[gIndexBuffer[i * 3 + j]];
			vec4 w = model * local;
			vec4 v = view * w;
			vec4 c = projection * v;

			clipCoords[j] = c;
			screenCoords[j] = viewport_transform(c);
			worldPos[j] = vec3(w);

			// 정점 normal = 로컬 정점 좌표 → 모델 행렬로 변환
			vec3 local_normal = normalize(vec3(local)); // 단위 구 기준
			vec3 world_normal = normalize(vec3(model * vec4(local_normal, 0.0f)));

			// 조명 계산
			vec3 light_pos = vec3(-4, 4, -3);
			vec3 light_dir = normalize(light_pos - worldPos[j]);
			vec3 view_dir = normalize(-worldPos[j]); // camera at origin
			vec3 reflect_dir = reflect(-light_dir, world_normal);

			vec3 ka(0, 1, 0);
			vec3 kd(0, 0.5, 0);
			vec3 ks(0.5, 0.5, 0.5);
			float Ia = 0.2f;

			float diff = std::max(dot(world_normal, light_dir), 0.0f);
			float spec = pow(std::max(dot(view_dir, reflect_dir), 0.0f), 32.0f);

			vec3 color = ka * Ia + kd * diff + ks * spec;
			color = pow(color, vec3(1.0f / 2.2f)); // 감마 보정
			colors[j] = color;
		}

		// bounding box
		int minX = glm::clamp(int(floor(min(screenCoords[0].x, min(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int maxX = glm::clamp(int(ceil(max(screenCoords[0].x, max(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int minY = glm::clamp(int(floor(min(screenCoords[0].y, min(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);
		int maxY = glm::clamp(int(ceil(max(screenCoords[0].y, max(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);

		for (int y = minY; y <= maxY; ++y) {
			for (int x = minX; x <= maxX; ++x) {
				vec3 p = vec3(x + 0.5f, y + 0.5f, 0);
				vec3 v0 = screenCoords[1] - screenCoords[0];
				vec3 v1 = screenCoords[2] - screenCoords[0];
				vec3 v2 = p - screenCoords[0];

				float d00 = dot(v0, v0), d01 = dot(v0, v1), d11 = dot(v1, v1);
				float d20 = dot(v2, v0), d21 = dot(v2, v1);
				float denom = d00 * d11 - d01 * d01;

				float v = (d11 * d20 - d01 * d21) / denom;
				float w = (d00 * d21 - d01 * d20) / denom;
				float u = 1.0f - v - w;

				if (u >= 0 && v >= 0 && w >= 0) {
					float depth = u * screenCoords[0].z + v * screenCoords[1].z + w * screenCoords[2].z;
					int idx = y * Width + x;
					if (depth < depthBuffer[idx]) {
						depthBuffer[idx] = depth;
						vec3 color = u * colors[0] + v * colors[1] + w * colors[2];
						OutputImage[3 * idx + 0] = color.r;
						OutputImage[3 * idx + 1] = color.g;
						OutputImage[3 * idx + 2] = color.b;
					}
				}
			}
		}
	}
}
*/
//Q3

void render() {
	if (vertices.empty()) create_scene();

	mat4 model = scale(translate(mat4(1), vec3(0, 0, -7)), vec3(2));
	mat4 view = lookAt(vec3(0, 0, 0), vec3(0, 0, -1), vec3(0, 1, 0));
	mat4 projection = frustum(-0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 1000.f);

	depthBuffer.assign(Width * Height, std::numeric_limits<float>::infinity());
	OutputImage.assign(Width * Height * 3, 0.0f);

	for (int i = 0; i < gNumTriangles; ++i) {
		vec4 clipCoords[3];
		vec3 screenCoords[3];
		vec3 worldPos[3];
		vec3 worldNormal[3];

		for (int j = 0; j < 3; ++j) {
			vec4 local = vertices[gIndexBuffer[i * 3 + j]];
			vec4 w = model * local;
			vec4 v = view * w;
			vec4 c = projection * v;

			clipCoords[j] = c;
			screenCoords[j] = viewport_transform(c);
			worldPos[j] = vec3(w);

			// 정점 normal 계산: 단위 구 기준
			vec3 local_normal = normalize(vec3(local));
			worldNormal[j] = normalize(vec3(model * vec4(local_normal, 0.0f)));
		}

		// bounding box
		int minX = glm::clamp(int(floor(min(screenCoords[0].x, min(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int maxX = glm::clamp(int(ceil(max(screenCoords[0].x, max(screenCoords[1].x, screenCoords[2].x)))), 0, Width - 1);
		int minY = glm::clamp(int(floor(min(screenCoords[0].y, min(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);
		int maxY = glm::clamp(int(ceil(max(screenCoords[0].y, max(screenCoords[1].y, screenCoords[2].y)))), 0, Height - 1);

		for (int y = minY; y <= maxY; ++y) {
			for (int x = minX; x <= maxX; ++x) {
				vec3 p = vec3(x + 0.5f, y + 0.5f, 0);
				vec3 v0 = screenCoords[1] - screenCoords[0];
				vec3 v1 = screenCoords[2] - screenCoords[0];
				vec3 v2 = p - screenCoords[0];

				float d00 = dot(v0, v0), d01 = dot(v0, v1), d11 = dot(v1, v1);
				float d20 = dot(v2, v0), d21 = dot(v2, v1);
				float denom = d00 * d11 - d01 * d01;

				float v = (d11 * d20 - d01 * d21) / denom;
				float w = (d00 * d21 - d01 * d20) / denom;
				float u = 1.0f - v - w;

				if (u >= 0 && v >= 0 && w >= 0) {
					float depth = u * screenCoords[0].z + v * screenCoords[1].z + w * screenCoords[2].z;
					int idx = y * Width + x;
					if (depth < depthBuffer[idx]) {
						depthBuffer[idx] = depth;

						// 위치와 normal 보간
						vec3 pos = u * worldPos[0] + v * worldPos[1] + w * worldPos[2];
						vec3 normal = normalize(u * worldNormal[0] + v * worldNormal[1] + w * worldNormal[2]);

						// 조명 계산
						vec3 light_pos = vec3(-4, 4, -3);
						vec3 light_dir = normalize(light_pos - pos);
						vec3 view_dir = normalize(-pos);
						vec3 reflect_dir = reflect(-light_dir, normal);

						vec3 ka(0, 1, 0);
						vec3 kd(0, 0.5, 0);
						vec3 ks(0.5, 0.5, 0.5);
						float Ia = 0.2f;

						float diff = std::max(dot(normal, light_dir), 0.0f);
						float spec = pow(std::max(dot(view_dir, reflect_dir), 0.0f), 32.0f);

						vec3 color = ka * Ia + kd * diff + ks * spec;

						// 감마 보정
						color = pow(color, vec3(1.0f / 2.2f));

						OutputImage[3 * idx + 0] = color.r;
						OutputImage[3 * idx + 1] = color.g;
						OutputImage[3 * idx + 2] = color.b;
					}
				}
			}
		}
	}
}




void resize_callback(GLFWwindow*, int nw, int nh)
{
	Width = nw;
	Height = nh;

	glViewport(0, 0, nw, nh);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, static_cast<double>(Width), 0.0, static_cast<double>(Height), 1.0, -1.0);

	OutputImage.resize(Width * Height * 3, 0.5f);
	depthBuffer.resize(Width * Height, std::numeric_limits<float>::infinity());

	render();
}



int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
