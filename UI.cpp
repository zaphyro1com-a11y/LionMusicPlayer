#include "UI.h"

#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#include <ncursesw/ncurses.h>
#include <chrono>
#include <locale.h>

using namespace std;

//° Constructor °//
UI::UI(int _highligth_color, int _complementary_color, LrcReader& _lrc_reader,
                                                        PlaylistManager& _playlist_manger,
                                                        AnimationManager& _animation_manager,
                                                        PlayerControl& _lmp_controller
    ) : HighlightColorPair(_highligth_color), ComplementaryColorPair(_complementary_color),
        LMP_LrcReader(&_lrc_reader), LMP_PlaylistMngr(&_playlist_manger), LMP_AnimationMngr(&_animation_manager), LMP_Control(&_lmp_controller){

    // setlocale(LC_ALL, "") en Windows NO activa UTF-8: resuelve al codepage por defecto del
    // sistema (tipicamente CP1252 o el que tenga configurado Windows), asi que ncursesw sigue
    // interpretando mal cualquier secuencia multi-byte (tildes, ñ) aunque esta llamada ya
    // estuviera antes de initscr(). ".UTF8" es la forma que reconoce el runtime de MSYS2/MinGW
    // (UCRT) para pedir UTF-8 explicitamente. Y aunque la locale del programa este bien, la
    // CONSOLA de Windows tiene su propio codepage de entrada/salida, independiente del locale,
    // que tampoco es UTF-8 por defecto - de ahi los SetConsole*CP. Los tres deben ir antes de
    // initscr(), que es cuando ncurses consulta las capacidades de la terminal.
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    initscr();      // Inicia ncurses
    cbreak();       // Entrada sin esperar Enter
    noecho();       // No mostrar teclas pulsadas
    keypad(stdscr, TRUE); // Habilitar teclas especiales (F1, flechas, etc.)
    curs_set(0);
    // getch() espera como maximo 50ms por una tecla y despues devuelve ERR en vez de
    // bloquear para siempre. Esto es lo que evita que el loop de UI/input se congele o
    // gire sin control (ver SelectColor/DrawBootScreen mas abajo para el otro lado del
    // problema: el parpadeo no era por el getch, era por un clear() de mas en cada frame).
    wtimeout(stdscr, 50);

    // Colores 
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);  // seleccionado (basico)        CYAN
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // no seleccionado (basico)     WHTE
    init_pair(3, COLOR_RED, COLOR_BLACK);   //                              RED
    init_pair(4, COLOR_GREEN, COLOR_BLACK); //                              GRENN
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);//                              YELLOW
    init_pair(6, COLOR_BLUE, COLOR_BLACK);  //                              BLUE
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);//                             MAGENTA
    init_pair(8, COLOR_BLACK, COLOR_BLACK); //                              BLACK

    getmaxyx(stdscr, MainWinRows, MainWinCols); // filas y columnas de la pantalla

}

//~ Funciones Init ~//

int UI::InitMainMenuOptions(){

    // la ventana ya esta creada
    if (MainMenuWindow != nullptr) return 0;

    // Iniciar La Ventana
    // filas = 8
    // Columnas = 50
    // pos= esquina superior izquierda
    MainMenuWindow = newwin(10, 30, 4, 15);

    // borde sin resaltar
    wattron(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));
    box(MainMenuWindow, 0, 0);
    wattroff(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));
    
    if (MainMenuWindow == nullptr) return -1;

    return 0;

}

int UI::InitPlayerSongInfoWindow(){

    // si la ventana ya esta creada
    if (PlayerSongInfoWindow != nullptr) return 0;

    // height = 3;
    // width = win_cols-2;
    PlayerSongInfoWindow = newwin(3, MainWinCols-2, 4, 1);

    // Error al crear ventana
    if (PlayerSongInfoWindow == nullptr) return -1;

    // Borde de ventana resaltado
    wattron(PlayerSongInfoWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerSongInfoWindow, 0, 0); 
    wattroff(PlayerSongInfoWindow, COLOR_PAIR(HighlightColorPair));

    //Dibujar
    wrefresh(PlayerSongInfoWindow);

    return 0;

}

int UI::InitMenuInfoWindow(){

    // si la ventana ya existe 
    if (MainMenuInfoWindow != nullptr) return 0;

    //Crear ventana
    MainMenuInfoWindow = newwin(12, (MainWinCols/2) * 0.75, MainWinRows-14, 6);

    // Error de crear venatana
    if (PlayerOptionsWindow == nullptr) return -1;

    return 0;

}

int UI::InitMenuTipWindow(){

    // si la ventana ya existe 
    if (MainMenuTipWindow != nullptr) return 0;

    //Crear ventana
    MainMenuTipWindow = newwin(12, (MainWinCols/2) * 0.5, MainWinRows-15, MainWinCols - (MainWinCols/2) + 2 );

    // Error de crear venatana
    if (MainMenuTipWindow == nullptr) return -1;

    return 0;

}

int UI::InitPlayerOptionsWindow(){

    // si la ventana ya existe 
    if (PlayerOptionsWindow != nullptr) return 0;

    //Crear ventana
    PlayerOptionsWindow = newwin(5, MainWinCols, MainWinRows - 5, 0);

    // Error de crear venatana
    if (PlayerOptionsWindow == nullptr) return -1;

    // Borde de la ventana resaltado
    wattron(PlayerOptionsWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerOptionsWindow, 0, 0); 
    wattroff(PlayerOptionsWindow, COLOR_PAIR(HighlightColorPair));
    
    // Mensaje inicial
    mvwprintw(PlayerOptionsWindow, 1, (MainWinCols - 29) / 2, "Seleccione usando ( <- y -> )");
    //Dibujar
    wrefresh(PlayerOptionsWindow);

    return 0;

}

