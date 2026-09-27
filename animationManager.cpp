#include "animationManager.h"

#include <ncursesw/ncurses.h>  
#include <windows.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <iostream>  // Necesario para std::cerr y std::endl
#include <random>
#include <algorithm>

using namespace std;

// Devuelve la carpeta donde está el .exe (con \ al final), sin importar
// desde dónde se haya lanzado el programa. A diferencia de una ruta
// relativa como ".\animaciones\", esto NO depende del directorio de
// trabajo actual del proceso (que Windows puede asignar distinto según
// cómo se abra el programa: doble clic, acceso directo, otra terminal, etc.)
string getExeDir() {
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        // No se pudo obtener la ruta; usar carpeta actual como último recurso
        return ".\\";
    }

    string ruta(buffer, len);
    size_t pos = ruta.find_last_of("\\/");
    if (pos == string::npos) return ".\\";

    return ruta.substr(0, pos + 1); // incluye la barra final
}

AnimationManager::AnimationManager() {

    // revisar directorio
    initFiles();

    // comprobar animaciones (se revisa AnimationFiles directamente, no un contador
    // que todavia no se ha calculado)
    if (AnimationFiles.empty()) {
        AnimationsCount = 0;
        return;
    }

    //asignar numero de animaciones
    AnimationsCount = static_cast<int>(AnimationFiles.size());

    // inciar una animacion
    ChangeAnimation();

}

void AnimationManager::readFile() {
    
    ifstream file(AnimationFiles[AnimationIndex]);
    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << AnimationFiles[AnimationIndex] << endl;
        AnimationsCount = 0;
        return;
    }

    // Leer timming, numero de frames y tamaño x, y
    file >> Timming;
    file >> TotalFrames;
    file >> Size_x;
    file >> Size_y;

    if (file.fail() || TotalFrames <= 0 || Size_y <= 0) {
        cerr << "Formato invalido (se esperaban 4 numeros: timing, frames, ancho, alto) en: "
             << AnimationFiles[AnimationIndex] << endl;
        AnimationsCount = 0;
        return;
    }

    string line;
    getline(file, line); // limpiar salto de línea después del último número

    Frames.reserve(TotalFrames);

    // Leer cada frame
    for (int f = 0; f < TotalFrames; f++) {
        vector<string> frame(Size_x, string(Size_x, ' ')); // Size filas, rellenas de espacios por defecto
        bool finDeArchivo = false;

        for (int y = 0; y < Size_y; y++) {
            if (!getline(file, line)) { finDeArchivo = true; break; }

            // Copiar hasta "Size" caracteres (lo que falte queda como espacio)
            for (int x = 0; x < Size_x && x < (int)line.size(); x++) {
                frame[y][x] = line[x];
            }
        }

        Frames.push_back(frame);

        if (finDeArchivo) break; // archivo truncado: no seguir buscando frames que no existen
    }

    // Si el archivo estaba truncado, TotalFrames prometia mas frames de los que realmente hay
    TotalFrames = static_cast<int>(Frames.size());

    file.close();
    
}

void AnimationManager::initFiles() {
    
    WIN32_FIND_DATAA fileData; // version "A" explicita: no depende de si UNICODE esta definido
    HANDLE hFind;

    // Carpeta de animaciones SIEMPRE relativa a donde está el .exe,
    // nunca al directorio de trabajo actual
    string exeDir = getExeDir();
    string carpeta = exeDir + "animations\\*.*";
    string basePath = exeDir + "animations\\";  // prefijo para cada archivo

    vector<string> archivos;

    hFind = FindFirstFileA(carpeta.c_str(), &fileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        int count = 0;
        do {
            // Ignorar directorios
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                string nombre = fileData.cFileName;

                // Filtrar solo archivos .aaf
                if (nombre.size() >= 4 &&
                    nombre.substr(nombre.size() - 4) == ".aaf") {
                    archivos.push_back(basePath + nombre);
                    count++;
                }
            }
        } while (FindNextFileA(hFind, &fileData) && count < 10);
        FindClose(hFind);
    }

    if (archivos.empty()) {
        // Esto corre ANTES de iniciar ncurses, así que un cerr normal se ve bien
        cerr << "No se encontraron animaciones .aff en: " << basePath << endl;
        return;
    }

    AnimationFiles= archivos;

    // Devolver la lista de Nombres de animaciones

    string Name;
    size_t pos;

    for(int i=0; i < AnimationFiles.size(); i++){

        Name = AnimationFiles[i];
        pos = Name.find_last_of('\\');

        AnimationNames.push_back( Name.substr(pos+1) );
    }

}

