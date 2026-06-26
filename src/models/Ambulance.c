#include "Ambulance.h"

void *ambulance_thread(void *arg)
{
    Vehicle *vehicle = (Vehicle *)arg;

    if (vehicle)
        vehicle_set_ambulance(vehicle, 1);

    return vehicle_thread(arg);
}