int UI::InitPlayerProgressBarWindow(){

    // si la ventana ya existe
    if (PlayerProgressBarWindow != nullptr) return 0;

    // crear ventana
    PlayerProgressBarWindow = newwin(3, MainWinCols - 2, MainWinRows - 8, 1);

    // error al crear ventana
    if (PlayerProgressBarWindow == nullptr) return -1;

    // Borde de ventana resaltado
    wattron(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerProgressBarWindow, 0, 0); // Dibujar borde base
    wattroff(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));

    // Dibujar
    wrefresh(PlayerProgressBarWindow);

    return 0;

}

int UI::InitPlayerLrcWindow(){

    // si la ventana ya existe
    if (PlayerLrcWindow != nullptr) return 0;

    // Definir dimensiones y posición de la ventana
    if(MainWinCols > 75 && MainWinRows > 10){
        // crear ventana si hay suficiente espacio para mostrar las letras
        // siempre ubicado a la derecha del UI
        PlayerLrcWindow= newwin(15, MainWinCols - 39, 7, MainWinCols-(MainWinCols - 36));

        // Error al crear ventana
        if (PlayerLrcWindow == nullptr ) return -1;

        // borde sin resaltar
        wattron(PlayerLrcWindow, COLOR_PAIR(ComplementaryColorPair));
        box(PlayerLrcWindow, 0, 0);
        wattroff(PlayerLrcWindow, COLOR_PAIR(ComplementaryColorPair));
        
        // dibujar
        wrefresh(PlayerLrcWindow);

        return 0;

    }else return -1;

}

int UI::InitPlaylistsSelectionWindow(){

    // si la ventana ya existe
    if (PlaylistsSelectionWindow != nullptr) return 0;

    // Definir dimensiones y posición de la ventana
    PlaylistsSelectionWindow = newwin(5, 55, 5, 2);

    // Error al crear ventana
    if (PlaylistsSelectionWindow == nullptr) return -1;

    // Dibujar 
    wrefresh(PlaylistsSelectionWindow);

    return 0;

}

int UI::InitPlaylistsOptions(){

    // si la ventana ya existe
    if (PlaylistsOptionsWindow != nullptr) return 0;

    // Definir dimensiones y posición de la ventana
    PlaylistsOptionsWindow = newwin(7, 25, (MainWinRows/2)-3, (MainWinCols/2)-13);

    // Error al crear la venta 
    if (PlaylistsOptionsWindow == nullptr) return -1;

    // Borde sin resaltar
    wattron(PlaylistsOptionsWindow, COLOR_PAIR(ComplementaryColorPair));
    box(PlaylistsOptionsWindow, 0, 0); 
    wattroff(PlaylistsOptionsWindow, COLOR_PAIR(ComplementaryColorPair));
    
    // Mensaje Inicial
    mvwprintw(PlaylistsOptionsWindow, 1, (MainWinCols - 29) / 2, "Seleccione usando ( <- y -> )");
    // Dibujar
    wrefresh(PlaylistsOptionsWindow);

    return 0;

}

int UI::InitAnimationWindow(int x, int y, int cols, int rows){

    // Si la ventana ya existe regresar
    if (AnimationsWindow != nullptr ){
        // Comprobar que las caracteristicas de la ventana sean 
        // las mismas que se solicitaron de lo contrario volver a crear
        int _x, _y, _cols, _rows;
        getbegyx(AnimationsWindow,_y, _x);
        getmaxyx(AnimationsWindow, _rows, _cols);

        if( _x != x || _y != y || _cols != cols || _rows != rows ){
            //limpiar y borrar
            wclear(AnimationsWindow);
            wrefresh(AnimationsWindow);

            delwin(AnimationsWindow);
            AnimationsWindow = nullptr;

        } else {
            // Las caracteristicas Coinciden
            // no es necesario volver a crear la ventana
            return 1;

        }

    }

    // Comprobar dimensiones de la terminal
    if( MainWinCols < cols || MainWinRows < rows){
        cerr << "El tamaño de la terminal no es adecuado para la animacion" << endl;
        cerr << cols << " " << rows << endl;   
        cerr << MainWinCols << " " << MainWinRows << endl; 
        return -1;

    }

    // Crear nueva ventna
    AnimationsWindow = newwin(rows, cols, y, x);

    // Error al crear Ventana
    if (AnimationsWindow == nullptr){
        cerr << "No se puedo crear la ventana de animacion" << endl;
        cerr << x << " " << y << endl;

        return -1;
    }

    return 0;

}

//~ Funciones Draw ~//

