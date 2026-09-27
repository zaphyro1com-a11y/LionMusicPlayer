#include "LMPlayer.h"

#include "audioDecoder.h"
#include "UI.h"
#include "LrcReader.h"
#include "playlistManager.h"
#include "playerControl.h"
#include "animationManager.h"

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
}

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include "ncursesw/ncurses.h"

using namespace std;

//! Importante !
//° La clase UI solo se encarga de dibujar la interfaz, no de actualizarla, por lo que cada funcion Draw
//° deber ser llamada dentro de un bucle controlado por el hilo de reproduccion.

//* Constructor *
//? LMP_UI se construye con colores de relleno (1 y 2) y los managers con un path
//? vacio, y recien en el cuerpo del constructor -donde todos los miembros ya existen- se
//? resuelven los colores y el path reales.
LionMusicPlayer::LionMusicPlayer() : LMP_UI(1, 2, LMP_LyricsReader, LMP_PlaylistsManager, LMP_AnimationManager, LMP_Controller),
                        LMP_PlaylistsManager(""), LMP_LyricsReader(), LMP_AnimationManager() {

    // LMP_UI ya llamo a initscr()/cbreak() en su constructor (linea de arriba). A partir de
    // ahi la terminal queda en modo raw y cualquier tecla que haya quedado pendiente antes
    // de este punto -tipicamente el mismo Enter que el usuario uso para correr el ejecutable
    // desde la shell- se acumula en el buffer de entrada. Si nada llama a wgetch() hasta el
    // primer InputManager() del loop principal (por ejemplo cuando BootScreen()/Menu() no se
    // llaman, como en un ShuffleAndPlay() directo), ese Enter residual termina siendo la
    // "primera tecla" que lee el programa: se interpreta como "confirmar opcion" con
    // UserOption en su valor por defecto (0 = Salir del Modo) y el programa se cierra solo,
    // sin ningun error visible, a los pocos segundos de arrancar. flushinp() descarta ese
    // input residual antes de que se lea nada.
    flushinp();

    // Colores: si ya existe user_data.bin los lee de ahi, si no los pregunta por UI
    int mainColor = GetMainColor();
    int secondaryColor = GetSecondaryColor();
    LMP_UI.SetColors(mainColor, secondaryColor);

    //~ Leer datos del usuario ~//
    // si el lector falla volver a crear el archivo
    if(READ_PATH() != 0){
        // Crea archivo y pide datos, los datos tambien se guardan en la clase 
        if(SET_DATA() != 0){
            cerr << "Error al crear el archivo de datos del usuario." << endl;
            return;
        }
    }

    // Cambiar las rutas de los objetos que lo requieran
    LMP_PlaylistsManager.SetDirectoryPath(MainPath);
    if( LMP_LyricsReader.SetDirPath(MainPath) != 0 ){
        cerr << "Error al establecer la ruta del lector de letras." << endl;
        return;
    }

    // Refrescar la lista de canciones
    RefreshLibrary();

}

//* Destructor *
LionMusicPlayer::~LionMusicPlayer(){

    // Cierra todas las ventanas abiertas
    LMP_UI.CloseAllWindows();

}

//~ Metodos Privados ~

string LionMusicPlayer::toString(const wchar_t* wstr){

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size_needed, nullptr, nullptr);
    if (!str.empty() && str.back() == '\0') str.pop_back();
    return str;

}

int LionMusicPlayer::SET_DATA() {

    // Solicitar directorio
    std::string _mainPath = GetLibraryPath();

    // Guardar datos
    USER_DATA.MainPath = _mainPath;
    MainPath = _mainPath;

    // Abrir archivo
    std::ofstream file("user_data.bin", std::ios::binary);

    if (!file.is_open()) {
        clear();
        printw("\nError! No se pudo abrir el archivo de datos del usuario.");
        refresh();
        napms(1500);

        return -1;
    }

    // -----------------------------
    // Guardar MainPath
    // -----------------------------

    std::size_t pathSize = USER_DATA.MainPath.size();

    file.write(
        reinterpret_cast<const char*>(&pathSize),
        sizeof(pathSize)
    );

    file.write(
        USER_DATA.MainPath.data(),
        pathSize
    );

    // -----------------------------
    // Guardar MainColor
    // -----------------------------

    file.write(
        reinterpret_cast<const char*>(&USER_DATA.MainColor),
        sizeof(USER_DATA.MainColor)
    );

    // -----------------------------
    // Guardar ComplementaryColor
    // -----------------------------

    file.write(
        reinterpret_cast<const char*>(&USER_DATA.ComplementaryColor),
        sizeof(USER_DATA.ComplementaryColor)
    );

    file.close();

    return 0;
}

