#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

    //* Esta libreria proporciona un objeto PlaylistManager que se encarga de la lectura de archivos .m3u
    //* los objetos creados a partir de esta clase son capaces de almacenar la ruta de cada una de las playlist en un directorio, 
    //* tambien pueden leer y almacenar los datos de una sola playlist a la vez.
    //* El principal objetivo de esta libreria es entregar un objeto capaz de generar listas de musica aleatoria
    //* a partir de una playlist seleccionada. Si el proyecto continua con normalidad se agregara 
    //* las funciones para crear y editar playlist dentro de algun directorio. */

#include <string>
#include <vector>

//* Estructura de datos de una cancion *//
typedef struct{

    std::string title;  // Titulo de la cancion
    std::string artist; // Artista de la cancion
    std::string length; // Duracion de la cancion

} SongData;

//* Clase PlaylistManager *//
class PlaylistManager{

    //~ Atributos Generales ~//
    std::string DirectoryPath;              // Carpeta en la que se encuentran las playlist
    int PlaylistsNumber;
    std::vector<std::string> PlaylistsPath; // Rutas de las playlists encontradas
    std::vector<std::string> Playlists;     // Nombre de las listas de reproduccion
    int CurrentPlaylistIndex;               // Indice de la playlist sobre la que se tiene informacion actualmente

    //~ Atributos sobre la playlist ~//
    std::string CurrentPlaylistName;                    // Nombre de la playlist
    std::vector<std::string> CurrentPlaylistSongsPath;  // Lista de canciones (nombre y extension)
    std::vector<SongData> SongInfo;                     // Información adicional (si el archivo .m3u contiene #EXTM3U)

    //~ Metodos Privados ~//
    // Obtener todos los m3u dentro de la carpeta especificada y devolver un vector de rutas de archivos .m3u
    int SearchPlaylists();
    // Lee la linea #EXTINFO  de un archivo m3u extendido y devuelve la información de la canción
    int parseExtendedInfo(const std::string& line);
    //Lee la linea de la ruta de la cancion
    int parseSongPath(const std::string& line);
    // Lee un archivo .m3u y obtiene su informacion dentro de sus atributos.
    int readPlaylist();
    //limpia los atributos sobre alguna playlist para poder cambiar de playlist
    void cleanPlaylistData();


public:

    //° Constructor. Tambien revisa el directorio desde que se instancia
    PlaylistManager(std::string _DirectoryPath);

    //~ Propiedades ~//
    // Devuelve el nombre de la playlist que contiene el PlayistManager actualmente
    std::string Get_CurrentPlaylist();
    // Devuelve el numero de Playlist encontradas en el directorio 
    int Get_PlaylistsNumber();
    // Devuelve el nombre de todas las playlist encontradas
    std::vector<std::string> Get_PLaylists();
    // Identifica si es un m3u simple o extendido
    bool IsActualPlaylistExtendedM3U();
    // Devuelve las rutas de todas las canciones encontradas
    std::vector<std::string> GetSongsPath();
    // Cambia y carga una playlist dentro de si
    void SetPlaylist(int _PlaylistIndex);
    // Devuelve una lista aleatorizada de las rutas de los archivos de playlist (.m3u) encontrados en el directorio (dirrecion relativa al DirectiryPath. Incluye extencion)
    std::vector<std::string> GetShuffledPlaylistPath();
    // Cambia el DirectoryPath al que apunta y busca playlists
    void SetDirectoryPath(std::string _DirectoryPath);

    //~ Metodos Publicos ~//
    // Crea un archivo .m3u extendido con el nombre especificado y lo guarda en la carpeta 
    // especificada o en el directorio asignado al Manager
    int NewPlaylist(const std::string& playlist_name, const std::string& directoryPath);
    int NewPlaylist(const std::string& playlist_name);
    // Añade una canción a un archivo .m3u extendido existente
    int AddSongToPlaylist(const std::string& playlist_name, const std::string& song_path, const std::string& directoryPath);
    int AddSongToPlaylist(const std::string& playlist_name, const std::string& song_path);
    // Elimina una cancion de una playlist, si la cancion no se encuentra devuelve -1
    int RemoveSongFromPlaylist(const std::string& playlist_name, const std::string& song_path, const std::string& directoryPath);
    int RemoveSongFromPlaylist(const std::string& playlist_name, const std::string& song_path);

};

#endif // PLAYLISTMANAGER_H