int UI::DrawAnimationWindow(const std::string& animationFile, int x, int y, bool draw_box){

    // El tamaño de la ventna sera extactamente 2u mas grande que el de la animacion para dejar espacio a la box
    // Cargar animacion en Manager
    if(LMP_AnimationMngr->GetAnimationName() != quitarExtension(animationFile) ){
        if (LMP_AnimationMngr->PlayAnimation(animationFile) != 0) {
            cerr << "Error al cargar la animacion "<< animationFile << " ." << endl;
            return -1;

        }
    }

    // Obtener datos de la animacion
    int cols= 2 + LMP_AnimationMngr->Size_x;
    int rows= 2 + LMP_AnimationMngr->Size_y;

    // InitAnimation... Se encarga de crear la ventna una sola vez
    if (InitAnimationWindow(x, y, cols, rows) < 0){
        // Error al crear la ventana 
        cerr << "Error al crear la ventana de animacion" << endl;
        return -1;

    }

    // Limpiar
    werase(AnimationsWindow); 

    // Dibujar Borde o no
    if(draw_box){
        wattron(AnimationsWindow, COLOR_PAIR(ComplementaryColorPair));
        box(AnimationsWindow, 0, 0);
        wattroff(AnimationsWindow,  COLOR_PAIR(ComplementaryColorPair));
    }

    //~ Dibujar Animacion ~

    //? Calcular diferencia de tiempo para actualizar animacion //
    // Tiempo Actual
    chrono::steady_clock::time_point now = chrono::steady_clock::now();
    // Tiempo Transcurrido (diferencia de tiempo entre la ultima iteracion)
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - AnimationLastUpdate).count();

    // Si ya pasó el tiempo (timming) en milisegundos, avanzamos un frame
    if (elapsed >= LMP_AnimationMngr->Timming) {
        LMP_AnimationMngr->NextFrame();
        
        // Reiniciar Temporizador
        AnimationLastUpdate= chrono::steady_clock::now();
    }

    // Obtener el frame del Gestor de animaciones
    vector<string> CurrentFrame= LMP_AnimationMngr->Frames[LMP_AnimationMngr->GetCurrentFrame()]; 

    // Pintar el FRAME
    wattron(AnimationsWindow, COLOR_PAIR(HighlightColorPair)); 
    // Dibujar el frame actual
    for ( int i=0; i < LMP_AnimationMngr->Size_y;  i++){

        mvwprintw(AnimationsWindow, i+1, 1, "%s", CurrentFrame[i].c_str());

    }
    wattroff(AnimationsWindow, COLOR_PAIR(HighlightColorPair));

    // actualizar (wnoutrefresh, no wrefresh: quien llama a esta funcion hace UN solo
    // doupdate() al final, junto con las demas ventanas del mismo frame - ver nota en
    // DrawMusicPlayer/DrawMainMenu/DrawBootScreen)
    wnoutrefresh(AnimationsWindow);

    return 0;

}

int UI::DrawMainMenuOptions(){

    // revisar que la ventana este abierta y de lo contrario crearla
    if (MainMenuWindow == nullptr){
        // error al crear ventana
        if (InitMainMenuOptions() != 0) return -1;
    }

    int w, h;
    getmaxyx(MainMenuWindow, h, w);

    // Actualizar ventana
    werase(MainMenuWindow);

    // Borde de la ventna
    wattron(MainMenuWindow, COLOR_PAIR(HighlightColorPair));
    box(MainMenuWindow, 0, 0);
    wattroff(MainMenuWindow, COLOR_PAIR(HighlightColorPair));

    // Titulo
    wattron(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(MainMenuWindow, 1, (w/2) - 6, "O P T I O N S");
    wattroff(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));

    // Monitorear UserOption
    UserOptionManager(0, 6);

    // Dibujar las 3 opciones cercanas
    for (int i = -1; i < 2; i++) {
        int idx = UserOption + i;

        if (idx < 0 || idx >= 7) { 
            // fuera de rango, no dibujar
            continue; 
        }
        
        if (i == 0){
            wattron(MainMenuWindow, COLOR_PAIR(HighlightColorPair));
        } else {                
            wattron(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));
        }

        // i + 5 porque las opciones empiezan en la fila 4 de la ventana
        mvwprintw(MainMenuWindow, i + 5, x_pos[idx], "%s", options[idx]);

        if (i == 0){
            wattroff(MainMenuWindow, COLOR_PAIR(HighlightColorPair));
        } else {                
            wattroff(MainMenuWindow, COLOR_PAIR(ComplementaryColorPair));
        }
        
    }

    // actualizar (wnoutrefresh: el doupdate lo hace DrawMainMenu al final del frame)
    wnoutrefresh(MainMenuWindow);

    // Comprobar si el usuario selecciono una opcion
    if(OptionSelected){
        OptionSelected= false;
        wclear(MainMenuWindow);
        wnoutrefresh(MainMenuWindow);
        delwin(MainMenuWindow);

    }

    return 0;

}

int UI::DrawMenuAnimationWindow(){

    // Revisar que el animationManager tenga animaciones
    if (!LMP_AnimationMngr->HasAnimations()) return -1;

    int f = DrawAnimationWindow( "Lio.aaf", (MainWinCols/2) + 4, 1, true); 

    return f;

}