int LionMusicPlayer::GetMainColor(){

    // LoadUserData ya sabe leer el formato real del archivo (length-prefixed) y cachea el
    // resultado; antes esta funcion hacia su propio file.read(&USER_DATA, sizeof(USER_DATA))
    // que volcaba bytes crudos sobre un std::string -> puntero interno corrupto -> segfault
    // en cuanto USER_DATA.MainPath se tocaba (por ejemplo, al destruirse el objeto).
    if ( LoadUserData() ) {
        return USER_DATA.MainColor;
    } 

    // De lo contrario, solicitar un color al usuario y guaradrlo en la estructura

    // Color Princiapal
    int color = 0;

    clear();
    mvprintw(1, 1, "Seleccione el color de RESALTADO para el reproductor");
    refresh();
    napms(2000);

    // Reiniciar la seleccion una sola vez antes de entrar al loop 
    LMP_UI.SetUserOption(0);

    do{
        // Evitar que la seleccion se vaya de rango (SelectColor tiene 8 opciones: indices 0-7)
        if (LMP_UI.GetUserOption() < 0) LMP_UI.SetUserOption(0);
        if (LMP_UI.GetUserOption() > 7) LMP_UI.SetUserOption(7);

        color = LMP_UI.SelectColor();
        // Si todavia no se confirmo nada, leer una tecla Seleccion horizontal
        if (color == 0) InputManager('v');

    } while( color == 0 );

    USER_DATA.MainColor= color;

    return color;

}

int LionMusicPlayer::GetSecondaryColor(){

    // Ver comentario en GetMainColor(): LoadUserData reemplaza el file.read crudo que
    // corrompia USER_DATA.MainPath y causaba el segfault al reabrir con user_data.bin existente.
    if ( LoadUserData() ) {
        return USER_DATA.ComplementaryColor;
    } 

    // De lo contrario, solicitar un color al usuario y guaradrlo en la estructura

    // Color Secundario
    int color = 0;

    clear();
    mvprintw(1, 1, "Seleccione el color Secundario para el reproductor");
    refresh();
    napms(2000);

    // Reiniciar la seleccion una sola vez antes de entrar al loop (SelectColor ya no lo hace
    // por si sola en cada llamada, o se perderia la navegacion del usuario)
    LMP_UI.SetUserOption(0);

    do{
        //° Evitar que la seleccion se vaya de rango (SelectColor tiene 8 opciones: indices 0-7)
        if (LMP_UI.GetUserOption() < 0) LMP_UI.SetUserOption(0);
        if (LMP_UI.GetUserOption() > 7) LMP_UI.SetUserOption(7);

        color = LMP_UI.SelectColor();
        // Si todavia no se confirmo nada, leer una tecla (arriba/abajo navega, Enter confirma)
        if (color == 0) InputManager('v');

    } while( color == 0 );

    USER_DATA.ComplementaryColor= color;

    return color;

}

