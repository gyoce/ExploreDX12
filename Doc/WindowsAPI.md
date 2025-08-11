# API Windows

[Page d'accueil](README.md)

Sommaire :
- [Généralités](#généralités)
- [Application Windows basique](#application-windows-basique)
- [Une meilleure boucle de message](#une-meilleure-boucle-de-message)
- [Mesure de temps](#mesure-de-temps)

## Généralités
Une application Windows suit le modèle de *Event-driven programming* (programmation basée sur les événements), ce qui signifie qu'elle attend des événements. Un événement peut être une touche de clavier, un clic de souris, quand la fenêtre est créée/redimensionnée/fermée, etc.

Quand un événement se produit, Windows (*l'OS*) envoie un message à l'application et ajoute ce message à la file de messages (*message queue*) de l'application (qui est une file avec priorité). L'application vérifie en permanence la file de messages et quand elle trouve un message, l'expédie à la fonction de traitement des messages. Les procedures de fenêtres (*Window Procedures*) sont des fonctions que l'on implémente qui contiennent le code pour traiter des messages spécifiques. 

## Application Windows basique
```cpp
#include <windows.h>

// Le "handle" de la fenêtre
HWND ghMainWnd = 0;

// Fonction qui permet d'initialiser la fenêtre, return true si succès, false sinon
bool InitWindowsApp(HINSTANCE instanceHandle, int show) 
{
    // On décrit les caractéristiques de la fenêtre avec la structure WNDCLASS
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instanceHandle;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"BasicWndClass";

    // Ensuite on doit inscrire la classe avec Windows pour pouvoir créer une fenêtre
    if (!RegisterClass(&wc))
    {
        MessageBox(0, L"RegisterClass FAILED", 0, 0);
        return false;
    }

    // On peut ensuite créer la fenêtre, si la création n'aboutit pas, le retour sera à 0, sinon on aura le handle de la fenêtre
    ghMainWnd = CreateWindow(
        L"BasicWncClass", // Notre classe inscrite précédemment
        L"Win32Basic", // Le titre de la fenêtre
        WS_OVERLAPPEDWINDOW, // Flag de style
        CW_USEDEFAULT, // Coordonnée x
        CW_USEDEFAULT, // Coordonnée y
        CW_USEDEFAULT, // Largeur
        CW_USEDEFAULT, // Hauteur
        0, // Fenêtre parente
        0, // Handle du menu
        instanceHandle, // Instance de l'app
        0 // Données supplémentaires
    );

    if (ghMainWnd == 0)
    {
        MessageBox(0, L"CreateWindow FAILED", 0, 0);
        return false;
    }

    // Initialement, la fenêtre n'est pas affichée, on doit donc le faire
    ShowWindow(ghMainWnd, show);
    UpdateWindow(ghMainWnd);
    return true;
}

// Fonction de boucle de message 
int Run()
{
    MSG msg = {0};
    BOOL bRet = 1;

    // Ici on boucle tant qu'on ne reçoit pas un message "WM_QUIT", car la fonction GetMessage() retourne 0 lorsque le message "WM_QUIT" est reçu. 
    // ATTENTION : la fonction GetMessage() met le thread de l'application en veille jusqu'à reception d'un message.
    while ( (bRet = GetMessage(&msg, 0, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            MessageBox(0, L"GetMessage FAILED", L"Error", MB_OK);
            break;
        }
        else 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

// La procédure de fenêtre qui gère les événements que notre fenêtre reçoit
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Ici on gère des messages spécifiques
    // ATTENTION : si on gère un message on doit alors retourner 0
    switch (msg)
    {

    // Si le clique gauche de la souris a été pressé
    case WM_LBUTTONDOWN:
        MessageBox(0, L"Hello World", L"Hello", MB_OK);
        return 0;

    // Si la touche échappe a été pressée
    case WM_KEYDOWN:
        if (wParam == VK_ESKCAPE)
            DestroyWindow(ghMainWnd);
        return 0;

    // Si on reçoit un message de destruction, alors on envoie un message de sortie qui terminera la boucle de message
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Pour tous les autres messages on les transfère 
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// L'équivalent Windows de la fonction main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
    if (!InitWindowsApp(hInstance, nShowCmd))
        return 0;
    return Run();
}
```

## Une meilleure boucle de message
Habituellement, les jeux n'attendent pas des événements mais sont constamment mis à jour. Dans le bout de code précédent cela pose problème car la fonction `GetMessage()` met le thread de l'application en veille jusqu'à réception d'un message. Le fix pour ce problème est assez simple, il faut utiliser la méthode `PeekMessage()` à la place de `GetMessage()`, s'il n'y a pas de messages, la fonction retourne de suite. La nouvelle boucle est donc : 
```cpp
int Run()
{
    MSG msg = {0};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else 
        {
            // Ici on peut mettre le code du jeu à jour    
        }
    }
    return (int)msg.wParam;
}
```

## Mesure de temps
Pour des mesures de temps on peut utiliser l'API Windows, plus particulièrement des *performance timer*. Le *timer* de performance mesure le temps en unités appelées *counts*.  On peut obtenir la valeur du temps actuel avec : 
```cpp
__int64 currentTime;
QueryPerformanceCounter((LARGE_INTEGER*)&currentTime);
```

Pour obtenir la fréquence (nombre de *count* par seconde) du *performance timer*, on peut utiliser la fonction suivante : 
```cpp
__int64 frequency;
QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
```

On peut alors calculer le nombre de seconds (ou fraction de seconde) par *count* avec : 
```cpp
double secondsPerCount = 1.0 / (double)frequency;
```

On peut donc calculer un nombre de *count* en secondes grâce à : 
```cpp
valueInSeconds = valueInCounts * secondsPerCount;
```

On peut donc calculer un temps écoulé entre deux instants grâce à :
```cpp
__int64 start = 0;
__int64 end = 0;
QueryPerformanceCounter((LARGE_INTEGER*)&start);
// ...
QueryPerformanceCounter((LARGE_INTEGER*)&end);
__int64 elapsed = end - start;
double elapsedInSeconds = elapsed * secondsPerCount;
```