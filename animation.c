#include "animation.h"

#include <time.h>
#include <math.h>

#include <GLFW/glfw3.h>

static void i_updateSmoother(PRWsmoother* smoother)
{
    smoother->p_previousTime = smoother->p_currentTime;
	smoother->p_currentTime = glfwGetTime();
	if (smoother->p_previousTime == 0) smoother->p_previousTime = smoother->p_currentTime;
	double delta = smoother->p_currentTime - smoother->p_previousTime;

    if(0.2 < delta) delta = 0.2;

	if (smoother->grabbed)
	{
		if (smoother->p_acceleration)
		{
			smoother->p_acceleration = (smoother->grabbingTo - smoother->p_value) * fabs(smoother->speed) * 32.0;
			smoother->velocity += smoother->p_acceleration * delta;
			smoother->velocity *= pow(0.0025 / smoother->speed, delta);
		}
		else smoother->velocity = (smoother->grabbingTo - smoother->p_value) * fabs(smoother->speed);
	}
	smoother->p_value += smoother->velocity * delta;
	smoother->velocity *= pow(0.0625 / (smoother->speed * smoother->friction), delta);
}

void prwaInitSmoother(PRWsmoother* smoother)
{
    smoother->p_acceleration = 0;
    smoother->velocity = 0;
    smoother->p_previousTime = 0;
    smoother->p_currentTime = 0;
    smoother->grabbed = 1;
    smoother->grabbingTo = 0;
    smoother->p_value = 0;
    smoother->springy = 0;
    smoother->speed = 10;
    smoother->friction = 1;
}

void prwaSmootherStartGrabbing(PRWsmoother* smoother)
{
    smoother->grabbed = 1;
}

void prwaSmootherGrabTo(PRWsmoother* smoother, double grabTo)
{
    smoother->grabbed = 1;
    smoother->grabbingTo = grabTo;
}

void prwaSmootherSetAndGrab(PRWsmoother* smoother, double value)
{
    smoother->grabbed = 1;
    smoother->grabbingTo = value;
    smoother->p_value = value;
}

void prwaSmootherRelease(PRWsmoother* smoother)
{
    smoother->grabbed = 0;
}

int prwaSmoohterIsGrabbed(PRWsmoother* smoother)
{
    return smoother->grabbed;
}

int prwaSmootherIsSpringy(PRWsmoother* smoother)
{
    return smoother->springy;
}

void prwaSmootherSetSpringy(PRWsmoother* smoother, int springy)
{
    smoother->springy = springy;
}

double prwaSmootherSpeed(PRWsmoother* smoother)
{
    return smoother->speed;
}

void prwaSmootherSetSpeed(PRWsmoother* smoother, double speed)
{
    smoother->speed = speed;
}

double prwaSmootherFriction(PRWsmoother* smoother)
{
    return smoother->friction;
}

double prwaSmootherGrabbingTo(PRWsmoother* smoother)
{
    return smoother->grabbingTo;
}

double prwaSmootherValue(PRWsmoother* smoother)
{
    i_updateSmoother(smoother);
    return smoother->p_value;
}

void prwaSmootherSetValue(PRWsmoother* smoother, double value)
{
    smoother->p_value = value;
}

void prwaInitTimer(PRWtimer* timer)
{
    timer->p_nextTick = glfwGetTime();
    timer->tps = 20.0;
}

int prwaTimerTicksPassed(PRWtimer* timer)
{
    double currentTime = glfwGetTime();
    double tickDelta = 1.0 / timer->tps;

    int i = 0;
    while(timer->p_nextTick < currentTime) 
    {
        i++;
        timer->p_nextTick += tickDelta;
    }

    return i;
}

double prwaTimerPartialTicks(PRWtimer* timer)
{
    double currentTime = glfwGetTime();
    double tickDelta = 1.0 / timer->tps;
    double result = (timer->p_nextTick - currentTime) / tickDelta;
    result = 1 - result;
    return 0 < result ? result : 0;
}

double prwaTimerLerp(PRWtimer* timer, double start, double end)
{
    double partialticks = prwaTimerPartialTicks(timer);
    return start + (end - start) * partialticks;
}