bool LionMusicPlayer::LoadUserData() {

    // Ya se cargo en esta ejecucion: no volver a tocar el disco
    if (UserDataLoaded) return true;

    std::ifstream file("user_data.bin", std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // -----------------------------
    // Leer tamaño de MainPath
    // -----------------------------

    std::size_t pathSize;

    file.read(
        reinterpret_cast<char*>(&pathSize),
        sizeof(pathSize)
    );

    if (!file || pathSize > 4096) {
        file.close();
        return false;
    }

    // -----------------------------
    // Leer MainPath
    // -----------------------------

    USER_DATA.MainPath.resize(pathSize);

    file.read(
        USER_DATA.MainPath.data(),
        pathSize
    );

    if (!file) {
        file.close();
        return false;
    }

    // -----------------------------
    // Leer MainColor
    // -----------------------------

    file.read(
        reinterpret_cast<char*>(&USER_DATA.MainColor),
        sizeof(USER_DATA.MainColor)
    );

    if (!file) {
        file.close();
        return false;
    }

    // -----------------------------
    // Leer ComplementaryColor
    // -----------------------------

    file.read(
        reinterpret_cast<char*>(&USER_DATA.ComplementaryColor),
        sizeof(USER_DATA.ComplementaryColor)
    );

    if (!file) {
        file.close();
        return false;
    }

    file.close();

    // Actualizar MainPath
    MainPath = USER_DATA.MainPath;
    UserDataLoaded = true;

    return true;
}

int LionMusicPlayer::READ_PATH() {

    return LoadUserData() ? 0 : -1;

}

int LionMusicPlayer::RefreshLibrary(){

    // Limpiar la lista de canciones
    MusicLibrary.clear();

    // Listar canciones en el directorio//
    WIN32_FIND_DATAW fileData;
    HANDLE hFind;

    // MP3 //
    hFind = FindFirstFileW((std::wstring(MainPath.begin(), MainPath.end()) + L"\\*.mp3").c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
           // Construir path completo
            wstring fullPath = wstring(MainPath.begin(), MainPath.end()) + L"\\" + fileData.cFileName;

            // Guardar path completo y nombre
            MusicLibraryPath.push_back(toString(fullPath.c_str()));          // ruta completa
            MusicLibrary.push_back(toString(fileData.cFileName));            // solo nombre
        } while (FindNextFileW(hFind, &fileData));

        FindClose(hFind);
    }

    // M4A //
    hFind = FindFirstFileW((std::wstring(MainPath.begin(), MainPath.end()) + L"\\*.m4a").c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            MusicLibrary.push_back(toString(fileData.cFileName));
        } while (FindNextFileW(hFind, &fileData));

        FindClose(hFind);
    }

    // FLAC //
    hFind = FindFirstFileW((std::wstring(MainPath.begin(), MainPath.end()) + L"\\*.flac").c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            MusicLibrary.push_back(toString(fileData.cFileName));
        } while (FindNextFileW(hFind, &fileData));

        FindClose(hFind);
    }

    // WAV //
    hFind = FindFirstFileW((std::wstring(MainPath.begin(), MainPath.end()) + L"\\*.wav").c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            MusicLibrary.push_back(toString(fileData.cFileName));
        } while (FindNextFileW(hFind, &fileData));

        FindClose(hFind);
    }

    // Obtener el tamaño de la biblioteca de música
    MusicLibrarySize= MusicLibrary.size();

    // Obtener la duracion de la libreria
    //LibraryDuration = GetTotalDuration();
    // Convertir la duracion a formato
    //LibraryDurationStr = SecondsToHMS(LibraryDuration);
    // Pasar la duracion a UI 
    //LMP_UI.SetLibraryDuration(LibraryDurationStr);

    //  Pasar el total de canciones a la UI
    LMP_UI.SetTotalSongs(MusicLibrary.size());
    // Pasar el total de playlists a la UI
    LMP_UI.SetTotalPlaylists(LMP_PlaylistsManager.Get_PlaylistsNumber());
    // Pasar el total de animaciones a la UI
    LMP_UI.SetTotalAnimations(LMP_AnimationManager.GetAnimationsList().size()); 

    return (MusicLibrarySize > 0) ? 0 : -1; 

}

string LionMusicPlayer::GetLibraryPath(){

    string _mainPath;
    do{

        //Abrir una ventana con UI y solicitar el directorio
        _mainPath = LMP_UI.GetString("Ubicacion de la Carpeta");

        // Revisar el directorio
        if( !( filesystem::exists(_mainPath) && filesystem::is_directory(_mainPath))){
            // La ruta no existe o no es valida
            clear();
            printw("\nError! Revise el directorio ingresado.");
            refresh();
            napms(1500);

        }else break;

    }while(1);

    return _mainPath;

}

int LionMusicPlayer::InputManager(char direction, MenuState currentState){

    int Input= wgetch(stdscr);
    if (Input != ERR){

        if(direction == 'v'){ // Vertical
            if(Input == KEY_UP) LMP_UI.SetUserOption(LMP_UI.GetUserOption() - 1);
            else if(Input == KEY_DOWN) LMP_UI.SetUserOption(LMP_UI.GetUserOption() + 1);

        }else if(direction == 'h'){ // Horizontal
            if(Input == KEY_LEFT) LMP_UI.SetUserOption(LMP_UI.GetUserOption() - 1);
            else if(Input == KEY_RIGHT) LMP_UI.SetUserOption(LMP_UI.GetUserOption() + 1);
        }

        // Enter 
        if (Input == '\n') LMP_UI.SetOptionSelected(true);

        // Regresar (valido para cualquier menu)
        if (Input  == 'q') LMP_Controller.quitRequested = true; 

        // Control de volumen: solo mientras se esta reproduciendo musica 
        if (currentState == MenuState::Player || currentState == MenuState::PlaylistsPlayer) {
            if (Input == '+') AdjustVolume(0.1f);
            else if (Input == '-') AdjustVolume(-0.1f);
        }

        return 0;

    }

    return -1;

}

