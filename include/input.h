#include <GLFW/glfw3.h>

void prwiRegisterWindow(GLFWwindow* window);

void prwiSetActiveWindow(GLFWwindow* window);

void prwiPollInputs();

int prwiKeyJustPressed(int key);

int prwiIsKeyPressed(int key);

int prwiKeyJustReleased(int key);

int prwiIsKeyRepeating(int key);

int prwiIsMButtonPressed(int button);

int prwiMButtonJustPressed(int button);

int prwiMButtonJustReleased(int button);

float prwiCursorX();

float prwiCursorY();

float prwiCursorXDelta();

float prwiCursorYDelta();