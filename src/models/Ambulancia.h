#ifndef AMBULANCIA_H
#define AMBULANCIA_H

#include "Veiculo.h"

#ifdef _WIN32
#include <windows.h>
DWORD WINAPI thread_ambulancia(LPVOID arg);
#else
void *thread_ambulancia(void *arg);
#endif

#endif /* AMBULANCIA_H */
