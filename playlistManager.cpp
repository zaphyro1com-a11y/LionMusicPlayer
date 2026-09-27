#include "playlistManager.h"

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <random>
#include <filesystem>

using namespace std;

//~ Metodos Privados ~//
PlaylistManager::PlaylistManager(std::string _DirectoryPath){

    DirectoryPath= _DirectoryPath;

    // revisar Directorio
    if(SearchPlaylists() <= -1){
        PlaylistsNumber = 0;
        CurrentPlaylistIndex = -1; // No se ha seleccionado una playlist aun
        return;
    }

    // asignamos valores a los atributos segun Playlists

    PlaylistsNumber= Playlists.size();
    CurrentPlaylistIndex= -1; // No se ha seleccionado una playlist aun
}

int PlaylistManager::SearchPlaylists(){

    vector<string> m3uFilesPath;
    vector<string> m3uFilesName; 
    WIN32_FIND_DATAA fileData;         
    
    HANDLE hFind = FindFirstFileA((DirectoryPath + "\\*.m3u").c_str(), &fileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            string nombreArchivo = fileData.cFileName;
            //Guardar path completo
            m3uFilesPath.push_back(DirectoryPath + "\\" + nombreArchivo);
            
            // Buscar el punto de la extensión (.m3u) y cortar el texto hasta ahí
            size_t ultimoPunto = nombreArchivo.find_last_of("."); 
            if (ultimoPunto != std::string::npos) {
                nombreArchivo = nombreArchivo.substr(0, ultimoPunto); 
            }

            // Guardamos solo el nombre limpio
            m3uFilesName.push_back(nombreArchivo);
            
        } while (FindNextFileA(hFind, &fileData));
        
        FindClose(hFind);
    } else {
        return -1;
    }

    PlaylistsPath= m3uFilesPath;
    Playlists= m3uFilesName;

    return 0;

}

int PlaylistManager::parseExtendedInfo(const std::string& line){

    if(PlaylistsNumber < 1){
        return -1;
    }

    SongData _songInfo;
    size_t firstComma = line.find(',');
    size_t firstDash = line.find('-', firstComma + 1);

    if (firstComma != std::string::npos && firstDash != std::string::npos) {
        _songInfo.length = line.substr(8, firstComma - 8);

        std::string artist = line.substr(firstComma + 1, firstDash - firstComma - 1);
        std::string title  = line.substr(firstDash + 1);

        // recortar espacios en los bordes
        auto trim = [](std::string s){
            size_t start = s.find_first_not_of(" \t");
            size_t end   = s.find_last_not_of(" \t");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };

        _songInfo.artist = trim(artist);
        _songInfo.title  = trim(title);
    }

    // guardar informacion de la cancion en el vector
    SongInfo.push_back(_songInfo);

    return 0;

}

//° NOTA: esta funcion antepone siempre DirectoryPath a "line", por lo que
//° AddSongToPlaylist() debe llamarse siempre con song_path como una RUTA
//° RELATIVA a DirectoryPath (nunca una ruta absoluta)
int PlaylistManager::parseSongPath(const std::string& line){

    if( PlaylistsNumber < 1){
        return -1;
    }

    // Devolver los nombres completos de las canciones + extension
    CurrentPlaylistSongsPath.push_back(line); 
    return 0;
}

int PlaylistManager::readPlaylist(){

    // abrir archivo
    ifstream playlistFile(PlaylistsPath[CurrentPlaylistIndex]);

    // leer
    if (playlistFile.is_open()){

        string line;

        if(IsActualPlaylistExtendedM3U()) { getline(playlistFile, line); } //saltar cabecera

        while(getline(playlistFile, line)){

            // saltar espacios vacios
            if (line.empty()) continue;

            if (line.rfind("#EXTINF:", 0) == 0) {
                // línea de metadata: la siguiente línea debe ser la ruta
                parseExtendedInfo(line);

                string pathLine;
                if (std::getline(playlistFile, pathLine) && !pathLine.empty()) {
                    parseSongPath(pathLine);
                }
            } else {
                // linea de ruta: el archivo es simple, la propia linea ya es la ruta
                parseSongPath(line);
            }
        }
    } else {
        return -1;
    }

    // asignar el nombre de la playlist actual
    CurrentPlaylistName= Playlists[CurrentPlaylistIndex];

    return 0;
}

void PlaylistManager::cleanPlaylistData(){

    CurrentPlaylistName.clear();
    CurrentPlaylistSongsPath.clear();
    SongInfo.clear();

}

