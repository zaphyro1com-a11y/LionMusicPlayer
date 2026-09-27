#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

    //* El AnimationManager se encarga de gestionar el directorio de animacion obteniendo cada animacion,
    //* ademas puede almacenar hasta una animacion (Frames, Timming, TotalFrames y Size) 
    //* en memoria para el display de animacion.

#include <vector>
#include <string>

class AnimationManager {

    std::vector<std::string> AnimationFiles;    // Lista de rutas de archivos de animación
    std::vector<std::string> AnimationNames;    // Lista de Animaciones (comparte orden con AnimationFiles)
    int AnimationsCount = 0;                    // Número de animaciones disponibles
    int AnimationIndex = 0;                     // Índice de la actual animación
    int CurrentFrame = 0;                       // Indice del frame actual
    bool AnimationLooped = false;               // Indica si la animacion se ha repetido

    //~ Animaciones Especiales ~
    // No aptas para el modo player
    std::vector<std::string> KeyAnimations = {"boot_animation.aaf", "transition_animation.aaf"};

    //  Lee el directorio de archivos de animacion y obtiene la lista de animaciones. 
    void initFiles();

    //  Lee un archivo de animacion y llena los datos en la clase.
    void readFile();

    // Limpia los valores de la clase para cambiar de animacion.
    void cleanAnimationProperties();

    // Devuelve el nombre de una animacion con extension
    void GetAnimationName(const std::string& animationFile);

    public:
    
        //~ Atributos Publicos :(
        // Frames[i] = un frame; cada frame tiene "Size" filas, cada una un string de "Size" caracteres
        std::vector<std::vector<std::string>> Frames;
        int Timming = 100;
        int TotalFrames = 0;
        int Size_x = 0;             // Tamaño en x de la animacion
        int Size_y = 0;             // Tamaño en y de la animacion

        //° Contructor, tambien inicializa una animacion en el objeto creado. °
        AnimationManager();

        //~ Metodos ~//
        // Cambia la animacion a una aleatoria de la lista de animaciones (evita repetir la actual si hay mas de una)
        int ChangeAnimation();
        // Cambia la animacion de forma lineal
        int NextAnimation();
        // Indica si hay al menos una animacion cargada correctamente
        bool HasAnimations() const;
        // Devuelve el nombre la animacion
        std::string GetAnimationName();
        // Devuelve Indice del frame actual
        int GetCurrentFrame();
        // Avanza al siguiente Frame ( CurrentFrame++ )
        void NextFrame();
        // Reinicia el bucle ( CurrentFrame = 0 )
        void Replay();
        // Devuelve una lista de strings lista para usar en funciones de C
        char** GetFrameToPrint();
        // devuelve la lista de archivos de animacion encontrados
        std::vector<std::string> GetAnimationsList();
        // Carga una animacion desde un archivo especifico (si existe)
        int PlayAnimation(const std::string& animationFile);
        // Devuelve el indice del frame actual
        int GetCurrentFrameIndex() const { return CurrentFrame; }
        // Devuelve true si la animacion actual se ha repetido al menos una vez
        bool HasAnimationLooped() const { return AnimationLooped; }

};


#endif //° ANIMATIONMANAGER_H