void LionMusicPlayer::AdjustVolume(float delta){

    float nuevoVolumen = LMP_Controller.getVolume() + delta;

    if (nuevoVolumen < 0.0f) nuevoVolumen = 0.0f;
    if (nuevoVolumen > 1.0f) nuevoVolumen = 1.0f;

    // Redondear a un decimal: sumar/restar 0.1f en punto flotante puede arrastrar error
    // (0.1 no es exacto en binario) y terminar en valores como 0.30000001
    nuevoVolumen = std::round(nuevoVolumen * 10.0f) / 10.0f;

    LMP_Controller.setVolume(nuevoVolumen);

    // Solo el backend que este sonando en este momento recibe el cambio de inmediato; el otro
    // lo tomara la proxima vez que arranque, leyendo Volume (ver ReproducirM4A/ReproducirGenerico)
    if (LMP_EngineReady)  ma_engine_set_volume(&LMP_Engine, nuevoVolumen);
    if (LMP_DeviceActive) ma_device_set_master_volume(&LMP_Device, nuevoVolumen);

}

int LionMusicPlayer::SyncManagers(){

    std::string currentSong = LMP_Controller.getCurrentSong();

    // Solo releer la letra y cambiar de animacion cuando la cancion actual realmente cambio.
    if (currentSong != LastSyncedSong) {
        LMP_LyricsReader.LoadLyrics(currentSong);
        LMP_AnimationManager.ChangeAnimation();
        LastSyncedSong = currentSong;
    }

    return 0;

}

double LionMusicPlayer::GetTotalDuration(){

    double totalDuration = 0.0;

    for (const auto& path : MusicLibraryPath) {
        AVFormatContext* fmtCtx = nullptr;

        if (avformat_open_input(&fmtCtx, path.c_str(), nullptr, nullptr) != 0) {
            std::cerr << "No se pudo abrir el archivo: " << path << std::endl;
            continue;
        }

        if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
            std::cerr << "No se pudo obtener info del archivo: " << path << std::endl;
            avformat_close_input(&fmtCtx);
            continue;
        }

        // La duración está en AV_TIME_BASE (microsegundos)
        if (fmtCtx->duration != AV_NOPTS_VALUE) {
            totalDuration += (double)fmtCtx->duration / AV_TIME_BASE;
        }

        avformat_close_input(&fmtCtx);
    }

    return totalDuration;

}

//~ Metodos~//

int LionMusicPlayer::BootScreen(){

    // Reproducir animacion de arranque una vez
    do {
        // Draw se encarga de actualizar y dibujar la ventana, no hace falta actualizar los frames
        if ( LMP_UI.DrawBootScreen() < 0 ){
            break;

        } 

        napms(30);

    } while (!LMP_AnimationManager.HasAnimationLooped());

    // Limpiar
    clear();
    refresh();

    // Eliminar la ventana 
    LMP_UI.CloseAnimationWindow();

    return 0;

}

int LionMusicPlayer::MenuTransition(){

    // Reproducir animacion de transicion entre menus una vez
    do {
        // Draw se encarga de actualizar y dibujar la ventana, no hace falta actualizar los frames
        LMP_UI.DrawTransitionWindow();

        napms(30);

    } while (!LMP_AnimationManager.HasAnimationLooped());

    // Eliminar la ventana de transicion
    LMP_UI.CloseAnimationWindow();
    LMP_UI.SetOptionSelected(false);

    return 0;

}

int LionMusicPlayer::PlayNextSong(){

    if (PlaylistQueue.empty()) return -1;

    // Si se llego al final de la cola, la volvemos a barajar y empezamos de nuevo
    if (CurrentQueueIndex >= QueueSize) {
        shuffle(PlaylistQueue.begin(), PlaylistQueue.end(), mt19937(random_device()()));
        CurrentQueueIndex = 0;
    }

    // PlaylistQueue solo guarda nombres de archivo, hay que anteponer el path de la biblioteca
    string ruta = MainPath + "\\" + PlaylistQueue[CurrentQueueIndex];
    CurrentQueueIndex++;

    // ReproducirCancion() bloquea hasta que la cancion termina (o se pide skip/quit) por eso
    // esta funcion SIEMPRE debe correr en el hilo de reproduccion, nunca en el hilo de la UI
    PlaySong(ruta);

    return 0;

}

void LionMusicPlayer::PlaybackThreadLoop(){

    // Recorre la cola de reproduccion hasta que se pida salir. El hilo de la UI (el que llamo
    // a ShuffleAndPlay) sigue corriendo en paralelo dibujando pantalla y leyendo el input;
    // ambos hilos se coordinan unicamente a traves de LMP_Controller (ya es thread-safe).
    while (!LMP_Controller.quitRequested) {
        PlayNextSong();
        napms(100);
    }

}