int UI::DrawMenuInfoWindow(){

    // revisar que la ventana este abierta y de lo contrario crearla
    if (MainMenuInfoWindow == nullptr){
        if (InitMenuInfoWindow() != 0){
            // error al crear ventana
            cerr << "Error al crear la ventana de informacion para el menu" << endl;
            return -1;
        }
        
    }

    int w, h;
    getmaxyx(MainMenuInfoWindow, h, w);

    // Actualizar ventana
    werase(MainMenuInfoWindow);

    // Borde de la ventna
    wattron(MainMenuInfoWindow, COLOR_PAIR(ComplementaryColorPair));
    box(MainMenuInfoWindow, 0, 0);
    wattroff(MainMenuInfoWindow, COLOR_PAIR(ComplementaryColorPair));

    // Titulo
    wattron(MainMenuInfoWindow, COLOR_PAIR(HighlightColorPair));
    mvwprintw(MainMenuInfoWindow, 1, (w/2) - 3, "I N F O");
    wattroff(MainMenuInfoWindow, COLOR_PAIR(HighlightColorPair));

    // Dibujar Informacion
    // Total de canciones
    // Total de playlist
    // Total de animaciones
    // Duracion de la libreria
    //Info a partir de y=4

    //Dibujar tags
    wattron(MainMenuInfoWindow, COLOR_PAIR(HighlightColorPair));
    for(int i= 0; i< 4; i++){
        mvwprintw(MainMenuInfoWindow, i+4, 3, "%s", InfoForMenu[i]);

    }
    wattroff(MainMenuInfoWindow, COLOR_PAIR(HighlightColorPair));

    // Imprimir la informacion
    //informacion a partir de x =22
    wattron(MainMenuInfoWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(MainMenuInfoWindow, 4, 22, "%d", LibraryTotalSongs);
    mvwprintw(MainMenuInfoWindow, 5, 22, "%d", LibraryTotalPlaylists);
    mvwprintw(MainMenuInfoWindow, 6, 22, "%d", LibraryTotalAnimations);
    mvwprintw(MainMenuInfoWindow, 7, 22, "%s", LibraryDurationFormat.c_str());
    wattroff(MainMenuInfoWindow, COLOR_PAIR(ComplementaryColorPair));

    // Refrescar (wnoutrefresh: el doupdate lo hace quien compone el frame - DrawMainMenu/DrawPlaylistsSelections)
    wnoutrefresh(MainMenuInfoWindow);

    return 0;


}

int UI::DrawMenuTipWindow(){

    // revisar que la ventana este abierta y de lo contrario crearla
    if (MainMenuTipWindow == nullptr){
        if (InitMenuTipWindow() != 0){
            // error al crear ventana
            cerr << "Error al crear la ventana de Tips para el menu" << endl;
            return -1;
        }
        
    }

    int w, h;
    getmaxyx(MainMenuTipWindow, h, w);

    // Actualizar ventana
    werase(MainMenuTipWindow);

    // Borde de la ventna
    wattron(MainMenuTipWindow, COLOR_PAIR(ComplementaryColorPair));
    box(MainMenuTipWindow, 0, 0);
    wattroff(MainMenuTipWindow, COLOR_PAIR(ComplementaryColorPair));

    // Titulo
    wattron(MainMenuTipWindow, COLOR_PAIR(HighlightColorPair));
    mvwprintw(MainMenuTipWindow, 1, (w/2) - 3, "T I P S");
    wattroff(MainMenuTipWindow, COLOR_PAIR(HighlightColorPair));

    // Refrescar (wnoutrefresh: el doupdate lo hace quien compone el frame)
    wnoutrefresh(MainMenuTipWindow);

    return 0;

}

int UI::DrawPlayerSongInfoWindow(){

    // revisar que la ventana este creada
    if ( PlayerSongInfoWindow == nullptr){
        if ( InitPlayerSongInfoWindow() != 0) {
            cerr << "Error al crear la ventana de informacion para el reproductor" << endl;
            return -1;
        }

    }

    // limpiar primero (werase, no wclear: ver nota en DrawBootScreen)
    werase(PlayerSongInfoWindow);

    // Borde de ventana resaltado
    wattron(PlayerSongInfoWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerSongInfoWindow, 0, 0); 
    wattroff(PlayerSongInfoWindow, COLOR_PAIR(HighlightColorPair));
    // Texto
    string CurrentSong = LMP_Control->getCurrentSong();
    int SongName_x = (MainWinCols/2)-(CurrentSong.size()/2);
    if (SongName_x < 1) SongName_x = 1; // Evitar que el texto se salga por la izquierda
    wattron(PlayerSongInfoWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(PlayerSongInfoWindow, 1, SongName_x, "%s", CurrentSong.c_str()); 
    wattroff(PlayerSongInfoWindow, COLOR_PAIR(ComplementaryColorPair));

    // Dibujar (wnoutrefresh: el doupdate lo hace DrawMusicPlayer al final del frame)
    wnoutrefresh(PlayerSongInfoWindow);

    return 0;

}

int UI::DrawPlayerAnimationWindow(){ 

    // Revisar que el animationManager tenga animaciones
    if (!LMP_AnimationMngr->HasAnimations()) {
        cerr << "No hay animaciones validas" << endl;
        return -1;
    }

    int f = DrawAnimationWindow(LMP_AnimationMngr->GetAnimationName() + ".aaf", 4, 7, true); 

    return f;

}

int UI::DrawPlayerLrcWindow(){

    // Crear ventana
    if (PlayerLrcWindow == nullptr ){
        // Error al abrir ventana
        if (InitPlayerLrcWindow() != 0) {
            cerr << "Error al iniciar la ventana de Lyrics" << endl;
            return -1;
        }
    }

    // Datos de la venata 
    int h, w; /* --> */ getmaxyx(PlayerLrcWindow, h, w);
    int Lyrics_color= 0;
    string CurrentLine;

    // Limpiar ventana y redibujar vorde
    werase(PlayerLrcWindow);
    wattron(PlayerLrcWindow, COLOR_PAIR(ComplementaryColorPair));
    box(PlayerLrcWindow, 0, 0);
    wattroff(PlayerLrcWindow, COLOR_PAIR(ComplementaryColorPair));

    // Revisar si el archivo tiene Letra
    if(!LMP_LrcReader->IsValid()){
        // Mensaje por defecto
        const char* msg = "No se encontro letra";
        mvwprintw(PlayerLrcWindow, h/2, (w/2) - (strlen(msg)/2), "%s", msg);
        wnoutrefresh(PlayerLrcWindow);
        return 0;

    }

    // Verificar que el archivo tenga marcas de tiempo y determinar la funcion usada para obtener la linea actual
    if (LMP_LrcReader->isLrcSynchronized()){
        // Obtener linea
        CurrentLine= LMP_LrcReader->GetCurrentLine(LMP_Control->elapsedMillis);
        // Elegir Color
        Lyrics_color= HighlightColorPair;

    } else {
        // Obtener linea //! TEMPORAL se debe cambiar el atributo totalSecons de PlayerControl
        CurrentLine= LMP_LrcReader->GetUnsyncedLineByTime(LMP_Control->elapsedMillis, LMP_Control->totalSeconds * 1000);
        // Elegir Color
        Lyrics_color= ComplementaryColorPair;
    }

    // Dibujar Linea
    // centrado
    int start_x = (w / 2) - (CurrentLine.length() / 2);
    if (start_x < 1) start_x = 1; // Evitar que el texto se salga por la izquierda

    wattron(PlayerLrcWindow, COLOR_PAIR(Lyrics_color));
    mvwprintw(PlayerLrcWindow, h/2, start_x, "%s", CurrentLine.c_str());
    wattroff(PlayerLrcWindow, COLOR_PAIR(Lyrics_color));

    // Refrescar ventana (wnoutrefresh: el doupdate lo hace DrawMusicPlayer al final del frame)
    wnoutrefresh(PlayerLrcWindow);

    return 0;

}

int UI::DrawPlayerProgressBarWindow(){

    // Dibujar Ventana
    if (PlayerProgressBarWindow == nullptr){
        // Error al abrir ventana
        if (InitPlayerProgressBarWindow() != 0) {
            cerr << "Error al iniciar la ventana para la barra de progreso del reproductor" << endl;
            return -1;
        }

    }

    // Total de barras
    int ttl_bars = (MainWinCols - 2) - 14; 

    // Copia local de los atomics: mvwprintw es variadica y no acepta std::atomic directamente
    int elapsed = LMP_Control->elapsedSeconds;
    int total = LMP_Control->totalSeconds;

    // Dibujar Borde
    wattron(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerProgressBarWindow, 0, 0);
    wattroff(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));

    // Inicio de la barra y tiempo transcurrido
    wattron(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(PlayerProgressBarWindow, 1, 1, " %-4d |-", elapsed);
    wattron(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));

    // Progreso en barras
    int progreso = 0;
    if (total > 0) {
        progreso = (ttl_bars * elapsed) / total;
    }

    // Dibujar cada barra
    for(int i = 0; i < ttl_bars; i++){
        if(i < progreso){
            wattron(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));
            mvwprintw(PlayerProgressBarWindow, 1, 7+i, ">");
            wattroff(PlayerProgressBarWindow, COLOR_PAIR(HighlightColorPair));
        } else {
            wattron(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));
            mvwprintw(PlayerProgressBarWindow, 1, 7+i, "-");
            wattroff(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));
        }
    }

    // Fin de la barra y tiempo total
    wattron(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(PlayerProgressBarWindow, 1, (MainWinCols - 2) - 8, "-|%-4d ", total);
    wattron(PlayerProgressBarWindow, COLOR_PAIR(ComplementaryColorPair));

    // Refrescar (wnoutrefresh: el doupdate lo hace DrawMusicPlayer al final del frame)
    wnoutrefresh(PlayerProgressBarWindow);

    return 0;

}