int AnimationManager::ChangeAnimation() {

    if (AnimationsCount <= 0) return -1;

    // Lista de indices de animacion validos
    std::vector<int> validIndices;
    
    for (int i = 0; i < AnimationsCount; i++) {
        // No repetir animacion
        if (i == AnimationIndex) continue;

        // Verificar si el nombre de la animación actual está en KeyAnimations
        std::string currentName = AnimationNames[i];
        auto it = std::find(KeyAnimations.begin(), KeyAnimations.end(), currentName);
        
        // Si no se encuentra en KeyAnimations, es un índice válido
        if (it == KeyAnimations.end()) {
            validIndices.push_back(i);
        }
    }

    // Elegir aleatoriamente solo entre los índices válidos
    if (!validIndices.empty()) {
        std::random_device rd;  // semilla
        std::mt19937 gen(rd()); // generador
        std::uniform_int_distribution<size_t> dist(0, validIndices.size() - 1);
        
        AnimationIndex = validIndices[dist(gen)];
    } 
    // Si validIndices está vacío (es decir, solo hay 1 animación y es la actual, 
    // o todas las demás son KeyAnimations), AnimationIndex simplemente no cambia.

    // Limpiar propiedades
    cleanAnimationProperties();

    // Cargar los datos de la nueva animación
    readFile();

    return 0;
}

int AnimationManager::NextAnimation(){

    // Verificar la cantidad de animaciones 
    if (AnimationsCount > 2){
        AnimationIndex++;

        // Verificar que el indice de animacion este dentro del rango
        if(AnimationIndex >= AnimationsCount) AnimationIndex= 0; 

        // Limpiar Propiedades 
        cleanAnimationProperties();
        // Leer la nueva animacion
        readFile();

    } else return -1;

    return 0;

}

void AnimationManager::cleanAnimationProperties(){

    // limpiar Propiedades de la animacion
    Frames.clear();
    Timming= 100;
    TotalFrames= 0;
    Size_x= 0;
    Size_y= 0;
    CurrentFrame= 0;
    AnimationLooped= false;

}

bool AnimationManager::HasAnimations() const {
    return AnimationsCount > 0;
}

string AnimationManager::GetAnimationName() {

    if (AnimationFiles.empty() || AnimationIndex < 0 ||
        AnimationIndex >= (int)AnimationFiles.size()) {
        return "Sin animacion";
    }

    string Name = AnimationNames[AnimationIndex];
    size_t pos = Name.find_last_of('.');

    if (pos != string::npos) {
        return Name.substr(0, pos); // todo antes del último punto
    }
    
    return Name; // si no hay extensión
}

int AnimationManager::GetCurrentFrame() {

    return CurrentFrame;

}

void AnimationManager::NextFrame(){

    if (!HasAnimations()) return;

    // avanzar o repetir el bucle
    if (++CurrentFrame >= TotalFrames) 
    {
        AnimationLooped = true;
        Replay();
    }

}

void AnimationManager::Replay() {

    if(!HasAnimations()) return;

    CurrentFrame = 0;

}

char** AnimationManager::GetFrameToPrint() {

    char** AnimationFrame;

    for (int i = 0; i < Size_y; i++){

        // Copiar cada linea dentro de la matriz
        strcpy(AnimationFrame[i], Frames[CurrentFrame][i].c_str());

    }

    return AnimationFrame;

}

std::vector<std::string> AnimationManager::GetAnimationsList(){

    return AnimationFiles;

}

int AnimationManager::PlayAnimation(const std::string& animationFile) {

    if (AnimationNames.empty()){
        cerr << "Lista de Nombres de animaciones Vacios" << endl;

    }

    // Verificar que el archivo exista en la lista de animaciones
    auto it = std::find(AnimationNames.begin(), AnimationNames.end(), animationFile);
    if (it == AnimationNames.end()) {
        cerr << "Animacion "<< animationFile << " No Encontrada" << endl;
        return -1; // no encontrado
    }

    // Actualizar el índice de animación
    AnimationIndex = static_cast<int>(std::distance(AnimationNames.begin(), it));

    // Limpiar propiedades y leer la nueva animación
    cleanAnimationProperties();
    readFile();

    return 0;
}
