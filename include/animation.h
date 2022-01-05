#pragma once

typedef struct PRWsmoother
{
    double p_acceleration;
    double velocity;
    double p_previousTime;
    double p_currentTime;
    char grabbed;
    double grabbingTo;
    double p_value;
    char springy;
    double speed;
    double friction;
} PRWsmoother;

typedef struct PRWtimer
{
    double p_nextTick;
    double tps;
} PRWtimer;

void prwaInitSmoother(PRWsmoother* smoother);

void prwaSmootherStartGrabbing(PRWsmoother* smoother);

void prwaSmootherGrabTo(PRWsmoother* smoother, double grabTo);

void prwaSmootherSetAndGrab(PRWsmoother* smoother, double value);

void prwaSmootherRelease(PRWsmoother* smoother);

int prwaSmoohterIsGrabbed(PRWsmoother* smoother);

int prwaSmootherIsSpringy(PRWsmoother* smoother);

void prwaSmootherSetSpringy(PRWsmoother* smoother, int springy);

double prwaSmootherSpeed(PRWsmoother* smoother);

void prwaSmootherSetSpeed(PRWsmoother* smoother, double speed);

double prwaSmootherFriction(PRWsmoother* smoother);

double prwaSmootherGrabbingTo(PRWsmoother* smoother);

double prwaSmootherValue(PRWsmoother* smoother);

void prwaSmootherSetValue(PRWsmoother* smoother, double value);

void prwaInitTimer(PRWtimer* timer);

int prwaTimerTicksPassed(PRWtimer* timer);

double prwaTimerPartialTicks(PRWtimer* timer);

double prwaTimerLerp(PRWtimer* timer, double start, double end);