void LionMusicPlayer::MusicPlayerMainLoop(MenuState currentState){

    // Abrir la UI y reproducir la cola de reproduccion 
    while(!LMP_Controller.quitRequested){

        // Actualizar la informacion de la cancion actual en los agentes de la clase (LrcReader, AnimationManager)
        SyncManagers();

        // Actualizar la UI
        LMP_UI.DrawMusicPlayer();

        // Actualizar la Opcion del usuario (currentState habilita +/- para volumen en Player/PlaylistsPlayer)
        InputManager('h', currentState);

        // Revisar la opcion del usuario y realizar la accion correspondiente
        if(LMP_UI.GetOptionSelected()){

            switch(LMP_UI.GetUserOption()){

                case 0: // Salir del Modo
                    LMP_Controller.quitRequested= true;
                    break;

                case 1: // Pausar/Reanudar
                    LMP_Controller.paused= !LMP_Controller.paused;
                    break;

                case 2: // Siguiente Cancion
                    LMP_Controller.skipRequested= true;
                    break;

                default:
                    break;
            }

            LMP_UI.SetOptionSelected(false); // Reiniciar la opcion seleccionada

        }
    }

}

int LionMusicPlayer::ForgetUser(){

    while(LMP_UI.GetOptionSelected() == false){

        InputManager('h');

        LMP_UI.SelectionBox("hola mundo", "si");

        napms(30);

    }

    return 0;

}

LionMusicPlayer::MenuState LionMusicPlayer::Menu(){

    MenuTransition();

    // Limpiar todas las ventanas antes de usar el menu
    clear();
    // Reiniciar la posicion del usuario
    LMP_UI.SetUserOption(0);
    LMP_UI.SetOptionSelected(false);

    while(!LMP_UI.GetOptionSelected()){

        // Dibujar menu
        LMP_UI.DrawMainMenu();

        // Controlar Inputs
        InputManager('v');

        napms(30);

    }

    LMP_UI.CloseMainMenu();

    //Opciones del menu principal = 7
    // ALEATORIO, Playlist, Actualizar libreria, buscar, opciones, salir, borrar datos.

    switch (LMP_UI.GetUserOption()){
    case 0: //ShuffleAndPlay
        return MenuState::Player;

    case 1: //PlayByMood
        return MenuState::PlaylistOptions;

    case 2: // Update Library
        if (RefreshLibrary() != 0){
            return MenuState::Error;
        } 
        return MenuState::Main;

    case 3: // Seeker
        return MenuState::Library;

    case 4: // MainOptions
        return MenuState::MainOptions;

    case 5: // Exit
        return MenuState::Exit;

    case 6: // Clean User Data
        //someFunction 
        
        return MenuState::Main;

    default:
        break;
    }

    return MenuState::Main;

}

LionMusicPlayer::MenuState LionMusicPlayer::PlaylistsMenu(){

    MenuTransition();

    // Limpiar todas las ventanas antes de usar el menu
    clear();

    LMP_UI.SetOptionSelected(false);

    while(!LMP_UI.GetOptionSelected()){

        // Dibujar menu
        LMP_UI.DrawPlaylistsSelections();

        // Controlar Inputs
        InputManager('h');

        // Regresar al menu principal
        if(LMP_Controller.quitRequested ==  true){
        
        LMP_Controller.quitRequested = false;
        return MenuState::Main;
        }   

        napms(30);

    }

    //Opciones del menu = Tantas como playlist haya (0, 1, 2, ...)
    LMP_PlaylistsManager.SetPlaylist(LMP_UI.GetUserOption());    

    LMP_UI.ClosePlaylistsMenu();

    return MenuState::PlaylistsPlayer;

}

LionMusicPlayer::MenuState LionMusicPlayer::ShuffleAndPlay(){

    MenuTransition();

    // Revisar la biblioteca de musica, si no hay canciones devolver error
    if(MusicLibrarySize <= 0){
        cerr << "Biblioteca vacia" << endl;
        return MenuState::Error;
    }

    // Limpiar banderas/tiempos 
    LMP_Controller.reset();
    LastSyncedSong.clear();
    // Reiniciar la posicion del usuario
    //LMP_UI.SetUserOption(0);
    LMP_UI.SetOptionSelected(false);

    // Generar una lista aleatoria de canciones
    PlaylistQueue.clear();                                                              // Limpiar la cola de reproduccion 
    PlaylistQueue= MusicLibrary;                                                        // Copiar la lista de canciones a la cola de reproduccion
    shuffle(PlaylistQueue.begin(), PlaylistQueue.end(), mt19937(random_device()()));    // Mezclar la cola de reproduccion
    QueueSize= (int)PlaylistQueue.size();
    CurrentQueueIndex= 0;

    // Iniciar una nueva Animacion valida
    LMP_AnimationManager.ChangeAnimation();

    // Dibujar UI antes de iniciar el backend de musica.
    LMP_UI.DrawMusicPlayer();

    // Hilo de reproduccion: reproduce la cola en segundo plano 
    thread playbackThread(&LionMusicPlayer::PlaybackThreadLoop, this);

    // Manejar el control y actualizar el reproductor de musica
    MusicPlayerMainLoop(MenuState::Player);

    // Esperar a que el hilo de reproduccion termine limpiamente (ReproducirCancion revisa
    // quitRequested internamente, asi que esto no se queda colgado) antes de cerrar la UI
    if (playbackThread.joinable()) playbackThread.join();

    // CERRAR LA UI //
    LMP_UI.CloseMusicPlayer();

    return MenuState::Main;

}

