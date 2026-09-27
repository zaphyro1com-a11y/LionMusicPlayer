#include "LrcReader.h"

#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

LrcReader::LrcReader(const std::string& _dir_path) {

    //° Se agrega un slash '\' al path para mayor simplicidad al cambiar de cancion °//
    if (!_dir_path.empty() && (_dir_path.back() == '\\' || _dir_path.back() == '/')) {
        dir_path = _dir_path;
    } else {
        dir_path = _dir_path + "\\" ;
    }

}

void LrcReader::ParseLrcFile() {

    //! Exepciones
    if (full_path.empty()) return;
    ifstream lrc_file(full_path);
    if (!lrc_file.is_open()) return;

    string line;
    static const std::regex timestamp_pattern(R"(\[(\d{2}):(\d{2})(?:\.(\d{2}))?\])");

    while (getline(lrc_file, line)) {

        // Puede haber mas de un timestamp por linea (ej. "[00:12.00][00:45.00]Coro"
        // repite el mismo texto en dos momentos), asi que se juntan todos primero
        std::vector<int> lineTimestamps;
        for (sregex_iterator it(line.begin(), line.end(), timestamp_pattern), rend; it != rend; ++it) {
            auto m = *it;
            int minutes = stoi(m[1].str());
            int seconds = stoi(m[2].str());
            int centiseconds = m[3].matched ? stoi(m[3].str()) : 0;
            int ms = (minutes * 60 * 1000) + (seconds * 1000) + (centiseconds * 10);
            lineTimestamps.push_back(ms);
        }

        string clean_line = regex_replace(line, timestamp_pattern, "");

        size_t start = clean_line.find_first_not_of(" \t\r\n");
        size_t last  = clean_line.find_last_not_of(" \t\r\n");
        if (start != string::npos && last != string::npos) {
            clean_line = clean_line.substr(start, last - start + 1);
        } else {
            clean_line.clear();
        }

        bool isMetadata = false;
        if (!clean_line.empty() && clean_line.front() == '[' && clean_line.back() == ']') {
            if (clean_line.find(':') != string::npos) {
                isMetadata = true;
            }
        }
        if (isMetadata) continue;

        if (!lineTimestamps.empty()) {
            // Una entrada en lrc_lines por CADA timestamp de la linea, para que
            // time_stamps y lrc_lines queden siempre alineados 1 a 1. Antes se
            // agregaba un timestamp por match pero el texto una sola vez (o ninguna,
            // si la linea era solo un timestamp de silencio/instrumental sin texto),
            // asi que time_stamps podia terminar mas largo que lrc_lines y
            // GetCurrentLine() indexaba lrc_lines fuera de rango.
            for (int ms : lineTimestamps) {
                time_stamps.push_back(ms);
                lrc_lines.push_back(clean_line); // vacio esta bien: linea en blanco durante el silencio
            }
        } else if (!clean_line.empty()) {
            // Linea sin timestamp: solo aporta al modo "no sincronizado"
            lrc_lines.push_back(clean_line);
        }
    }

    // Se considera "sincronizado" si se encontro al menos un timestamp real.
    // Se cachea aca en vez de releer el archivo en cada llamada a isLrcSynchronized()
    // (se invoca una vez por frame dibujado, desde el hilo de la UI).
    synced = !time_stamps.empty();

    lrc_file.close();
}

void LrcReader::ClearLyricsData(){

    song_name.clear();
    full_path.clear();
    time_stamps.clear();
    lrc_lines.clear();
    LastTimeStampMacthIndex= 0;
    synced= false;

}

int LrcReader::SetDirPath(const std::string& _new_dir_path){

    if (_new_dir_path.empty()) return -1;

    dir_path = _new_dir_path;
    ClearLyricsData();

    return 0;

}

string LrcReader::GetDirPath(){

    return dir_path;

}

string LrcReader::GetCurrentSongName(){

    return (song_name.empty())? "" : song_name;

}

string LrcReader::GetCurrentLyricsFullPath(){

    return (full_path.empty())? "" : full_path ;

}

string LrcReader::GetCurrentLine(int _elapsed_time_ms) {
    // lrc_lines.empty() queda de mas por el invariante que mantiene ParseLrcFile()
    // (lrc_lines.size() >= time_stamps.size() siempre), pero se deja como chequeo
    // defensivo: si algo rompe ese invariante a futuro, esto evita el out-of-range
    // en vez de crashear en silencio.
    if (time_stamps.empty() || lrc_lines.empty() || !IsValid()) return "";

    // Avanzar mientras el tiempo actual supere el siguiente timestamp
    while ((size_t)LastTimeStampMacthIndex + 1 < time_stamps.size() &&
        _elapsed_time_ms >= time_stamps[LastTimeStampMacthIndex + 1]) {
        LastTimeStampMacthIndex++;
    }

    // Retroceder si el tiempo actual está antes del timestamp actual
    while (LastTimeStampMacthIndex > 0 &&
        _elapsed_time_ms < time_stamps[LastTimeStampMacthIndex]) {
        LastTimeStampMacthIndex--;
    }

    return lrc_lines[LastTimeStampMacthIndex];
}

string LrcReader::GetUnsyncedLineByTime(int _elapsed_time_ms, int _song_duration_ms) {

    //Revisar que el archivo no este vacio
    // IsValid() ahora exige lrc_lines no vacio, asi que la division de abajo ya
    // no puede ser por cero (antes IsValid() solo miraba full_path, que quedaba
    // seteado aunque no se hubiera encontrado/parseado ningun archivo real, y esta
    // division reventaba con SIGFPE apenas la UI pedia la letra de una cancion sin .lrc)
    if (!IsValid() ) return "";

    // Calcular tiempo por linea
    int average_time = _song_duration_ms/lrc_lines.size();

    // llenar las marcas de tiempo artificialmente
    if (time_stamps.empty()){
        for (size_t i=0; i < lrc_lines.size(); i++){
            time_stamps.push_back(average_time * i);

        }
    }

    // Devolver la linea segun el tiempo promedio
    return GetCurrentLine(_elapsed_time_ms);

}

bool LrcReader::SearchLrcFile() {
    try {
        fs::path p = fs::u8path(full_path);
        if (fs::exists(p) && fs::is_regular_file(p)) {
            full_path = p.u8string();
            return true;
        }

        fs::path dir = fs::u8path(dir_path);
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            return false;
        }

        std::string target = song_name;
        //std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c){ return std::tolower(c); });

        fs::path candidate = fs::u8path(dir_path) / (song_name + ".lrc");
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            full_path = candidate.u8string();
            return true;
        }

    } catch (...) {
        return false;
    }

    return false;
}

bool LrcReader::isLrcSynchronized() const {
    // Antes volvia a abrir y recorrer full_path en cada llamada (se invoca una vez
    // por frame dibujado). ParseLrcFile() ya sabe si encontro timestamps al cargar
    // el archivo, asi que alcanza con devolver ese resultado cacheado.
    return synced;
}

int LrcReader::LoadLyrics(const std::string& _song_name){

    ClearLyricsData();

    song_name= _song_name;
    full_path= dir_path + _song_name + ".lrc";

    if ( !SearchLrcFile() ) {
        // No se encontro ningun archivo real
        full_path.clear();
        return -1;
    }

    ParseLrcFile();

    if (lrc_lines.empty()) {
        // El archivo existe pero no aporto ninguna linea utilizable (vacio, o
        // solo metadata) -> no es un archivo de letra valido
        ClearLyricsData();
        return -1;
    }

    return 0;

}