int UI::DrawPlayerOptionsWindow(){

    // Abrir ventana
    if (PlayerOptionsWindow == nullptr){
        // Error al abrir ventana
        if (InitPlayerOptionsWindow() != 0) {
            cerr << "Error al iniciar la ventna de opciones para el reproductor" << endl;
            return -1;
        } 
    }

    // Monitorear la posicion del usuario
    UserOptionManager(0, 2);

    //~ Opciones
    const char* state_sign[2] = {"||","|>"};   
    const char* back_sign = "(X)";
    const char* next_sign = ">>";
    const char* volume_sign[2] = {"<} ))", "<} X"};

    // Limpiar 
    werase(PlayerOptionsWindow);

    // Info
    // Volumen
    wattron(PlayerOptionsWindow, COLOR_PAIR(ComplementaryColorPair));
    mvwprintw(PlayerOptionsWindow, 2, 2, "Volume (- +)");
    mvwprintw(PlayerOptionsWindow, 3, 3, "%s : %d", volume_sign[(LMP_Control->getVolume() > 0.0f )? 0 : 1], (int)(LMP_Control->getVolume()*10));
    wattroff(PlayerOptionsWindow, COLOR_PAIR(UserOption == 0? HighlightColorPair : ComplementaryColorPair));

    //box
    wattron(PlayerOptionsWindow, COLOR_PAIR(HighlightColorPair));
    box(PlayerOptionsWindow, 0, 0);
    wattroff(PlayerOptionsWindow, COLOR_PAIR(HighlightColorPair));
    // Dibujar
    wattron(PlayerOptionsWindow, COLOR_PAIR(UserOption == 0? HighlightColorPair : ComplementaryColorPair));
    mvwprintw(PlayerOptionsWindow, 3, (MainWinCols / 2) - 10, "%s", back_sign);
    wattroff(PlayerOptionsWindow, COLOR_PAIR(UserOption == 0? HighlightColorPair : ComplementaryColorPair));

    wattron(PlayerOptionsWindow, COLOR_PAIR(UserOption == 1? HighlightColorPair : ComplementaryColorPair));
    mvwprintw(PlayerOptionsWindow, 3, (MainWinCols / 2), "%s", state_sign[LMP_Control->paused]);
    wattroff(PlayerOptionsWindow, COLOR_PAIR(UserOption == 1? HighlightColorPair : ComplementaryColorPair));

    wattron(PlayerOptionsWindow, COLOR_PAIR(UserOption == 2? HighlightColorPair : ComplementaryColorPair));
    mvwprintw(PlayerOptionsWindow, 3, (MainWinCols / 2) + 10, "%s", next_sign);
    wattroff(PlayerOptionsWindow, COLOR_PAIR(UserOption == 2? HighlightColorPair : ComplementaryColorPair));
    
    // Refrescar (wnoutrefresh: el doupdate lo hace DrawMusicPlayer al final del frame)
    wnoutrefresh(PlayerOptionsWindow);

    return 0;
}