//~ Propiedades ~//
string PlaylistManager::Get_CurrentPlaylist(){

    // si no se ha leido un archivo aun
    if (CurrentPlaylistIndex == -1){ return "";}

    // devolver el nombre, no el path
    return Playlists[CurrentPlaylistIndex];
}

int PlaylistManager::Get_PlaylistsNumber(){

    return PlaylistsNumber;
}

vector<string> PlaylistManager::Get_PLaylists(){

    return Playlists;
}

bool PlaylistManager::IsActualPlaylistExtendedM3U(){

    if (CurrentPlaylistIndex < 0) return false;

    ifstream playlistFile(PlaylistsPath[CurrentPlaylistIndex]);

    string line;
    // revisar la primera linea
    getline(playlistFile, line);
    // cerrar
    playlistFile.close();

    // Devuelve true si la línea comienza con "#EXTM3U"
    return line.rfind("#EXTM3U", 0) == 0; 

}

vector<string> PlaylistManager::GetSongsPath(){

    return CurrentPlaylistSongsPath;

}

void PlaylistManager::SetPlaylist(int _PlaylistIndex){

    if (_PlaylistIndex < 0 || _PlaylistIndex >= PlaylistsNumber) return;

    // limpiar los datos de la playlist anterior antes de cargar la nueva
    cleanPlaylistData();

    // asignar el valor
    CurrentPlaylistIndex= _PlaylistIndex;

    // leer la playlist
    if(readPlaylist() != 0) return;

}

vector<string> PlaylistManager::GetShuffledPlaylistPath() {

    // semilla
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // nuevo vector de canciones
    vector<string> ShuffledSongs = CurrentPlaylistSongsPath;

    mt19937 rng(seed); // motor aleatorio recomendado
    // barajar
    shuffle(ShuffledSongs.begin(), ShuffledSongs.end(), rng);

    return ShuffledSongs;
}

void PlaylistManager::SetDirectoryPath(string _DirectoryPath) {

    // revisar la ruta
    if(_DirectoryPath.empty() ) return ;

    // Cambiar ruta
    DirectoryPath= _DirectoryPath;

    // Buscar Playlists
    if(SearchPlaylists() <= -1){
        PlaylistsNumber = 0;
        CurrentPlaylistIndex = -1; // No se ha seleccionado una playlist aun
        return;
    }

    // asignamos valores a los atributos segun Playlists
    PlaylistsNumber= Playlists.size();
    CurrentPlaylistIndex= -1; // No se ha seleccionado una playlist aun

}

//~ Metodos Publicos ~//

int PlaylistManager::NewPlaylist(const std::string& playlist_name, const std::string& directoryPath){

    string fullPath = directoryPath + "\\" + playlist_name + ".m3u";

    // Revisar si el archivo ya existe
    ifstream checkFile(fullPath);
    if (checkFile.is_open()) {
        checkFile.close();
        return 1; // No crear el archivo si ya existe
    }

    ofstream playlistFile(fullPath);    

    if (playlistFile.is_open()) {

        playlistFile << "#EXTM3U\n"; // Cabecera de playlist extendida
        playlistFile.close();

    } else return -1; // No se pudo crear el archivo.

    // si el archivo se creo dentro del directorio que administra este manager,
    // actualizar tambien la cache de playlists conocidas
    if (directoryPath == DirectoryPath) {
        Playlists.push_back(playlist_name);
        PlaylistsPath.push_back(fullPath);
        PlaylistsNumber = Playlists.size();
    }

    return 0;

}

int PlaylistManager::NewPlaylist(const std::string& playlist_name){

    string fullPath = DirectoryPath + "\\" + playlist_name + ".m3u";

    // Revisar si el archivo ya existe
    ifstream checkFile(fullPath);
    if (checkFile.is_open()) {
        checkFile.close();
        return 1; // No crear el archivo si ya existe
    }

    ofstream playlistFile(fullPath);    

    if (playlistFile.is_open()) {

        playlistFile << "#EXTM3U\n"; // Cabecera de playlist extendida
        playlistFile.close();

    } else return -1; // No se pudo crear el archivo.

    // actualizar la cache de playlists conocidas
    Playlists.push_back(playlist_name);
    PlaylistsPath.push_back(fullPath);
    PlaylistsNumber = Playlists.size();

    return 0;

} 