LionMusicPlayer::MenuState LionMusicPlayer::PlayByMood(){

    //° Antes de entrar a esta funcion, el menu de playlists se encarga de seleccionar y cargar la playlist

    MenuTransition();

    // Revisar la biblioteca de musica, si no hay canciones devolver error
    if(MusicLibrarySize <= 0){
        cerr << "Biblioteca vacia" << endl;
        return MenuState::Error;
    }

    // Limpiar banderas/tiempos 
    LMP_Controller.reset();
    LastSyncedSong.clear();
    // Reiniciar la posicion del usuario
    LMP_UI.SetUserOption(0);
    LMP_UI.SetOptionSelected(false);

    // Generar una lista aleatoria de canciones
    PlaylistQueue = LMP_PlaylistsManager.GetShuffledPlaylistPath();
    QueueSize= PlaylistQueue.size();
    CurrentQueueIndex= 0;

    // Hilo de reproduccion: reproduce la cola en segundo plano 
    thread playbackThread(&LionMusicPlayer::PlaybackThreadLoop, this);

    // Manejar el control y actualizar el reproductor de musica
    MusicPlayerMainLoop(MenuState::PlaylistsPlayer);

    // Esperar a que el hilo de reproduccion termine limpiamente (ReproducirCancion revisa
    // quitRequested internamente, asi que esto no se queda colgado) antes de cerrar la UI
    if (playbackThread.joinable()) playbackThread.join();

    // CERRAR LA UI //
    LMP_UI.CloseMusicPlayer();

    return MenuState::Main;


}

//! Backend de reproduccion (helpers internos usados por LionMusicPlayer::ReproducirCancion) !

// Contexto compartido entre el hilo principal (decodifica) y el callback de audio (reproduce)
struct PlaybackCtx {
    ma_pcm_rb* rb;
    ma_uint32  bytesPerFrame;
};

// Callback de miniaudio: solo consume del ring buffer, nunca decodifica
static void audioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    PlaybackCtx* ctx = (PlaybackCtx*)pDevice->pUserData;
    char* pOut = (char*)pOutput;
    ma_uint32 framesRead = 0;

    while (framesRead < frameCount) {
        ma_uint32 framesToRead = frameCount - framesRead;
        void* pBufferIn;
        if (ma_pcm_rb_acquire_read(ctx->rb, &framesToRead, &pBufferIn) != MA_SUCCESS) break;
        if (framesToRead == 0) break; // no hay más datos disponibles por ahora

        memcpy(pOut + (size_t)framesRead * ctx->bytesPerFrame, pBufferIn, (size_t)framesToRead * ctx->bytesPerFrame);
        ma_pcm_rb_commit_read(ctx->rb, framesToRead);
        framesRead += framesToRead;
    }

                // Si el decodificador no alcanzó a llenar el buffer a tiempo, rellenar con silencio
    if (framesRead < frameCount) {
        memset(pOut + (size_t)framesRead * ctx->bytesPerFrame, 0, (size_t)(frameCount - framesRead) * ctx->bytesPerFrame);
    }
}

// Escribe un PCMFrame completo en el ring buffer, esperando si está lleno
static void escribirFrameEnBuffer(ma_pcm_rb* rb, PCMFrame& f, ma_uint32 bytesPerFrame) {
    const uint8_t* src = f.data.data();
    ma_uint32 framesToWrite = (ma_uint32)(f.data.size() / bytesPerFrame);

    while (framesToWrite > 0) {
        ma_uint32 framesAvailable = framesToWrite;
        void* pBufferOut;
        if (ma_pcm_rb_acquire_write(rb, &framesAvailable, &pBufferOut) != MA_SUCCESS) break;

        if (framesAvailable == 0) {
            // Buffer lleno: esperar a que el hilo de audio consuma datos
            Sleep(5);
            continue;
        }

        memcpy(pBufferOut, src, (size_t)framesAvailable * bytesPerFrame);
        ma_pcm_rb_commit_write(rb, framesAvailable);

        src += (size_t)framesAvailable * bytesPerFrame;
        framesToWrite -= framesAvailable;
    }
}

