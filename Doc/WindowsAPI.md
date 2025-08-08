# API Windows

[Page d'accueil](README.md)

Sommaire :
- [Généralités](#généralités)

## Généralités
Une application Windows suit le modèle de *Event-driven programming* (programmation basée sur les événements), ce qui signifie qu'elle attend des événements. Un événement peut être une touche de clavier, un clic de souris, quand la fenêtre est créée/redimensionnée/fermée, etc.

Quand un événement se produit, Windows (*l'OS*) envoie un message à l'application et ajoute ce message à la file de messages (*message queue*) de l'application (qui est une file avec priorité). L'application vérifie en permanence la file de messages et quand elle trouve un message, l'expédie à la fonction de traitement des messages. Les procedures de fenêtres (*Window Procedures*) sont des fonctions que l'on implémente qui contiennent le code pour traiter des messages spécifiques. 