#ifndef LMPLAYER_H
#define LMPLAYER_H

    //* Lion MusicPlayer (LMP) Esta libreria se encarga de organizar a los objetos entregados pot el resto de librerias
    //* y unificar sus funciones en un solo objeto que permita reproducir canciones, controlar su reproduccion, leer playlists,
    //* mostrar animaciones, gestionar subventanas, buscar letras dentro del directorio, etc.
    //* Este objeto es capaz de iniciar un reproductor de musica con sus funciones más simples simplemente usando los recursos
    //* del resto de librerias y APIs (Miniaudio).
    //* Apesar de que usa los recursos de las demas librerias, esta tambien se encarga de lo mas simple
    //* buscar carnciones, listarlas y reproducirlas.

#include "playerControl.h"          // Control del Reproductor
#include "UI.h"                     // Gestor de SubVentanas
#include "playlistManager.h"        // Gestor de Playlists
#include "animationManager.h"       // Gestor de Animaciones
#include "LrcReader.h"              // Lector de archivos .lrc
#include "miniaudio/miniaudio.h"    // ma_engine / ma_device (solo declaraciones aca; la implementacion se compila una unica vez en LMPlayer.cpp)

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>
#include <atomic>

    //! CODIGO EN REFACTORIZACION !//
    //ToDo: Crear la clase LMP
    //ToDo: Refactorizar Funciones
    //ToDo: Agregar más funciones
    //ToDo: La clase debe ser capaz de gestionar todo el reproductor 

//* Estructura del archivo userdat.bin *
typedef struct{

    std::string MainPath;                   // Dirrecion de la carpeta de Musica y letras
    int MainColor;                          // Color Principal (Resaltado) de la UI
    int ComplementaryColor;                 // Color Secundario de la UI

} UserDat;

class LionMusicPlayer{

public:
    //* Enum, Maquina de estados para controlar el flujo de escenarios/menus
    //! Se declara aca arriba (antes de los atributos y metodos privados) porque InputManager()
    //! y MusicPlayerMainLoop() ahora reciben un MenuState como parametro: dentro de una clase,
    //! un tipo anidado solo puede usarse en la FIRMA de un metodo si ya fue declarado mas arriba
    //! en el cuerpo de la clase (a diferencia de los CUERPOS de los metodos, que si pueden
    //! referenciar miembros declarados mas abajo).
    enum class MenuState {
        Main,
        Playlists,
        Player,
        PlaylistsPlayer,
        Library,
        MainOptions,
        PlaylistOptions,
        Exit,
        Error   // estado para errores
    };

private:
    //~ Atributos ~//
    std::string MainPath;                   // Dirrecion de la carpeta de Musica y letras
    std::vector<std::string> MusicLibraryPath;
    std::vector<std::string> MusicLibrary;  // Lista de canciones encontradas (incluye extencion pero no el path completo)
    std::vector<std::string> PlaylistQueue; // Lista de canciones que se esta reproduciendo (incluye extencion pero no el path completo)
    int MusicLibrarySize= 0;                // Total de Canciones Encontradas
    int QueueSize= 0;                       // Total de canciones en la cola de reproduccion
    int CurrentQueueIndex= 0;               // Indice de la cancion actual dentro de PlaylistQueue (solo lo usa el hilo de reproduccion)
    std::string LastSyncedSong;             // Ultima cancion sincronizada con los agentes (evita releer la letra en cada vuelta del loop)
    UserDat USER_DATA;                      // Estructura del userdat.bin
    bool UserDataLoaded= false;             // true si USER_DATA ya se cargo desde el disco en esta ejecucion
    double LibraryDuration;                 // Duracion de toda la libreria en segundos
    std::string LibraryDurationStr;         //  Duracion de toda la libreria en formato

    //~ OJBETOS (Atributos) ~//
    PlayerControl LMP_Controller;           // Controlador de Reproduccion
    UI LMP_UI;                              // Gestor de subVentanas
    PlaylistManager LMP_PlaylistsManager;   // Gestor de Playlists
    LrcReader LMP_LyricsReader;             // Gestor de Letras 
    AnimationManager LMP_AnimationManager;  // Gestor de animaciones

    //~ Miniaudio (Atributos) ~//
    // Antes ma_device vivia como variable local y ma_engine como 'static' local de una funcion
    // libre. Ahora son atributos de la clase para que InputManager() pueda ajustar el volumen
    // del audio que este sonando en cada momento sin tener que pasarlo como parametro extra
    // por todas las funciones de reproduccion.
    ma_engine LMP_Engine;                      // Motor de miniaudio: reproduce mp3/wav/flac de forma nativa. Se crea una unica vez (ver LMP_EngineReady) y se reutiliza durante toda la vida del reproductor.
    std::atomic<bool> LMP_EngineReady{false};  // true una vez que LMP_Engine ya fue inicializado con exito
    ma_device LMP_Device;                      // Dispositivo de audio crudo: usado solo mientras se reproduce un .m4a (FFmpeg + ring buffer). Se re-inicializa en cada cancion .m4a.
    std::atomic<bool> LMP_DeviceActive{false}; // true mientras LMP_Device esta listo para recibir comandos (entre ma_device_start y ma_device_uninit de la cancion .m4a actual)

