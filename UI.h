#ifndef UI_H
#define UI_H

    //* Esta libreria se encarga especificamente de la interfaz de usuario de este programa
    //* por lo que no es reutilizable o modular a diferencia de otras librerias 
    //* Su objetivo principal es entregar una clase UI, la cual puede ser usada para
    //* Abrir un grupo de ventanas o una ventana en especifico, actualizarlas y cerrarlas

#include "animationManager.h"
#include "LrcReader.h"
#include "playlistManager.h"
#include "playerControl.h"
#include <ncursesw/ncurses.h> 
#include <chrono>
#include <string>

//* CLASE UI *//
class UI {

    //~ ATRIBUTOS ~//
    int MainWinRows;                                           // Numero de filas de la ventana compleata
    int MainWinCols;                                           // Numero de columnas de la ventana completa
    int HighlightColorPair= 1;                                 // Indice del color principal para la UI
    int ComplementaryColorPair= 2;                             // Indice del color secundario para la UI
    std::chrono::steady_clock::time_point AnimationLastUpdate; // Ultima actualizacoion de la ventana de animacion
    
    //~ Atributos sobre Desiciones del usuario ~//
    int UserOption = 0;                               // Opcion/Ubicacion del cursor sobre el que el usuario cree que esta
    int LastUserOption = 0;                           // Ultima posicion del usuario (Asegurarse de actualizarla si se va a usar)
    WINDOW* FocusWindow = nullptr;                    // es la ventana sobre la que el usuario esta/puede interactuar
    MEVENT* MouseEvent = nullptr;                     // Sirve para optener propiedades y eventos del mouse (posicion/clicks/scroll, etc)
    bool OptionSelected = false;                      // Sirve para saber si el usuario selecciono una opcion
    //~ Atributos de las ventanas ~//
    WINDOW* MainMenuWindow = nullptr;                 // Menu Principal
    WINDOW* MainMenuInfoWindow = nullptr;             // Ventana en la que se muestra informacion sobre el reproductor de musica
    WINDOW* MainMenuTipWindow = nullptr;              // Ventana que da sugerencias o mensajes al usuario
    WINDOW* PlayerSongInfoWindow = nullptr;           // Informacion sobre la cancion que se esta reproduciendo
    WINDOW* PlayerOptionsWindow = nullptr;            // Opciones del reproductor (pausa/play, quit, next) 
    WINDOW* PlayerProgressBarWindow = nullptr;        // Barra de progreso de la cancion que se esta reproduciendo
    WINDOW* PlayerLrcWindow = nullptr;                // Letras de la cancion que se esta reproduciendo
    WINDOW* PlaylistsSelectionWindow = nullptr;       // Menu de seleccion de playlits
    WINDOW* PlaylistsOptionsWindow = nullptr;         // Opciones de playlist (añadir, agregar, quitar)
    WINDOW* AnimationsWindow = nullptr;               // Ventana de animaciones
    WINDOW* SelectionBoxWindow = nullptr;             // Ventana para seleccion de opciones simple 

    //~ Atributos (Objetos) ~//
    LrcReader* LMP_LrcReader = nullptr;                // Lector de letras de canciones del sistema
    PlaylistManager* LMP_PlaylistMngr = nullptr;       // Gestionador de Playlist general
    AnimationManager* LMP_AnimationMngr = nullptr;     // Gestor de animaciones general
    //~ Atributos (estructura) ~//
    // Conexion entre el hilo de reproduccion y el de UI, se obtiene informacion de reproduccion desde el
    PlayerControl* LMP_Control = nullptr; 

    //~ Datos para el menu de opciones principal
    // Nombres de las opciones y sus posiciones en X
    const char* options[7] = {  "S H U F L E", 
                                "F E E L S", 
                                "REFRESH LIBRARY", 
                                "SEARCH SONG", 
                                "OPTIONS", 
                                "EXIT!", 
                                "FORGET ME"};

    // Centrado manual de las opciones
    int x_pos[7] = {15-6, 15-5, 15-8, 15-6, 15-4, 15-3, 15-5}; 

    //~ Datos para el menu de informacion
    // informacion
    const char* InfoForMenu[4] = {  "TOTAL SONGS:", 
                                    "TOTAL PLAYLISTS:", 
                                    "TOTAL ANIMATIONS:", 
                                    "LIBRARY DURATION:", };

    // variables
    std::string LibraryDurationFormat;  // duracion de la libreruia en formato
    int LibraryTotalSongs;              // total de canciones cargadas
    int LibraryTotalPlaylists;          // total de playlists encontradas
    int LibraryTotalAnimations;         // total de animaciones encontradas
    int FontInt = 0, BackgroundInt = 0; // para el color de la ui

    //~ Metodos Privados ~//
    int InitMainMenuOptions();              // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitMenuInfoWindow();               // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitMenuTipWindow();                // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlayerSongInfoWindow();         // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlayerOptionsWindow();          // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlayerProgressBarWindow();      // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlayerLrcWindow();              // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlaylistsSelectionWindow();     // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitPlaylistsOptions();             // Crea el puntero de la ventana y ajusta sus caracteristicas
    int InitAnimationWindow(int x, int y, int cols, int rows); 