int UI::DrawPlaylistsSelectionMenu() {

    // Dibujar ventana
    if (PlaylistsSelectionWindow == nullptr){
        if (InitPlaylistsSelectionWindow() != 0) return -1;

    }

    // Datos de la ventana
    int w, h;

    getmaxyx(PlaylistsSelectionWindow, h, w);

    werase(PlaylistsSelectionWindow);

    // Borderline
    wattron(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));
    box(PlaylistsSelectionWindow, 0, 0);
    wattroff(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));

    mvwprintw(PlaylistsSelectionWindow, 0, w/2 - 10, "Seleccion de Playlist");

    // Obtener playlist
    vector<string> PlaylistsNames = LMP_PlaylistMngr->Get_PLaylists();

    // Revision de limites: evita acceso fuera de rango si UserOption viene de otro menu
    // con mas opciones, o si aun no hay playlists
    if (PlaylistsNames.empty()) return -1;
    if (UserOption < 0) UserOption = 0;
    if (UserOption >= (int)PlaylistsNames.size()) UserOption = (int)PlaylistsNames.size() - 1;

    // Dibujar segun el espacio disponible
    if(MainWinCols > 100){
        // tags
        const char* current = PlaylistsNames[UserOption].c_str(); 
        const char* previous = (UserOption == 0) ? " " : PlaylistsNames[UserOption - 1].c_str();
        const char* next = (UserOption == LMP_PlaylistMngr->Get_PlaylistsNumber() - 1) ? " " : PlaylistsNames[UserOption+ 1].c_str();

        // Calculos de posición (x) 
        int current_len = strlen(current);
        int previous_len = strlen(previous);

        int current_x = (w / 2) - (current_len / 2);
        int previous_x = current_x - 3 - previous_len; 
        int next_x = current_x + current_len + 3;

        // Dibujar Opciones
        // anterior
        wattron(PlaylistsSelectionWindow, COLOR_PAIR(ComplementaryColorPair));
        mvwprintw(PlaylistsSelectionWindow, 2, previous_x, "%s", previous);
        wattroff(PlaylistsSelectionWindow, COLOR_PAIR(ComplementaryColorPair));
        
        // actual
        wattron(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));
        mvwprintw(PlaylistsSelectionWindow, 2, current_x, "%s", current);
        wattroff(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));
        
        // siguiente
        wattron(PlaylistsSelectionWindow, COLOR_PAIR(ComplementaryColorPair));
        mvwprintw(PlaylistsSelectionWindow, 2, next_x, "%s", next);
        wattroff(PlaylistsSelectionWindow, COLOR_PAIR(ComplementaryColorPair));

    } else {
        // mostrar solo la actual
        wattron(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));
        mvwprintw(PlaylistsSelectionWindow, 2, (w/2) - (strlen(PlaylistsNames[UserOption].c_str()) / 2), "%s", PlaylistsNames[UserOption].c_str());
        wattroff(PlaylistsSelectionWindow, COLOR_PAIR(HighlightColorPair));
    }

    // wnoutrefresh: el doupdate lo hace DrawPlaylistsSelections al final del frame
    wnoutrefresh(PlaylistsSelectionWindow);

    return 0;

}

int UI::DrawPlaylistsOptions(){

    // Crear la ventana
    if (PlaylistsOptionsWindow == nullptr){ 
        if( InitPlaylistsOptions() != 0) return -1;

    }

    // Opciones y centrado manual
    const char* options[4] = {"Add Playlist", "Add Song", "Remove Song", "Back"};
    int x_pos[4] = {12-5, 12-6, 12-4, 12-3}; // Centrado manual

    // El tamaño y la posicion de texto no cambian por lo que no hace falta limpiar la ventana, solo sobresscribirla
    // Sobreescribir
    for (int i = 0; i < 4; i++) {
        if (i == UserOption) {
            wattron(PlaylistsOptionsWindow, COLOR_PAIR(HighlightColorPair));
        } else {
            wattron(PlaylistsOptionsWindow, COLOR_PAIR(ComplementaryColorPair));
        }
        
        mvwprintw(PlaylistsOptionsWindow, 3, x_pos[i], "%s", options[i]);
        
        if (i == UserOption) {
            wattroff(PlaylistsOptionsWindow, COLOR_PAIR(HighlightColorPair));
        } else {
            wattroff(PlaylistsOptionsWindow, COLOR_PAIR(ComplementaryColorPair));
        }
    }

    wrefresh(PlaylistsOptionsWindow);

    return 0;

}

int UI::DrawMainMenu(){

    int result = 0;

    if (DrawMainMenuOptions() < 0) { result = -1; }
    if (result == 0 && DrawMenuAnimationWindow() < 0 ) { 
        cerr << "Error al dibujar animacion para el menu principal" << endl;
        result = -1;
     }
    if (result == 0 && DrawMenuInfoWindow() < 0 ) { result = -1; }

    if (result == 0 && DrawMenuTipWindow() < 0 ) { result = -1; }

    // Un solo doupdate() para las 4 sub-ventanas (ver nota en DrawMusicPlayer)
    doupdate();

    return result;

}

