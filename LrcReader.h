#ifndef LRCREADER_H
#define LRCREADER_H

    //* Esta libreria proporciona un objeto LRCreader (lector de archivos .lrc)
    //* la clase identifica los datos de uno de estos archivos a la vez, 
    //* ademas puede cambiar de archivo en cualquier momento. La letra y marcas de tiempo
    //* son accesibles mediante las propiedades de la clase

#include <vector>
#include <string>

//* Clase LrcReader *//
class LrcReader {

    //~ Atributos ~//
    std::string dir_path;                   // Carpeta en la que se guardan las canciones y sus lyrics
    std::string song_name;                  // Nombre de la cancion actual 
    std::string full_path;                  // Direccion del archivo actual
    std::vector<int> time_stamps;           // Lista de marcas de tiempo de la cancion
    std::vector<std::string> lrc_lines;     // Lista de Lineas de la letra de la cancion
    int LastTimeStampMacthIndex= 0;         // Guarda el indice de la ultima coincidencia de marca de tiempo para reducir los bucles
    bool synced= false;                     // true si ParseLrcFile() encontro al menos un timestamp real.
                                             // Se calcula una sola vez al cargar el archivo en vez de
                                             // releerlo del disco en isLrcSynchronized() (se llama una
                                             // vez por frame dibujado).

    //~ Metodos Privados ~//
    void ParseLrcFile();                    // Obtiene los datos del archivo actual
    void ClearLyricsData();                 // Usar al cambiar de cancion 
    bool SearchLrcFile();                   // Devuelve verdadero si encuentra el archivo de letra actual

public:

    //° Constructor. Ademas agrega un slash al dir_path°//
    LrcReader() = default;
    LrcReader(const std::string& _dir_path);

    //~ Propiedades ~//
    // Cambia el directorio de las canciones y actualiza el resto de atributos
    int SetDirPath(const std::string& _new_dir_path);
    // Devuelve el path actual de la clase
    std::string GetDirPath();
    // Devuelve el nombre de la cancion actual
    std::string GetCurrentSongName();
    // Devuelve el archivo actual que se esta leyendo
    std::string GetCurrentLyricsFullPath();
    // Devuelve una linea de letra segun el tiempo (ms)
    std::string GetCurrentLine(int _elapsed_time_ms);
    // Devuelve una linea si el archivo no tiene marcas de tiempo
    std::string GetUnsyncedLineByTime(int _elapsed_time_ms, int _song_duration_ms);

    //~ Metodos Publicos ~//
    // Devuelve verdadero si el archivo de letra tiene marcas de tiempo
    bool isLrcSynchronized() const;
    // Carga un archivo .lrc y guarda sus datos dentro del objeto
    int LoadLyrics(const std::string& _song_name);
    //? indica si el objeto tiene un archivo valido CARGADO Y CON LINEAS UTILIZABLES ?//
    //? (antes miraba full_path, que LoadLyrics() asigna ANTES de saber si el archivo
    //? realmente existe/tiene contenido -> quedaba en true aunque no hubiera letra) ?//
    bool IsValid() const { return !lrc_lines.empty(); }

};

#endif