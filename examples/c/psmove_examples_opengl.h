#pragma once

#include <GLFW/glfw3.h>
#include <functional>

#ifdef __APPLE__
    #include <OpenGL/gl.h>
    #include <GLUT/glut.h>
#elif _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>

	#include <GL/gl.h>
	#include <GL/glu.h>
#else
    #include <GL/gl.h>
    #include <GL/glut.h>
#endif

static GLUquadricObj *g_quadObj;

void initGLUQuadricObject(void)
{
	g_quadObj = gluNewQuadric();
	if (!g_quadObj) {
            fprintf(stderr, "out of mempry");
            exit(1);
        }
}
#define QUADRIC_OBJECT_INIT() { if(!g_quadObj) initGLUQuadricObject(); }

void drawBox(float size, GLenum type)
{
	static float n[6][3] =
	{
		{ -1.0, 0.0, 0.0 },
		{ 0.0, 1.0, 0.0 },
		{ 1.0, 0.0, 0.0 },
		{ 0.0, -1.0, 0.0 },
		{ 0.0, 0.0, 1.0 },
		{ 0.0, 0.0, -1.0 }
	};
	static int faces[6][4] =
	{
		{ 0, 1, 2, 3 },
		{ 3, 2, 6, 7 },
		{ 7, 6, 5, 4 },
		{ 4, 5, 1, 0 },
		{ 5, 6, 2, 1 },
		{ 7, 4, 0, 3 }
	};
	float v[8][3];
	int i;

	v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
	v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
	v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
	v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
	v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
	v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

	for (i = 5; i >= 0; i--)
	{
		glBegin(type);
		glNormal3fv(&n[i][0]);
		glVertex3fv(&v[faces[i][0]][0]);
		glVertex3fv(&v[faces[i][1]][0]);
		glVertex3fv(&v[faces[i][2]][0]);
		glVertex3fv(&v[faces[i][3]][0]);
		glEnd();
	}
}

void drawWireCube(float size)
{
	drawBox(size, GL_LINE_LOOP);
}

void drawSolidCube(float size)
{
	drawBox(size, GL_QUADS);
}

void drawWireSphere(float radius, int slices, int stacks)
{
	QUADRIC_OBJECT_INIT();
	gluQuadricDrawStyle(g_quadObj, GLU_LINE);
	gluQuadricNormals(g_quadObj, GLU_SMOOTH);
	gluSphere(g_quadObj, radius, slices, stacks);
}

void drawSolidSphere(float radius, int slices, int stacks)
{
	QUADRIC_OBJECT_INIT();
	gluQuadricDrawStyle(g_quadObj, GLU_FILL);
	gluQuadricNormals(g_quadObj, GLU_SMOOTH);
	gluSphere(g_quadObj, radius, slices, stacks);
}

class GLFWRenderer {
public:
    GLFWRenderer();
    ~GLFWRenderer();

    void mainloop(std::function<void()> func);

private:
    GLFWwindow *m_window;
};

GLFWRenderer::GLFWRenderer()
{
    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize glfw\n");
        exit(1);
    }

    m_window = glfwCreateWindow(640, 480, "PS Move API -- OpenGL", NULL, NULL);
    if (m_window == NULL)
    {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(m_window);
}

GLFWRenderer::~GLFWRenderer()
{
    glfwTerminate();
}

void
GLFWRenderer::mainloop(std::function<void()> func)
{
    while (!glfwWindowShouldClose(m_window)) {
        func();

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