int UI::DrawMusicPlayer(){

    int result = 0;

    if ( DrawPlayerSongInfoWindow() != 0 || DrawPlayerAnimationWindow() != 0 ||
         DrawPlayerLrcWindow() != 0 || DrawPlayerProgressBarWindow() != 0 || DrawPlayerOptionsWindow() != 0 ){
        cerr << "Error al Dibujar las ventanas de la UI" << endl;
        result = -1;
    }

    doupdate();

    return result;

}

int UI::DrawPlaylistsSelections(){

    int result = 0;

    if(DrawPlaylistsSelectionMenu() < 0){
        cerr << "Error al crear la ventana de seleccion de playlists" << endl;
        result = -1;

    }

    if (result == 0 && DrawMenuAnimationWindow() < 0 ) { 
        cerr << "Error al dibujar animacion para el menu principal" << endl;
        result = -1;
     }
    if (result == 0 && DrawMenuInfoWindow() < 0 ) { result = -1; }

    if (result == 0 && DrawMenuTipWindow() < 0 ) { result = -1; }

    // Un solo doupdate() para las sub-ventanas (ver nota en DrawMusicPlayer)
    doupdate();

    return result;

}

// Animaciones Predeterminadas
int UI::DrawBootScreen(){

    // pos: centrado en pantalla
    int f = DrawAnimationWindow("boot_animation.aaf", (MainWinCols/2) - 40, (MainWinRows/2) - 12, false);

    // DrawAnimationWindow ahora solo usa wnoutrefresh (ver nota en DrawMusicPlayer); como esta
    // funcion se llama sola (no como parte de un composite), necesita su propio doupdate().
    doupdate();

    return f;
}

int UI::DrawTransitionWindow(){

    // pos: centrado en pantalla
    int f = DrawAnimationWindow("transition_animation.aaf", (MainWinCols/2) - 40, (MainWinRows/2) - 12, false);

    // Mismo motivo que en DrawBootScreen: doupdate() propio, no forma parte de un composite.
    doupdate();

    return f;
}

//~ Funciones Close ~//

int UI::CloseMenuInfoWindow(){

    //revisar que la ventana este abierta
    if (MainMenuInfoWindow == nullptr ) return -1;

    // Limpiar y cerrar
    wclear(MainMenuInfoWindow);   
    wrefresh(MainMenuInfoWindow);
    delwin(MainMenuInfoWindow);
    MainMenuInfoWindow = nullptr;
    
    return 0;

}

int UI::CloseMenuTipsWindow(){

    //revisar que la ventana este abierta
    if (MainMenuTipWindow == nullptr ) return -1;

    // Limpiar y cerrar
    wclear(MainMenuTipWindow);   
    wrefresh(MainMenuTipWindow);
    delwin(MainMenuTipWindow);
    MainMenuTipWindow = nullptr;
    
    return 0;

}

int UI::CloseAnimationWindow(){

    //revisar que la ventana este abierta
    if (AnimationsWindow == nullptr ) return -1;

    // Limpiar y cerrar
    wclear(AnimationsWindow);   
    wrefresh(AnimationsWindow);
    delwin(AnimationsWindow);
    AnimationsWindow = nullptr;
    
    return 0;

}

int UI::CloseMainMenuOptions(){

    //revisar que la ventana este abierta
    if (MainMenuWindow == nullptr ) return -1;

    // Limpiar y cerrar
    wclear(MainMenuWindow);   
    wrefresh(MainMenuWindow);
    delwin(MainMenuWindow);
    MainMenuWindow = nullptr;
    
    return 0;
}

int UI::CloseMusicPlayer(){

    // cerrar cada subventana del reproductor
    // si el puntero esta vacio la funcion devuelve ERR y continua
    delwin(PlayerSongInfoWindow);
    CloseAnimationWindow();
    delwin(PlayerLrcWindow);
    delwin(PlayerProgressBarWindow);
    delwin(PlayerOptionsWindow);

    PlayerSongInfoWindow = nullptr;
    PlayerLrcWindow = nullptr;
    PlayerProgressBarWindow = nullptr;
    PlayerOptionsWindow = nullptr;

    // Limpiar toda la ventana
    clear();
    refresh();

    return 0;

}

int UI::ClosePlaylistsMenu(){

    //revisar que la ventana este abierta
    if (PlaylistsSelectionWindow == nullptr ) return -1;

    // Limpiar y cerrar
    clear();
    delwin(PlaylistsSelectionWindow);
    PlaylistsSelectionWindow = nullptr;

    CloseAnimationWindow();

    // cerrar info y tips 
    CloseMenuInfoWindow();
    CloseMenuTipsWindow();

    refresh();
    
    return 0;

}

int UI::ClosePlaylistsOptions(){

    //revisar que la ventana este abierta
    if (PlaylistsOptionsWindow == nullptr ) return -1;

    // Limpiar y cerrar
    wclear(PlaylistsOptionsWindow);   
    wrefresh(PlaylistsOptionsWindow);
    delwin(PlaylistsOptionsWindow);
    PlaylistsOptionsWindow = nullptr;
    
    return 0;

}

int UI::CloseMainMenu(){

    CloseMainMenuOptions();
    CloseAnimationWindow();

    // cerrar info y tips 
    CloseMenuInfoWindow();
    CloseMenuTipsWindow();

    return 0;

}

void UI::CloseAllWindows(){

    // Cerrar todas las ventanas abiertas
    CloseAnimationWindow();
    CloseMainMenu();
    CloseMusicPlayer();
    ClosePlaylistsMenu();
    ClosePlaylistsOptions();

    //Cerrar ncurses
    endwin();

}



//~ Propiedades (Getters & Setters) ~//

void UI::SetLrcReader(LrcReader* _LMP_LrcReader){

    // Asignar
    LMP_LrcReader = _LMP_LrcReader;

}

