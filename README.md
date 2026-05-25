# Bomberman-CPP
Console Bomberman game developed in C++ using Windows API. Features singleplayer, local multiplayer, AI bot mode, enemies, bosses, powerups, ranking system, procedural maps, explosions, and gameplay music. Compatible with VS Code, CodeBlocks and MinGW g++.
# Bomberman-CPP 💣

Jogo estilo Bomberman feito em C++ utilizando Windows API.

O projeto possui:
- Singleplayer
- Multiplayer local
- Modo IA/Bot
- Inimigos e Boss
- Powerups
- Sistema de ranking
- Mapas procedurais
- Explosões
- Música durante gameplay

Compatível com:
- VS Code
- CodeBlocks
- MinGW g++

---

# 📦 Estrutura do Projeto

```text
Bomberman/
├── main.cpp
├── bomberman_music.wav
├── ranking.txt
└── Bomberman.exe
🎮 Controles
Jogador 1
W A S D → Movimentação
ESPAÇO → Colocar bomba
Jogador 2
SETAS → Movimentação
DELETE → Colocar bomba
Geral
ESC → Sair do jogo
🧠 Modos de Jogo
1 Jogador
2 Jogadores
Modo Bot/IA
⚡ Powerups
Powerup	Função
Fogo	Aumenta alcance da explosão
Bomba	Aumenta quantidade máxima de bombas
Vida	Vida extra
Relógio	Próxima bomba mais forte
Escudo	Protege contra dano
Fantasma	Atravessa paredes frágeis
🛠️ Requisitos
Windows
VS Code ou CodeBlocks
MinGW g++
▶️ Como Compilar

Abra a pasta do projeto no VS Code.

Abra o terminal:

Ctrl + '

Compile:

g++ "main.cpp" -o Bomberman.exe -lwinmm

Execute:

.\Bomberman.exe
🎵 Música

O jogo utiliza:

bomberman_music.wav

O arquivo de música deve permanecer na mesma pasta do:

main.cpp
📚 Bibliotecas Utilizadas
#include <windows.h>
#include <mmsystem.h>
#include <conio.h>
#include <vector>
#include <fstream>
#include <ctime>
💻 Compatibilidade

Compatível com:

VS Code

Windows
MinGW g++
🚀 Funcionalidades
Explosões em cruz
Sistema de colisão
IA de inimigos
IA do bot
Ranking salvo em arquivo
Fases progressivas
Boss fight
Sistema de pontuação
Música em loop
Geração aleatória de mapas
Powerups únicos
🎓 Projeto Acadêmico

Projeto desenvolvido para a disciplina de Algoritmos e Programação II em Ciência da Computação Univali
Alunos: Gabriel Debiasi, Vitor Alexandre Soares, Pedro Henrique Rondon.