int PlaylistManager::AddSongToPlaylist(const std::string& playlist_name, const std::string& song_path, const std::string& directoryPath) {
    namespace fs = std::filesystem;

    fs::path fullPath = fs::path(directoryPath) / (playlist_name + ".m3u");

    // Revisar que el archivo existe
    if (!fs::exists(fullPath)) {
        return -1;  // el archivo no existe
    }

    // Abrir en modo append
    ofstream playlistFile(fullPath, std::ios::app);

    if (playlistFile.is_open()) {
        // Extraer nombre del archivo
        fs::path file(song_path);
        string fileName = file.filename().string();

        // Añadir metadata y ruta
        playlistFile << "#EXTINF:-1," << fileName << "\n";
        playlistFile << song_path << "\n"; // usar ruta completa o relativa

        playlistFile.close();

    }else return -1;

    return 0;
}

int PlaylistManager::AddSongToPlaylist(const std::string& playlist_name, const std::string& song_path) {
    namespace fs = std::filesystem;

    fs::path fullPath = fs::path(DirectoryPath) / (playlist_name + ".m3u");

    // Revisar que el archivo existe
    if (!fs::exists(fullPath)) {
        return -1;  // el archivo no existe
    }

    // Abrir en modo append
    ofstream playlistFile(fullPath, std::ios::app);

    if (playlistFile.is_open()) {
        // Extraer nombre del archivo
        fs::path file(song_path);
        string fileName = file.filename().string();

        // Añadir metadata y ruta
        playlistFile << "#EXTINF:-1," << fileName << "\n";
        playlistFile << song_path << "\n"; // usar ruta completa o relativa

        playlistFile.close();

    } else return -1;

    return 0;
}

int PlaylistManager::RemoveSongFromPlaylist(const std::string& playlist_name, const std::string& song_path, const std::string& directoryPath) {
    namespace fs = std::filesystem;

    string fullPath = directoryPath + "\\" + playlist_name + ".m3u";

    // Revisar que el archivo existe
    if (!fs::exists(fullPath)) {
        return -1; // el archivo no existe
    }

    ifstream playlistFile(fullPath);
    vector<string> lines;
    string line;

    // Leer todas las líneas del archivo
    while (getline(playlistFile, line)) {
        lines.push_back(line);
    }
    playlistFile.close();

    // Abrir el archivo para reescribirlo
    ofstream outFile(fullPath);
    bool skipNextLine = false;

    for (const auto& currentLine : lines) {
        if (skipNextLine) {
            skipNextLine = false; // Saltar la línea de ruta de la canción
            continue;
        }

        if (currentLine.rfind("#EXTINF:", 0) == 0) {
            // Línea de metadata, verificar si la siguiente línea es la ruta que queremos eliminar
            size_t index = &currentLine - &lines[0]; // Obtener el índice actual
            if (index + 1 < lines.size() && lines[index + 1] == song_path) {
                skipNextLine = true; // Marcar para saltar la siguiente línea
                continue; // No escribir esta línea de metadata
            }
        }

        outFile << currentLine << "\n"; // Escribir la línea al archivo
    }

    outFile.close();

    return 0;

}

int PlaylistManager::RemoveSongFromPlaylist(const std::string& playlist_name, const std::string& song_path) {
    namespace fs = std::filesystem;

    string fullPath = DirectoryPath + "\\" + playlist_name + ".m3u";

    // Revisar que el archivo existe
    if (!fs::exists(fullPath)) {
        return -1; // el archivo no existe
    }

    ifstream playlistFile(fullPath);
    vector<string> lines;
    string line;

    // Leer todas las líneas del archivo
    while (getline(playlistFile, line)) {
        lines.push_back(line);
    }
    playlistFile.close();

    // Abrir el archivo para reescribirlo
    ofstream outFile(fullPath);
    bool skipNextLine = false;

    for (const auto& currentLine : lines) {
        if (skipNextLine) {
            skipNextLine = false; // Saltar la línea de ruta de la canción
            continue;
        }

        if (currentLine.rfind("#EXTINF:", 0) == 0) {
            // Línea de metadata, verificar si la siguiente línea es la ruta que queremos eliminar
            size_t index = &currentLine - &lines[0]; // Obtener el índice actual
            if (index + 1 < lines.size() && lines[index + 1] == song_path) {
                skipNextLine = true; // Marcar para saltar la siguiente línea
                continue; // No escribir esta línea de metadata
            }
        }

        outFile << currentLine << "\n"; // Escribir la línea al archivo
    }

    outFile.close();

    return 0;

}
