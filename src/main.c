#include <stdio.h>
#include "relogio_global.h"
#include "semaforo_transito.h"

// Inclusões para gerenciar a thread inicial na main (dependente do SO)
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

int main() {
    printf("--- Iniciando o Simulador de Trafico Urbano ---\n");
    
    // 1. Inicializa o relógio global
    init_relogio();
    
    // 2. Inicializa um semáforo de teste
    SemaforoTransito sem1;
    // Semáforo ID 1 | 3 ticks no VERDE | 3 ticks no VERMELHO
    init_semaforo(&sem1, 1, 3, 3); 
    
    // 3. Cria e inicia a thread do relógio
    os_thread_t thread_id;
    
#ifdef _WIN32
    thread_id = CreateThread(NULL, 0, thread_relogio, NULL, 0, NULL);
#else
    pthread_create(&thread_id, NULL, thread_relogio, NULL);
#endif

    // Loop principal da simulação para fins de teste
    // Vamos simular por 10 ticks e depois encerrar
    for (int i = 1; i <= 10; i++) {
        // Pausa a thread principal até o relógio "bater" o próximo tick
        esperar_proximo_tick(global_tick);
        
        // Assim que o tick vira, atualizamos o semáforo
        atualizar_semaforo(&sem1);
        
        // Imprime o estado atual
        printf("[Tick: %02d] Semaforo %d esta: %s\n", 
            global_tick, 
            sem1.id, 
            sem1.estado == VERDE ? "VERDE" : "VERMELHO");
    }
    
    printf("\nEncerrando a simulacao...\n");
    
    // 4. Sinaliza para a thread do relógio parar
    simulation_running = false;
    
    // 5. Aguarda a thread do relógio finalizar com segurança
#ifdef _WIN32
    WaitForSingleObject(thread_id, INFINITE);
    CloseHandle(thread_id);
#else
    pthread_join(thread_id, NULL);
#endif
    
    // 6. Limpa os recursos da memória
    destroy_semaforo(&sem1);
    destroy_relogio();
    
    printf("Limpeza concluida. Saindo do simulador.\n");
    return 0;
}