// Reproduce un archivo .m4a decodificándolo con AudioDecoder y alimentando LMP_Device (miniaudio)
void LionMusicPlayer::ReproducirM4A(const std::string& ruta) {

    AudioDecoder decoder;
    if (!decoder.openFile(ruta)) {
        std::cerr << "No se pudo abrir: " << ruta << std::endl;
        return;
    }

    // Decodificamos el primer frame para conocer sampleRate/canales reales del archivo
    PCMFrame frame;
    if (!decoder.getNextFrame(frame)) {
        std::cerr << "Archivo sin datos de audio: " << ruta << std::endl;
        decoder.close();
        return;
    }

    const int channels = frame.channels;
    const int sampleRate = frame.sampleRate;
    const ma_uint32 bytesPerFrame = (ma_uint32)channels * sizeof(int16_t);

    // Obtener solo el nombre de la cancion
    string songname = ruta.substr(ruta.find_last_of("\\") + 1);

    // " " = nombreArchivo(ruta)
    LMP_Controller.setCurrentSong(quitarExtension(songname), decoder.getDurationSeconds());

    // Ring buffer con ~1 segundo de margen entre decodificación y reproducción
    ma_pcm_rb rb;
    if (ma_pcm_rb_init(ma_format_s16, channels, sampleRate, nullptr, nullptr, &rb) != MA_SUCCESS) {
        std::cerr << "Error creando ring buffer" << std::endl;
        decoder.close();
        return;
    }

    PlaybackCtx ctx{ &rb, bytesPerFrame };

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;
    config.playback.channels = channels;
    config.sampleRate        = sampleRate;
    config.dataCallback      = audioDataCallback;
    config.pUserData         = &ctx;

    // LMP_Device es ahora un atributo de la clase (antes era local a esta funcion): se
    // re-inicializa en cada cancion .m4a, pero al vivir en el objeto, InputManager() puede
    // ajustarle el volumen mientras suena usando LMP_DeviceActive como bandera de "esta listo".
    if (ma_device_init(nullptr, &config, &LMP_Device) != MA_SUCCESS) {
        std::cerr << "Error iniciando dispositivo de audio" << std::endl;
        ma_pcm_rb_uninit(&rb);
        decoder.close();
        return;
    }

    if (ma_device_start(&LMP_Device) != MA_SUCCESS) {
        std::cerr << "Error arrancando dispositivo de audio" << std::endl;
        ma_device_uninit(&LMP_Device);
        ma_pcm_rb_uninit(&rb);
        decoder.close();
        return;
    }

    // Aplicar el volumen actual (por si el usuario ya lo habia ajustado en una cancion anterior)
    ma_device_set_master_volume(&LMP_Device, LMP_Controller.getVolume());
    LMP_DeviceActive = true;

    std::cout << "Reproduciendo: " << ruta << std::endl;

    bool devicePaused = false;
    long long framesEscritosTotal = 0;

    // Revisa LMP_Controller.paused y pausa/reanuda el dispositivo en consecuencia.
    // Mientras el dispositivo está pausado, no consume del ring buffer, así que
    // el loop de decodificación de más abajo se frena solo (el ring buffer se
    // llena y escribirFrameEnBuffer queda esperando espacio) — no hace falta
    // pausar el decoder por separado.
    // Devuelve true si hay que abortar la canción (skip o salir).
    auto manejarPausa = [&]() -> bool {
        while (LMP_Controller.paused && !LMP_Controller.skipRequested && !LMP_Controller.quitRequested) {
            if (!devicePaused) {
                ma_device_stop(&LMP_Device);
                devicePaused = true;
            }
            Sleep(50);
        }
        if (devicePaused && !LMP_Controller.paused) {
            ma_device_start(&LMP_Device);
            devicePaused = false;
        }
        return LMP_Controller.skipRequested || LMP_Controller.quitRequested;
    };

    // Primer frame ya decodificado
    escribirFrameEnBuffer(&rb, frame, bytesPerFrame);
    framesEscritosTotal += (long long)(frame.data.size() / bytesPerFrame);

    // Resto del archivo
    while (decoder.getNextFrame(frame)) {
        if (manejarPausa()) break;

        escribirFrameEnBuffer(&rb, frame, bytesPerFrame);
        framesEscritosTotal += (long long)(frame.data.size() / bytesPerFrame);
        LMP_Controller.elapsedSeconds = (int)(framesEscritosTotal / sampleRate);
        LMP_Controller.elapsedMillis = (int)((framesEscritosTotal * 1000LL) / sampleRate);
    }

    // Esperar a que el ring buffer se vacíe, es decir, a que termine de sonar
    // (salvo que nos pidan saltar o salir antes de que eso pase)
    while (ma_pcm_rb_available_read(&rb) > 0) {
        if (LMP_Controller.quitRequested || LMP_Controller.skipRequested) break;
        if (manejarPausa()) break;
        Sleep(20);
    }

    if (devicePaused) ma_device_start(&LMP_Device); // por las dudas, antes de destruir

    LMP_Controller.skipRequested = false; // ya lo consumimos, no debe afectar la próxima canción

    LMP_DeviceActive = false; // a partir de aca LMP_Device ya no es un objetivo valido para ajustar volumen
    ma_device_uninit(&LMP_Device);
    ma_pcm_rb_uninit(&rb);
    decoder.close();
}