void UI::SetAnimationManager(AnimationManager* _LMP_Animation_Mngr){

    // Asignar
    LMP_AnimationMngr = _LMP_Animation_Mngr;

}

void UI::SetPlaylistManager(PlaylistManager* _LMP_PlaylistMngr){

    // Asignar
    LMP_PlaylistMngr = _LMP_PlaylistMngr;

}

void UI::SetPlayerContoller(PlayerControl* _LMP_Player_Control){

    // Asignar
    LMP_Control = _LMP_Player_Control;

}

void UI::SetColors(int _highligth_color, int _complementary_color){

    if(_highligth_color > 0){
        HighlightColorPair= _highligth_color;

    }

    if(_complementary_color > 0){
        ComplementaryColorPair = _complementary_color;

    }

}

void UI::SetUserOption(int _new_user_option_pos){

    // Sin restricciones
    UserOption= _new_user_option_pos;

}

void UI::SetFocusWindow(WINDOW* _focus_window){

    if( _focus_window != nullptr){
        FocusWindow= _focus_window;

    }

}

void UI::SetMouseEvent(MEVENT* _mouse_event){

    MouseEvent= _mouse_event;

}

void UI::SetLibraryDuration(string format){

    LibraryDurationFormat= format;

}

void UI::SetTotalSongs(int _totalSongs){
    LibraryTotalSongs= _totalSongs;

}

void UI::SetTotalPlaylists(int _totalPlaylists){
    LibraryTotalPlaylists= _totalPlaylists;

}

void UI::SetTotalAnimations(int _totalAnimations){
    LibraryTotalAnimations= _totalAnimations;

}

// -- OTROS

string UI::GetString(string Title){

    string String;
    // Datos de la ventana
    int y, x;
    getmaxyx(stdscr, y, x);

    // Limpiar ventana
    clear();
    refresh();

    // Generar cuadro de texto 
    WINDOW *TextBox = newwin(5, x-4, (y/2)-3, 2);
    box(TextBox, 0, 0); // borde
    mvwprintw(TextBox, 0, (Title.size()/2)+1, " %s ", Title.c_str());
    wrefresh(TextBox);

    //Buffer para wgetnstr
    char buffer[256];

    // posicionar cursor dentro del cuadro y leer una string
    wmove(TextBox, 3, 1);
    
    // mostrar cursor y teclas
    echo();
    curs_set(1);

    // Leer string
    wgetnstr(TextBox, buffer, sizeof(buffer) - 1);

    // Guardar en string
    String = string(buffer);

    // Ocultar cursor y teclas
    noecho();
    curs_set(0);

    // Cerrar la ventana
    clear();
    refresh();
    delwin(TextBox);
    
    return String;

}

int UI::SelectColor(){

    // Subventana
    WINDOW* ColorSelectWindow = newwin(15, 10, (MainWinRows/2)-7, (MainWinCols/2)-5);
    // Opciones del menu (COLORES)
    vector<string> Colors= {"CYAN", "WHITE", "RED", "GREEN", "YELLOW", "BLUE", "MAGENTA", "BLACK"};

    // Monitorear UserOoption
    UserOptionManager(0, 7);

    // Dibujar opciones
    for(int i=0; i<(int)Colors.size(); i++){
        int pair = (i == UserOption) ? UserOption+1 : ComplementaryColorPair;
        wattron(ColorSelectWindow, COLOR_PAIR(pair));
        mvwprintw(ColorSelectWindow, i+1, 6-(int)(Colors[i].length()/2), "%s", Colors[i].c_str());
        wattroff(ColorSelectWindow, COLOR_PAIR(pair));
    }

    // Actualizar ventana
    wrefresh(ColorSelectWindow);

    // Devolver la opcion seleccionada
    if(OptionSelected){
        OptionSelected= false;
        delwin(ColorSelectWindow);
        return UserOption+1;

    }

    // Cerrar la ventana
    delwin(ColorSelectWindow);
    
    
    return 0;

}

int UI::SelectionBox(const std::string &message, const std::string &options){


    //° Crear una nueva ventana temporal (tamaño, pos)
    int size_x= (MainWinCols/2), size_y;
    bool word_wrapping = false;

    //Asegurarse de crear la ventana solo una vez
    if (SelectionBoxWindow == nullptr){
        // Elegir el alto de la ventana segun el tamaño del mensaje
        if(message.length() > size_x - 4 ){
            // (message.length / (MainWinRows/2) - 4) -> Lineas que ocupara el mensaje
            // 5 -> borde + espacio entre mensaje y ocpines 
            size_y= (message.length() / (size_x - 4)) + 5;
            
            word_wrapping= true;

        } else {
            // Dimensiones normales
            size_y = 6; 

        }

        // Crear Ventana
        SelectionBoxWindow = newwin(size_y, size_x, (MainWinRows/2)-(size_y/2), size_x - (size_x/2));

    }

    //° Dibujar

    // limpiar
    werase(SelectionBoxWindow);

    // Borde
    wattron(SelectionBoxWindow, COLOR_PAIR(HighlightColorPair));
    box(SelectionBoxWindow, 0, 0);
    wattroff(SelectionBoxWindow, COLOR_PAIR(HighlightColorPair));

    return 0;

}

void UI::UserOptionManager(int min, int max){
    if (UserOption < min) {
        UserOption = max;

    } else if (UserOption > max) {
        UserOption = min;
    }
}

string quitarExtension(const string& titulo) {
    size_t punto = titulo.find_last_of('.');
    if (punto != std::string::npos && punto > 0) {
        return titulo.substr(0, punto);
    }
    return titulo; // si no hay extensión, devuelve el original
}
