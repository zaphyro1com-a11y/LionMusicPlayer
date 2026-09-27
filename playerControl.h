#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

#include <atomic>
#include <mutex>
#include <string>

// Estado compartido entre el hilo que reproduce música y el hilo que
// atiende la interfaz (por ahora comandos de texto; después ncurses).
//
// Reglas de uso:
//  - Los flags (paused/skipRequested/quitRequested) y los contadores
//    (elapsedSeconds/totalSeconds) son std::atomic: se pueden leer/escribir
//    desde cualquier hilo sin necesidad de mutex.
//  - El título de la canción es un std::string normal, así que sí necesita
//    un mutex propio (titleMutex) para no leerlo/escribirlo a la vez desde
//    los dos hilos.
//  - El volumen es un std::atomic<float> como los contadores, pero se expone
//    solo a través de getVolume()/setVolume() (en vez de como campo público)
//    para que setVolume() pueda recortarlo siempre a [0.0, 1.0] sin importar
//    quién lo llame: LMPlayer (al reproducir), InputManager (al ajustarlo
//    con +/-) o la UI (si en algún momento necesita fijarlo, no solo leerlo).
struct PlayerControl {
    std::atomic<bool> paused{false};
    std::atomic<bool> skipRequested{false};
    std::atomic<bool> quitRequested{false};

    std::atomic<int> elapsedSeconds{0};
    std::atomic<int> elapsedMillis{0};   // igual que elapsedSeconds pero con precisión de milisegundos
    std::atomic<int> totalSeconds{0}; // 0 = duración desconocida

    // Llamado por el hilo de reproducción cada vez que arranca una canción nueva
    void setCurrentSong(const std::string& name, int total) {
        std::lock_guard<std::mutex> lock(titleMutex);
        currentTitle = name;
        totalSeconds = total;
        elapsedSeconds = 0;
        elapsedMillis = 0;
    }

    // Llamado por el hilo de interfaz para mostrar qué está sonando
    std::string getCurrentSong() {
        std::lock_guard<std::mutex> lock(titleMutex);
        return currentTitle;
    }

    // Devuelve el volumen actual (0.0 - 1.0). Thread-safe: volume es atomic, no hace falta mutex.
    float getVolume() const {
        return volume.load();
    }

    // Fija el volumen, recortandolo siempre a [0.0, 1.0]. Centralizar el recorte acá evita que
    // cada llamador (LMPlayer, la UI, etc.) tenga que acordarse de hacerlo por su cuenta.
    void setVolume(float value) {
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        volume = value;
    }

    // Deja todo listo para arrancar una sesión de reproducción nueva desde cero
    void reset() {
        paused = false;
        skipRequested = false;
        quitRequested = false;
        elapsedSeconds = 0;
        elapsedMillis = 0;
    }

private:
    std::mutex titleMutex;
    std::string currentTitle;
    std::atomic<float> volume{1.0f}; // Volumen actual del reproductor, en el rango [0.0, 1.0]
};

#endif // PLAYER_CONTROL_H