    int DrawMainMenuOptions();              // Crea, dibuja y actualiza la ventana
    int DrawMenuAnimationWindow();          // Crea, dibuja y actualiza la ventana
    int DrawMenuInfoWindow();               // Crea, dibuja y actualiza la ventana
    int DrawMenuTipWindow();                // Crea, dibuja y actualiza la ventana
    int DrawPlayerSongInfoWindow();         // Crea, dibuja y actualiza la ventana 
    int DrawPlayerOptionsWindow();          // Crea, dibuja y actualiza la ventana
    int DrawPlayerAnimationWindow();        // Crea, dibuja y actualiza la ventana
    int DrawPlayerProgressBarWindow();      // Crea, dibuja y actualiza la ventana
    int DrawPlayerLrcWindow();              // Crea, dibuja y actualiza la ventana
    int DrawPlaylistsSelectionMenu();       // Crea, dibuja y actualiza la ventana

    int CloseMenuInfoWindow();
    int CloseMenuTipsWindow();

    // Crea una ventana e inicia una animacion especifica
    int DrawAnimationWindow(const std::string& animationFile,int x, int y, bool draw_box);

    void UserOptionManager(int min,int max);// Monitorea el valor del UserOption para que no sobrepase los limites
    

public:

    //° Constructor, Inicia Ncurses °//
    UI() = default;
    UI(int _highligth_color, int _complementary_color, LrcReader& _lrc_reader,
                                                        PlaylistManager& _playlist_manger,
                                                        AnimationManager& _animation_manager,
                                                        PlayerControl& _lmp_controller);

    //~ Propiedades ~//
    // Asigna el objeto de lector de letras
    void SetLrcReader(LrcReader* _LMP_LrcReader);
    // Asigna un objeto Gestionador de Playlist
    void SetPlaylistManager(PlaylistManager* _LMP_PlaylistMngr);
    // Asigna un gestor de animaciones
    void SetAnimationManager(AnimationManager* _LMP_Animation_Mngr);
    // Asigna el controlador para el reproductor de musica
    void SetPlayerContoller(PlayerControl* _LMP_Player_Control);
    // asgina un nuevo par de colres para la UI
    void SetColors(int _highligth_color, int _complementary_color = 0);
    // Cambia el valor del UserOption
    void SetUserOption(int _new_user_option_pos);
    // Recibe el puntero de la ventana con la que se esta interactuando
    void SetFocusWindow(WINDOW* _focus_window);
    // Recibe un puntero MEVENT (Mouse Event);
    void SetMouseEvent(MEVENT* _mouse_event);
    // Recibe la duracion total de la libreria
    void SetLibraryDuration(std::string format);
    // Recibe el total de canciones 
    void SetTotalSongs(int _totalSongs);
    // Recibe el total de playlists encontradas
    void SetTotalPlaylists(int _totalPlaylists);
    // Recibe el total de Animaciones encontradas
    void SetTotalAnimations(int _totalAnimations);

    // Devuelve el valor del UserOption
    int GetUserOption() const { return UserOption; }
    // Cambia el valor de OptionSelected 
    void SetOptionSelected(bool _new_value) { OptionSelected= _new_value; }
    // Devuelve el valor de OptionSelected
    bool GetOptionSelected() const { return OptionSelected; }

    //~ Metodos ~//
    
    // Dibuja todas las subventanas del menu princiapal (ventanas MainMenu), y las crea de ser necesario
    int DrawMainMenu();
    // Dibuja el reproductor de musica (ventanas "Player"),y crea las ventanas de ser necesario
    int DrawMusicPlayer();
    // Dibuja todas las ventanas para la seleccion de playlists
    int DrawPlaylistsSelections();
    // Dibuja el menu de opciones de las playlist, crea la ventana de ser necesario
    int DrawPlaylistsOptions();
    //Dibuja la ventana de arranque, crea la ventana de ser necesario
    int DrawBootScreen();
    // Dibuja la ventana de transicion entre menus, crea la ventana de ser necesario
    int DrawTransitionWindow();     

    // Cierra y termina la ventana de arranque
    int CloseBootScreen();
    // Cierra y termina la ventana de opciones del menu
    int CloseMainMenuOptions();
    // Cierra y termina el reproductor de musica
    int CloseMusicPlayer();
    // Cierra y termina el menu de playlists
    int ClosePlaylistsMenu();
    // Cierra y termina elmenu de opciones de playlists
    int ClosePlaylistsOptions();
    // Cierra y termina la ventana de transicion entre menus
    int CloseTransitionWindow();
    // Cierrra y termina las ventanas del menu principal
    int CloseMainMenu();
    // Cierra la ventna de Animacion general
    int CloseAnimationWindow();
    // Fuerza el cierre de todas las ventanas abiertas y termina la UI
    void CloseAllWindows();

    // Obtiene una string en un cuadro de texto que ocupa toda la ventana
    std::string GetString(std::string Title);
    // Pide al usuario seleccionar un color (devuelve el int del pair del color)
    int SelectColor();
    // Imprime un mensaje en una nueva ventana y pide seleccionar una opcion
    int SelectionBox(const std::string &message, const std::string &options);

};

// -- Otros
std::string quitarExtension(const std::string &titulo);




#endif //° _UI_H