// Reproduce cualquier formato que miniaudio decodifique de forma nativa
// (mp3, wav, flac) usando LMP_Engine (motor de miniaudio, atributo de la clase). El formato
// se detecta por contenido, no por extensión, así que esta misma función
// sirve para todos esos casos.
void LionMusicPlayer::ReproducirGenerico(const std::string& ruta) {
    // LMP_Engine (y el dispositivo de audio que abre por debajo) se crea UNA sola vez y se
    // reutiliza para todas las canciones: abrir y cerrar el dispositivo en cada cancion (como
    // se hacia antes con ma_engine_init/ma_engine_uninit por llamada) es lo que generaba el
    // "salto lento" al pasar de cancion, porque reabrir el dispositivo de audio en Windows
    // tiene un costo real. ma_engine reamuestrea automaticamente cada ma_sound al formato
    // interno del motor, asi que reutilizarlo funciona igual aunque cada archivo tenga un
    // sample rate o numero de canales distinto.
    if (!LMP_EngineReady) {
        if (ma_engine_init(NULL, &LMP_Engine) != MA_SUCCESS) return;
        // Aplicar el volumen actual apenas se crea el motor (por si el usuario ya lo habia
        // ajustado antes, por ejemplo mientras sonaba un .m4a)
        ma_engine_set_volume(&LMP_Engine, LMP_Controller.getVolume());
        LMP_EngineReady = true;
    }

    ma_sound sound;
    if (ma_sound_init_from_file(&LMP_Engine, ruta.c_str(), 0, NULL, NULL, &sound) != MA_SUCCESS) {
        cerr << "Error al iniciar: " << ruta << endl;
        return;
    }

    float totalSec = 0.0f;
    ma_sound_get_length_in_seconds(&sound, &totalSec);

    // Obtener solo el nombre de la cancion
    string songname = ruta.substr(ruta.find_last_of("\\") + 1);

    LMP_Controller.setCurrentSong(quitarExtension(songname), (int)totalSec);

    ma_sound_start(&sound);

    // ma_sound_stop() no pierde la posición: llamar a ma_sound_start() de
    // nuevo retoma desde donde se quedó, así que pausar es así de simple.
    bool devicePaused = false;

    while (ma_sound_is_playing(&sound) || devicePaused) {
        if (LMP_Controller.quitRequested || LMP_Controller.skipRequested) break;

        if (LMP_Controller.paused && !devicePaused) {
            ma_sound_stop(&sound);
            devicePaused = true;
        } else if (!LMP_Controller.paused && devicePaused) {
            ma_sound_start(&sound);
            devicePaused = false;
        }

        float cursorSec = 0.0f;
        ma_sound_get_cursor_in_seconds(&sound, &cursorSec);
        LMP_Controller.elapsedSeconds = (int)cursorSec;
        LMP_Controller.elapsedMillis = (int)(cursorSec * 1000.0f);

        Sleep(100);
    }

    LMP_Controller.skipRequested = false; // ya lo consumimos, no debe afectar la próxima canción

    ma_sound_uninit(&sound);
}

// Reproduce canción según extensión: .m4a va por el decoder de FFmpeg,
// todo lo demás (mp3, wav, flac) va por el motor nativo de miniaudio
void LionMusicPlayer::PlaySong(const std::string& ruta) {
    if (ruta.find(".m4a") != std::string::npos) {
        ReproducirM4A(ruta);
    } else {
        ReproducirGenerico(ruta);
    }
}

// Convierte segundos a formato HH:MM:SS
string SecondsToHMS(double seconds) {
    int totalSeconds = static_cast<int>(seconds);

    int hours   = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs    = totalSeconds % 60;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours   << ":"
        << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << secs;

    return oss.str();
}