    //~ Metodos Privados ~//
    // Convierte wchar_t* a std::string (UTF-8)
    std::string toString(const wchar_t* wstr);
    // Crea el archivo donde se guardan los datos del usuario
    int SET_DATA();
    // Lee user_data.bin UNA sola vez (cachea en UserDataLoaded) y llena USER_DATA + MainPath.
    bool LoadUserData();
    // Lee el path desde el archivo userdat
    int READ_PATH();
    // Busca todas las canciones en el directorio
    int RefreshLibrary();
    // Obtiene el path de la carpeta de musica desde el userdat o desde el usuario si no existe el archivo
    std::string GetLibraryPath();
    // Obtiene el int del color Principal desde del userdat o desde el usuario si no existe el archivo
    int GetMainColor();
    // Obtiene el int del color Secundario dede el userdat o desde el usuario si no existe el archivo
    int GetSecondaryColor();
    // Obtiene los inputs del usuario y modifica el UserOption de la clase UI vertical u horizontalmente 
    // currentState indica el menu/pantalla actual: si es Player o PlaylistsPlayer, ademas de la
    // navegacion habitual, las teclas '+' y '-' suben/bajan el volumen en pasos de 0.1 (ver AdjustVolume)
    int InputManager(char direction, MenuState currentState = MenuState::Main);
    // Sube o baja Volume en 'delta' (recortado a [0.0, 1.0]) y lo aplica en caliente al backend
    // de audio que este sonando en este momento (LMP_Engine o LMP_Device, el que este activo)
    void AdjustVolume(float delta);
    // Reproduce la siguiente cancion de la cola de reproduccion
    int PlayNextSong();
    // Reproduce un archivo segun su extension
    void PlaySong(const std::string& ruta);
    // Reproduce un .m4a decodificandolo con AudioDecoder (FFmpeg) y alimentando LMP_Device
    void ReproducirM4A(const std::string& ruta);
    // Reproduce mp3/wav/flac usando LMP_Engine (miniaudio nativo)
    void ReproducirGenerico(const std::string& ruta);
    // Comparte la informacion de la cancion actual con los agentes de la clase (LrcReader, AnimationManager)
    int SyncManagers();
    // mantiene viva la interfaz, sincroniza los módulos y responde a las acciones del usuario hasta que se solicita salir.
    // currentState se le reenvia tal cual a InputManager para habilitar (o no) el control de volumen
    void MusicPlayerMainLoop(MenuState currentState);
    // Bucle que corre en un hilo aparte: va reproduciendo PlaylistQueue 
    void PlaybackThreadLoop();
    // Devuelve la duración total en segundos
    double GetTotalDuration(); 


public:
    //° Constructor °
    // Instancia el objeto y lee/crea el archivo en donde se aloja el MainPath.
    // Implementado en LMPlayer.cpp: el orden real de construccion de los miembros lo define
    // el orden de declaracion de arriba (Controller, UI, Playlists, Lyrics, Animation), no el
    // orden de la lista de inicializacion, asi que la resolucion de colores/path se hace en el
    // cuerpo del constructor, una vez que todos los miembros ya existen.
    LionMusicPlayer();

    //° Destructor °
    // Libera los recursos de la clase y cierra la UI
    ~LionMusicPlayer();

    //~ Propiedades ~//
    
    //~ Metodos~//
    // Pantalla de arranque
    int BootScreen();
    // Transicion entre menus
    int MenuTransition();
    // Pantalla de Power-Off
    int ExitScreen();
    // Borra los datos del usuario y termina el programa
    int ForgetUser();
    // Menu Principal
    MenuState Menu();
    // Menu Principal para la seleccion de playlists
    MenuState PlaylistsMenu();
    // Pide el nombre completo de una cancion y la reproduce abriendo la UI
    MenuState SearchAndPlay();
    // Genera una QUEUE aleatoria con todo el directorio y la reproduce abriendo la UI
    MenuState ShuffleAndPlay();
    // Genera una QUEUE aleatoria con el directorio de la playlist seleccionada y la reproduce abirendo la UI
    MenuState PlayByMood();

};

 //-- OTROS --
std::string SecondsToHMS(double seconds); 

#endif //LMPLAYER_H