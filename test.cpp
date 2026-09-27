
    // ! Comando De Compilacion //
//* g++ $(ls *.cpp | grep -v main.cpp) -o test -IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib -I/mingw64/include/ncursesw -lavformat -lavcodec -lavutil -lswresample -lncursesw


    //ToDo: Diseñar el resto del menu principal
    //ToDo: Diseñar los metodos para que LMPlayer reproduxca musica
    //ToDo: Agregar Opciones para cambiar de colores o directorio
    //ToDo: Agregar funcion para borrar datos
    //ToDo:

// MODULOS 
#include "LMPlayer.h"

#include <memory>

using namespace std;


int main(){

    auto LMPlayer = LionMusicPlayer();

    LMPlayer.ForgetUser();

    /*
    if (LMPlayer.BootScreen() != 0) {
        cerr << "Error al mostrar la pantalla de arranque." << endl;
    }
    */

    // Estado inicial
   /* LionMusicPlayer::MenuState state = LionMusicPlayer::MenuState::Main;

    // State Machine & Bucle principal
    while(state != LionMusicPlayer::MenuState::Exit) {
        switch(state) {

            case LionMusicPlayer::MenuState::Main:
                state = LMPlayer.Menu(); 
                break;

            case LionMusicPlayer::MenuState::PlaylistOptions:
                state = LMPlayer.PlaylistsMenu();
                break;

            case LionMusicPlayer::MenuState::Player:
                state = LMPlayer.ShuffleAndPlay();
                // Al terminar la reproducción, regresa al menú principal
                //state = LionMusicPlayer::MenuState::Main;
                break;
            
            case LionMusicPlayer::MenuState::PlaylistsPlayer:
                state = LMPlayer.PlayByMood();

            case LionMusicPlayer::MenuState::Library:
                // Aquí podrías implementar un menú de búsqueda
                state = LionMusicPlayer::MenuState::Main;
                break;

            case LionMusicPlayer::MenuState::MainOptions:
                // Aquí podrías implementar opciones generales
                state = LionMusicPlayer::MenuState::Main;
                break;

            case LionMusicPlayer::MenuState::Error:
                std::cerr << "Error detectado, regresando al menú principal\n";
                state = LionMusicPlayer::MenuState::Main;
                break;

            default:
                state = LionMusicPlayer::MenuState::Main;
                break;
        }
    }
    */

    return 0;

}
