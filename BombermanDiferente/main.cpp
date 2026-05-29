#include <iostream>
#include <windows.h>
#include <conio.h>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

#define MAP_H 19
#define MAP_W 21

#define EMOJI_JOGADOR1      u8"🗿"
#define EMOJI_JOGADOR2      u8"🤖"
#define EMOJI_INIMIGO       u8"💀"
#define EMOJI_BOSS          u8"👺"
#define EMOJI_PAREDE_SOLIDA u8"🧱"
#define EMOJI_PAREDE_FRAGIL u8"❎"
#define EMOJI_BOMBA         u8"💣"
#define EMOJI_EXPLOSAO      u8"💥"
#define EMOJI_PORTAL        u8"🌀"
#define EMOJI_VAZIO         "  "

#define EMOJI_ITEM_FOGO     u8"🧨"
#define EMOJI_ITEM_BOMBA    u8"🎁"
#define EMOJI_ITEM_VIDA     u8"💚"
#define EMOJI_ITEM_RELOGIO  u8"⌚"
#define EMOJI_ITEM_ESCUDO   u8"🥽"
#define EMOJI_ITEM_FANTASMA u8"👻"


#define CELULA_VAZIO   0
#define CELULA_SOLIDA  1
#define CELULA_FRAGIL  2
#define CELULA_PORTAL  3

#define ITEM_NENHUM   0
#define ITEM_FOGO     1
#define ITEM_BOMBA    2
#define ITEM_VIDA     3
#define ITEM_RELOGIO  4
#define ITEM_ESCUDO   5
#define ITEM_FANTASMA 6

#define DONO_J1 1
#define DONO_J2 2

#define FRAMES_INIMIGO_FACIL   22
#define FRAMES_INIMIGO_MEDIO   14
#define FRAMES_INIMIGO_DIFICIL  9
#define FRAMES_INIMIGO_BOSS     5
#define FRAMES_EXPLOSAO         4
#define FRAMES_BOT               6

struct Jogador {
    int id = 1;
    int x, y;
    bool vivo = true;
    int vidas = 3;
    int raioFogo = 1;
    int maxBombas = 1;
    int movimentos = 0;
    int bombasUsadas = 0;
    int pontuacao = 0;
    int inimigosAbatidos = 0;
    int caixasDestruidas = 0;
    bool temEscudo = false;
    bool temFantasma = false;
    bool temRelogio = false;
    bool tomouDanoExplosao = false;
    string nome = "P1";
    int dificuldade = 1;
    int faseAtual = 1;
    int tempoJogo = 0;
};

struct Inimigo {
    int x, y;
    bool vivo = true;
    bool inteligente = false;
    int escudos = 0;       // boss nasce com 2, inimigos comuns ficam com 0
    bool tomouDano = false; // evita levar multiplos hits da mesma explosao
};

struct Bomba {
    int x, y;
    int raio;
    int contador;
    int dono;
    bool relogio;
    bool ativa = true;
};

struct Item {
    int x, y;
    int tipo = ITEM_NENHUM;
    bool visivel = true;
};

struct Registro {
    string nome;
    int pontos;
    string data;
};

template <typename T>
struct ListaEntidades {
    vector<T> itens;

    void adicionar(T obj) {
        itens.push_back(obj);
    }

    void limpar() {
        itens.clear();
    }

    int tamanho() const {
        return (int)itens.size();
    }

    T& operator[](int i) {
        return itens[i];
    }
};

int mapa[MAP_H][MAP_W];
bool areaExplosao[MAP_H][MAP_W];

bool explosaoAtiva = false;


#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Escreve um inteiro de 4 bytes no arquivo (little-endian)
void wavWrite4(ofstream& f, unsigned int v) {
    f.put((char)(v & 0xFF));
    f.put((char)((v >> 8) & 0xFF));
    f.put((char)((v >> 16) & 0xFF));
    f.put((char)((v >> 24) & 0xFF));
}

// Escreve um inteiro de 2 bytes no arquivo (little-endian)
void wavWrite2(ofstream& f, unsigned short v) {
    f.put((char)(v & 0xFF));
    f.put((char)((v >> 8) & 0xFF));
}

void gerarWav(const string& arquivo) {
    // Melodia: pares de {frequencia Hz, duracao ms} — 0 Hz = silencio
    int melodia[][2] = {
        {659,150},{659,150},{0,75},{659,150},{0,75},{523,150},{659,150},{0,75},
        {784,200},{0,200},{392,200},{0,200},
        {523,150},{0,100},{392,150},{0,100},{330,200},{0,200},
        {440,150},{0,75},{494,150},{0,75},{466,150},{0,75},{440,150},{0,75},
        {392,150},{659,100},{784,100},{880,150},{0,75},{698,150},
        {784,150},{0,75},{659,150},{0,75},{523,150},{587,100},{494,150},{0,150},
        {523,150},{0,100},{392,150},{0,100},{330,200},{0,400}
    };
    int totalNotas = sizeof(melodia) / sizeof(melodia[0]);

    int sampleRate = 22050;
    int totalSamples = 0;

    for (int i = 0; i < totalNotas; i++) {
        totalSamples += (melodia[i][1] * sampleRate) / 1000;
    }

    // Gera os samples PCM em memoria
    vector<unsigned char> samples(totalSamples);
    int pos = 0;

    for (int i = 0; i < totalNotas; i++) {
        int freq = melodia[i][0];
        int dur  = (melodia[i][1] * sampleRate) / 1000;

        for (int s = 0; s < dur; s++) {
            if (freq == 0) {
                samples[pos++] = 128; // silencio
            } else {
                // Onda quadrada 8-bit
                double t = (double)s / sampleRate;
                int periodo = sampleRate / freq;
                samples[pos++] = (s % periodo < periodo / 2) ? 200 : 55;
            }
        }
    }

    // Escreve o arquivo WAV
    ofstream f(arquivo, ios::binary);
    if (!f.is_open()) return;

    int dataSize   = totalSamples;
    int chunkSize  = 36 + dataSize;

    f.write("RIFF", 4);
    wavWrite4(f, chunkSize);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    wavWrite4(f, 16);          // tamanho do bloco fmt
    wavWrite2(f, 1);           // PCM
    wavWrite2(f, 1);           // mono
    wavWrite4(f, sampleRate);  // taxa de amostragem
    wavWrite4(f, sampleRate);  // byte rate
    wavWrite2(f, 1);           // block align
    wavWrite2(f, 8);           // bits por sample
    f.write("data", 4);
    wavWrite4(f, dataSize);
    f.write(reinterpret_cast<const char*>(samples.data()), dataSize);
    f.close();
}

void iniciarMusicaGameplay() {
    PlaySound(TEXT("PinkFloyd.wav"),
              NULL,
              SND_FILENAME | SND_ASYNC | SND_LOOP);
}

void pararMusica() {
    PlaySound(NULL, 0, 0);
}

int contadorExplosao = 0;

int frameInimigo = 0;
int frameBoss    = 0;
int frameExplosao = 0;
int frameBot = 0;
int itensGeradosNaFase = 0;

Jogador j1, j2;
bool doisJogadores = false;
bool modoBot = false;

ListaEntidades<Inimigo> inimigos;
ListaEntidades<Bomba> bombas;
ListaEntidades<Item> itens;

bool podeMoverPara(Jogador& j, int nx, int ny);
void colocarBomba(Jogador& j, int dono);
bool temBombaNaPos(int x, int y);
bool temInimigoNaPos(int x, int y, bool& boss);
bool temItemNaPos(int x, int y, int& tipoOut);
void rodarJogo();
bool todosInimigosMortos();
void moverBot(Jogador& j, int dono);
int bombasAtivasDoJogador(int dono);

void moverCursor(int x, int y) {
    COORD c;
    c.X = (SHORT)x;
    c.Y = (SHORT)y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void ocultarCursor() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(out, &ci);
    ci.bVisible = false;
    SetConsoleCursorInfo(out, &ci);
}

void limparAreaExplosaoArr() {
    for (int i = 0; i < MAP_H; i++) {
        for (int j = 0; j < MAP_W; j++) {
            areaExplosao[i][j] = false;
        }
    }

    j1.tomouDanoExplosao = false;
    j2.tomouDanoExplosao = false;

    for (int i = 0; i < inimigos.tamanho(); i++) {
        inimigos[i].tomouDano = false;
    }
}

void ordenarRanking(vector<Registro>& r, int n) {
    if (n <= 1) return;

    for (int i = 0; i < n - 1; i++) {
        if (r[i].pontos < r[i + 1].pontos) {
            Registro temp = r[i];
            r[i] = r[i + 1];
            r[i + 1] = temp;
        }
    }

    ordenarRanking(r, n - 1);
}

void calcularPontos(Jogador& j) {
    j.pontuacao = (j.inimigosAbatidos * 100)
                + (j.caixasDestruidas * 20)
                - (j.movimentos / 10)
                - (j.bombasUsadas * 5);

    if (j.pontuacao < 0) {
        j.pontuacao = 0;
    }
}

void calcularPontos(Jogador& j, int bonusFase) {
    calcularPontos(j);
    j.pontuacao += bonusFase;
}

void salvarRanking(const string& nome, int pontos) {
    vector<Registro> registros;

    ifstream entrada("ranking.txt");

    if (entrada.is_open()) {
        Registro reg;

        while (entrada >> reg.nome >> reg.pontos >> reg.data) {
            registros.push_back(reg);
        }

        entrada.close();
    }

    time_t agora = time(NULL);
    char buf[20];
    strftime(buf, sizeof(buf), "%d/%m/%Y", localtime(&agora));

    Registro novo;
    novo.nome = nome;
    novo.pontos = pontos;
    novo.data = string(buf);

    registros.push_back(novo);

    ordenarRanking(registros, (int)registros.size());

    ofstream saida("ranking.txt");

    int limite = (int)registros.size();
    if (limite > 10) limite = 10;

    for (int i = 0; i < limite; i++) {
        saida << registros[i].nome << " "
              << registros[i].pontos << " "
              << registros[i].data << "\n";
    }

    saida.close();
}

void garantirCaminhoRecursivo(int x, int y, bool visitado[MAP_H][MAP_W]) {
    if (x < 1 || x >= MAP_H - 1 || y < 1 || y >= MAP_W - 1) return;
    if (visitado[x][y]) return;

    visitado[x][y] = true;

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    for (int k = 0; k < 4; k++) {
        int nx = x + dx[k];
        int ny = y + dy[k];

        if (nx >= 1 && nx < MAP_H - 1 && ny >= 1 && ny < MAP_W - 1 && !visitado[nx][ny]) {
            if (mapa[nx][ny] == CELULA_SOLIDA && !(nx % 2 == 0 && ny % 2 == 0)) {
                mapa[nx][ny] = CELULA_VAZIO;
            }

            if (mapa[nx][ny] != CELULA_SOLIDA) {
                garantirCaminhoRecursivo(nx, ny, visitado);
            }
        }
    }
}

void gerarMapa(int fase, int dificuldade) {
    int pctFragil = 30 + (fase * 5);

    if (dificuldade == 2) pctFragil += 5;
    if (dificuldade == 3) pctFragil += 10;

    if (pctFragil > 55) pctFragil = 55;

    for (int i = 0; i < MAP_H; i++) {
        for (int j = 0; j < MAP_W; j++) {
            if (i == 0 || i == MAP_H - 1 || j == 0 || j == MAP_W - 1) {
                mapa[i][j] = CELULA_SOLIDA;
            }
            else if (i % 2 == 0 && j % 2 == 0) {
                mapa[i][j] = CELULA_SOLIDA;
            }
            else {
                int sorteio = rand() % 100;
                mapa[i][j] = (sorteio < pctFragil) ? CELULA_FRAGIL : CELULA_VAZIO;
            }
        }
    }

    mapa[1][1] = CELULA_VAZIO;
    mapa[1][2] = CELULA_VAZIO;
    mapa[2][1] = CELULA_VAZIO;

    mapa[MAP_H - 2][MAP_W - 2] = CELULA_VAZIO;
    mapa[MAP_H - 2][MAP_W - 3] = CELULA_VAZIO;
    mapa[MAP_H - 3][MAP_W - 2] = CELULA_VAZIO;

    mapa[1][MAP_W - 2] = CELULA_VAZIO;
    mapa[1][MAP_W - 3] = CELULA_VAZIO;
    mapa[2][MAP_W - 2] = CELULA_VAZIO;

    mapa[MAP_H - 2][1] = CELULA_VAZIO;
    mapa[MAP_H - 2][2] = CELULA_VAZIO;
    mapa[MAP_H - 3][1] = CELULA_VAZIO;

    mapa[MAP_H - 2][MAP_W / 2] = CELULA_PORTAL;

    bool visitado[MAP_H][MAP_W] = {};
    garantirCaminhoRecursivo(1, 1, visitado);
}

void inicializarInimigos(int fase, int dificuldade) {
    inimigos.limpar();

    int qtd;

    if (dificuldade == 1) qtd = 3;
    else if (dificuldade == 2) qtd = 5;
    else qtd = 7;

    if (fase == 3) {
        Inimigo boss;
        boss.vivo = true;
        boss.inteligente = true;
        boss.escudos = 2;

        // Procura uma posicao livre perto do centro para o boss nascer
        int cx = MAP_H / 2;
        int cy = MAP_W / 2;
        // Se o centro for solido fixo (linha e coluna pares), desloca uma casa
        if (cx % 2 == 0) cx++;
        if (cy % 2 == 0) cy++;

        boss.x = cx;
        boss.y = cy;

        // Limpa o spawn e vizinhos que NAO sejam paredes solidas fixas
        int limpX[] = {cx, cx-1, cx+1, cx, cx};
        int limpY[] = {cy, cy, cy, cy-1, cy+1};
        for (int k = 0; k < 5; k++) {
            int lx = limpX[k];
            int ly = limpY[k];
            if (lx < 1 || lx >= MAP_H - 1 || ly < 1 || ly >= MAP_W - 1) continue;
            if (lx % 2 == 0 && ly % 2 == 0) continue; // parede solida fixa, nao mexe
            if (mapa[lx][ly] != CELULA_PORTAL) {
                mapa[lx][ly] = CELULA_VAZIO;
            }
        }

        inimigos.adicionar(boss);
        qtd--;
    }

    int spawnX[] = {1, MAP_H - 2, 1, MAP_H / 2, MAP_H / 2, MAP_H - 2, 3};
    int spawnY[] = {MAP_W - 2, 1, MAP_W / 2, 1, MAP_W - 2, MAP_W / 2 - 2, MAP_W - 4};

    for (int i = 0; i < qtd && i < 7; i++) {
        Inimigo in;
        in.x = spawnX[i];
        in.y = spawnY[i];

        if (mapa[in.x][in.y] == CELULA_PORTAL) {
            in.y -= 2;
        }

        in.vivo = true;
        in.inteligente = false;

        if (mapa[in.x][in.y] != CELULA_PORTAL) {
            mapa[in.x][in.y] = CELULA_VAZIO;
        }

        inimigos.adicionar(in);
    }
}

bool temBombaNaPos(int x, int y) {
    for (int _bi = 0; _bi < bombas.tamanho(); _bi++) { Bomba& b = bombas[_bi];
        if (b.ativa && b.x == x && b.y == y) {
            return true;
        }
    }

    return false;
}

bool temInimigoNaPos(int x, int y, bool& boss) {
    for (int _ii = 0; _ii < inimigos.tamanho(); _ii++) { Inimigo& in = inimigos[_ii];
        if (in.vivo && in.x == x && in.y == y) {
            boss = in.inteligente;
            return true;
        }
    }

    return false;
}

bool temItemNaPos(int x, int y, int& tipoOut) {
    for (int _iti = 0; _iti < itens.tamanho(); _iti++) { Item& it = itens[_iti];
        if (it.visivel && it.x == x && it.y == y) {
            tipoOut = it.tipo;
            return true;
        }
    }

    return false;
}

string emojiItem(int tipo) {
    switch (tipo) {
        case ITEM_FOGO: return EMOJI_ITEM_FOGO;
        case ITEM_BOMBA: return EMOJI_ITEM_BOMBA;
        case ITEM_VIDA: return EMOJI_ITEM_VIDA;
        case ITEM_RELOGIO: return EMOJI_ITEM_RELOGIO;
        case ITEM_ESCUDO: return EMOJI_ITEM_ESCUDO;
        case ITEM_FANTASMA: return EMOJI_ITEM_FANTASMA;
        default: return EMOJI_VAZIO;
    }
}

void imprimirBuffs(const Jogador& j, const string& titulo) {
    cout << titulo;

    bool temAlgum = false;

    if (j.temEscudo) {
        cout << "[ESCUDO] ";
        temAlgum = true;
    }

    if (j.temFantasma) {
        cout << "[FANTASMA] ";
        temAlgum = true;
    }

    if (j.temRelogio) {
        cout << "[RELOGIO] ";
        temAlgum = true;
    }

    if (!temAlgum) {
        cout << "Nenhum";
    }

    cout << "                    \n";
}

bool perguntarReinicio(const string& mensagem) {
    moverCursor(0, MAP_H + 8);
    cout << mensagem << "\n";
    cout << "1 - Reiniciar jogo\n";
    cout << "2 - Voltar ao menu\n";
    cout << "Escolha: ";

    char op;

    do {
        op = getch();
    } while (op != '1' && op != '2');

    return op == '1';
}



bool botFugindo = false;
int botUltimaBombaX = -1;
int botUltimaBombaY = -1;

// Historico das ultimas posicoes do bot para detectar loop
#define HIST_TAM 8
int botHistX[HIST_TAM] = {-1,-1,-1,-1,-1,-1,-1,-1};
int botHistY[HIST_TAM] = {-1,-1,-1,-1,-1,-1,-1,-1};
int botHistIdx = 0;

bool explosaoDaBombaAlcancaPosicao(const Bomba& b, int x, int y) {
    if (!b.ativa) return false;

    if (b.x == x && b.y == y) return true;

    if (b.x == x && abs(b.y - y) <= b.raio) {
        int passo = (y > b.y) ? 1 : -1;
        for (int cy = b.y + passo; cy != y + passo; cy += passo) {
            if (mapa[x][cy] == CELULA_SOLIDA) return false;
            if (mapa[x][cy] == CELULA_FRAGIL && cy != y) return false;
        }
        return true;
    }

    if (b.y == y && abs(b.x - x) <= b.raio) {
        int passo = (x > b.x) ? 1 : -1;
        for (int cx = b.x + passo; cx != x + passo; cx += passo) {
            if (mapa[cx][y] == CELULA_SOLIDA) return false;
            if (mapa[cx][y] == CELULA_FRAGIL && cx != x) return false;
        }
        return true;
    }

    return false;
}

int distanciaInimigoMaisProximo(int x, int y) {
    int menor = 9999;
    for (int i = 0; i < inimigos.tamanho(); i++) {
        if (!inimigos[i].vivo) continue;
        int dist = abs(inimigos[i].x - x) + abs(inimigos[i].y - y);
        if (dist < menor) menor = dist;
    }
    return menor;
}


bool posicaoPerigosa(int x, int y) {
    if (x < 0 || x >= MAP_H || y < 0 || y >= MAP_W) return true;
    if (explosaoAtiva && areaExplosao[x][y]) return true;
    for (int i = 0; i < bombas.tamanho(); i++) {
        if (explosaoDaBombaAlcancaPosicao(bombas[i], x, y)) return true;
    }
    if (distanciaInimigoMaisProximo(x, y) <= 1) return true;
    return false;
}

bool botPodePisar(Jogador& j, int x, int y) {
    if (x < 0 || x >= MAP_H || y < 0 || y >= MAP_W) return false;
    if (temBombaNaPos(x, y)) return false;
    if (mapa[x][y] == CELULA_SOLIDA) return false;
    if (mapa[x][y] == CELULA_FRAGIL && !j.temFantasma) return false;
    return true;
}

bool posicaoSegura(Jogador& j, int x, int y) {
    if (!botPodePisar(j, x, y)) return false;
    return !posicaoPerigosa(x, y);
}

bool existeRotaDeFuga(Jogador& j) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    for (int k = 0; k < 4; k++) {
        int nx = j.x + dx[k];
        int ny = j.y + dy[k];
        if (posicaoSegura(j, nx, ny)) return true;

        for (int k2 = 0; k2 < 4; k2++) {
            int nx2 = nx + dx[k2];
            int ny2 = ny + dy[k2];
            if (botPodePisar(j, nx2, ny2) && !posicaoPerigosa(nx2, ny2)) return true;
        }
    }
    return false;
}

bool bombaAtingiriaAlgo(Jogador& j) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    for (int k = 0; k < 4; k++) {
        for (int r = 1; r <= j.raioFogo; r++) {
            int nx = j.x + dx[k] * r;
            int ny = j.y + dy[k] * r;
            if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) break;
            if (mapa[nx][ny] == CELULA_SOLIDA) break;
            if (mapa[nx][ny] == CELULA_FRAGIL) return true;
            bool boss = false;
            if (temInimigoNaPos(nx, ny, boss)) return true;
        }
    }
    return false;
}

void tentarFugir(Jogador& j) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    int melhor = -1;
    int melhorScore = -9999;

    for (int k = 0; k < 4; k++) {
        int nx = j.x + dx[k];
        int ny = j.y + dy[k];

        if (!botPodePisar(j, nx, ny)) continue;

        int perigoPenalidade = posicaoPerigosa(nx, ny) ? -50 : 0;

        int distBomba = 0;
        if (botUltimaBombaX >= 0) {
            distBomba = abs(nx - botUltimaBombaX) + abs(ny - botUltimaBombaY);
        }
        int distInimigo = distanciaInimigoMaisProximo(nx, ny);

        int score = distBomba * 6 + distInimigo * 4 + perigoPenalidade;

        if (score > melhorScore) {
            melhorScore = score;
            melhor = k;
        }
    }

    if (melhor != -1) {
        j.x += dx[melhor];
        j.y += dy[melhor];
        j.movimentos++;
    }
}

void registrarPosicaoBot(int x, int y) {
    botHistX[botHistIdx] = x;
    botHistY[botHistIdx] = y;
    botHistIdx = (botHistIdx + 1) % HIST_TAM;
}

int contarRepeticoesHist(int x, int y) {
    int count = 0;
    for (int i = 0; i < HIST_TAM; i++) {
        if (botHistX[i] == x && botHistY[i] == y) count++;
    }
    return count;
}

void moverEmDirecaoA(Jogador& j, int alvoX, int alvoY) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    int melhor = -1;
    int melhorScore = 9999;

    for (int k = 0; k < 4; k++) {
        int nx = j.x + dx[k];
        int ny = j.y + dy[k];

        if (!botPodePisar(j, nx, ny)) continue;
        if (posicaoPerigosa(nx, ny)) continue;

        int dist = abs(alvoX - nx) + abs(alvoY - ny);

        if (distanciaInimigoMaisProximo(nx, ny) <= 2) dist += 20;

        // Penaliza posicoes que aparecem muito no historico (evita loop)
        dist += contarRepeticoesHist(nx, ny) * 15;

        if (dist < melhorScore) {
            melhorScore = dist;
            melhor = k;
        }
    }

    if (melhor != -1) {
        j.x += dx[melhor];
        j.y += dy[melhor];
        j.movimentos++;
        registrarPosicaoBot(j.x, j.y);
    }
}

bool encontrarPosicaoAtaqueInimigo(Jogador& j, int& alvX, int& alvY) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    int melhorDist = 9999;
    bool achou = false;

    for (int i = 0; i < inimigos.tamanho(); i++) {
        if (!inimigos[i].vivo) continue;

        for (int k = 0; k < 4; k++) {
            int ax = inimigos[i].x + dx[k];
            int ay = inimigos[i].y + dy[k];
            if (ax < 0 || ax >= MAP_H || ay < 0 || ay >= MAP_W) continue;
            if (mapa[ax][ay] != CELULA_VAZIO && mapa[ax][ay] != CELULA_PORTAL) continue;
            int dist = abs(ax - j.x) + abs(ay - j.y);
            if (dist < melhorDist) {
                melhorDist = dist;
                alvX = ax;
                alvY = ay;
                achou = true;
            }
        }
    }
    return achou;
}

bool encontrarPosicaoAtaqueCaixa(Jogador& j, int& alvX, int& alvY) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    int melhorDist = 9999;
    bool achou = false;

    for (int i = 1; i < MAP_H - 1; i++) {
        for (int k = 1; k < MAP_W - 1; k++) {
            if (mapa[i][k] != CELULA_FRAGIL) continue;
            for (int d = 0; d < 4; d++) {
                int ax = i + dx[d];
                int ay = k + dy[d];
                if (ax < 0 || ax >= MAP_H || ay < 0 || ay >= MAP_W) continue;
                if (mapa[ax][ay] != CELULA_VAZIO) continue;
                int dist = abs(ax - j.x) + abs(ay - j.y);
                if (dist < melhorDist) {
                    melhorDist = dist;
                    alvX = ax;
                    alvY = ay;
                    achou = true;
                }
            }
        }
    }
    return achou;
}

bool encontrarItemMaisProximo(Jogador& j, int& alvX, int& alvY) {
    int melhorDist = 9999;
    bool achou = false;
    for (int i = 0; i < itens.tamanho(); i++) {
        if (!itens[i].visivel) continue;
        int dist = abs(itens[i].x - j.x) + abs(itens[i].y - j.y);
        if (dist < melhorDist && dist <= 6) {
            melhorDist = dist;
            alvX = itens[i].x;
            alvY = itens[i].y;
            achou = true;
        }
    }
    return achou;
}

void moverBot(Jogador& j, int dono) {
    if (!j.vivo) return;

    frameBot++;
    if (frameBot < FRAMES_BOT) return;
    frameBot = 0;

    bool aindaTemBomba = false;
    for (int i = 0; i < bombas.tamanho(); i++) {
        if (bombas[i].ativa && bombas[i].dono == dono) {
            aindaTemBomba = true;
            botUltimaBombaX = bombas[i].x;
            botUltimaBombaY = bombas[i].y;
            break;
        }
    }
    if (!aindaTemBomba) {
        botFugindo = false;
    }

    // PRIORIDADE 1: Fugir se estiver em perigo ou logo apos colocar bomba
    bool emPerigo = posicaoPerigosa(j.x, j.y);
    if (emPerigo || botFugindo) {
        tentarFugir(j);
        // So para de fugir quando estiver seguro E a bomba ja explodiu
        if (!emPerigo &&
            !posicaoPerigosa(j.x, j.y) &&
            !aindaTemBomba) {
            botFugindo = false;
        }
        return;
    }

    // PRIORIDADE 2: Plantar bomba se estiver em boa posição E tiver rota de fuga
    if (!temBombaNaPos(j.x, j.y) &&
        bombasAtivasDoJogador(dono) < j.maxBombas &&
        bombaAtingiriaAlgo(j) &&
        existeRotaDeFuga(j)) {

        colocarBomba(j, dono);
        botFugindo = true;
        botUltimaBombaX = j.x;
        botUltimaBombaY = j.y;
        tentarFugir(j);
        return;
    }

    // PRIORIDADE 3: Coletar item próximo
    int alvX, alvY;
    if (encontrarItemMaisProximo(j, alvX, alvY)) {
        moverEmDirecaoA(j, alvX, alvY);
        return;
    }

    // PRIORIDADE 4: Todos os inimigos mortos → vai ao portal
    if (todosInimigosMortos()) {
        moverEmDirecaoA(j, MAP_H - 2, MAP_W / 2);
        return;
    }

    // PRIORIDADE 5: Se já está na posição de ataque ao inimigo, planta bomba
    if (bombaAtingiriaAlgo(j) &&
        !temBombaNaPos(j.x, j.y) &&
        bombasAtivasDoJogador(dono) < j.maxBombas &&
        existeRotaDeFuga(j)) {

        colocarBomba(j, dono);
        botFugindo = true;
        botUltimaBombaX = j.x;
        botUltimaBombaY = j.y;
        tentarFugir(j);
        return;
    }

    // PRIORIDADE 6: Mover até posição adjacente ao inimigo mais próximo
    if (encontrarPosicaoAtaqueInimigo(j, alvX, alvY)) {
        moverEmDirecaoA(j, alvX, alvY);
        return;
    }

    // PRIORIDADE 7: Mover até caixa para destruir
    if (encontrarPosicaoAtaqueCaixa(j, alvX, alvY)) {
        moverEmDirecaoA(j, alvX, alvY);
    }
}


void tentarGerarItem(int x, int y) {
    if (rand() % 100 >= 35) return;

    bool jaExiste[7] = {};

    for (int _ie = 0; _ie < itens.tamanho(); _ie++) {
        Item& itemExistente = itens[_ie];
        if (itemExistente.visivel) {
            jaExiste[itemExistente.tipo] = true;
        }
    }

    vector<int> disponiveis;

    for (int t = 1; t <= 6; t++) {
        if (!jaExiste[t]) {
            disponiveis.push_back(t);
        }
    }

    if (!disponiveis.empty()) {
        Item it;
        it.x = x;
        it.y = y;
        it.tipo = disponiveis[rand() % disponiveis.size()];
        it.visivel = true;
        itens.adicionar(it);
    }
}

void destruirParedeFragil(int x, int y, Jogador& dono, bool destruirFrageis) {
    if (!destruirFrageis) return;
    if (mapa[x][y] != CELULA_FRAGIL) return;

    mapa[x][y] = CELULA_VAZIO;
    dono.caixasDestruidas++;
    tentarGerarItem(x, y);
}

struct ParedeParaDestruir {
    int x;
    int y;
};

void explodirBomba(int bx, int by, int raio, Jogador& dono, bool destruirFrageis = true) {
    limparAreaExplosaoArr();

    explosaoAtiva = true;
    contadorExplosao = 10;
    frameExplosao = 0;

    /*
        Regra classica do Bomberman:
        - a explosao anda em cruz;
        - parede solida bloqueia tudo;
        - parede fragil e destruida, mas BLOQUEIA a explosao;
        - se tiver duas paredes frageis em linha, so a primeira e destruida.

        Importante:
        Primeiro calculamos tudo usando mapaAntes.
        So depois destruimos as paredes.
        Assim nenhuma parede removida durante a explosao vira "caminho livre"
        para a propria explosao continuar.
    */
    int mapaAntes[MAP_H][MAP_W];
    for (int i = 0; i < MAP_H; i++) {
        for (int j = 0; j < MAP_W; j++) {
            mapaAntes[i][j] = mapa[i][j];
        }
    }

    vector<ParedeParaDestruir> paredesParaDestruir;

    areaExplosao[bx][by] = true;

    // Se a bomba foi colocada em cima de uma parede fragil usando fantasma,
    // destrói somente essa parede. Ela tambem funciona como bloqueio.
    if (mapaAntes[bx][by] == CELULA_FRAGIL) {
        ParedeParaDestruir p;
        p.x = bx;
        p.y = by;
        paredesParaDestruir.push_back(p);
    } else {
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};

        for (int k = 0; k < 4; k++) {
            for (int r = 1; r <= raio; r++) {
                int nx = bx + dx[k] * r;
                int ny = by + dy[k] * r;

                if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) {
                    break;
                }

                if (mapaAntes[nx][ny] == CELULA_SOLIDA) {
                    break;
                }

                if (mapaAntes[nx][ny] == CELULA_FRAGIL) {
                    areaExplosao[nx][ny] = true;

                    ParedeParaDestruir p;
                    p.x = nx;
                    p.y = ny;
                    paredesParaDestruir.push_back(p);

                    break; // PARA NA PRIMEIRA PAREDE FRAGIL DESSA DIRECAO
                }

                areaExplosao[nx][ny] = true;
            }
        }
    }

    for (int i = 0; i < (int)paredesParaDestruir.size(); i++) {
        destruirParedeFragil(paredesParaDestruir[i].x,
                             paredesParaDestruir[i].y,
                             dono,
                             destruirFrageis);
    }
}

void explodirBomba(int bx, int by, int raio, Jogador& dono, int multiplicador) {
    explodirBomba(bx, by, raio * multiplicador, dono, true);
}

void processarBombas() {
    for (int _bi = 0; _bi < bombas.tamanho(); _bi++) { Bomba& b = bombas[_bi];
        if (!b.ativa) continue;

        b.contador--;

        if (b.contador <= 0) {
            b.ativa = false;

            Jogador& dono = (b.dono == DONO_J1) ? j1 : j2;

            if (b.relogio) {
                explodirBomba(b.x, b.y, b.raio, dono, 2);
            } else {
                explodirBomba(b.x, b.y, b.raio, dono);
            }
        }
    }

    int totalBombas = 0;
    for (int i = 0; i < bombas.tamanho(); i++) {
        if (bombas.itens[i].ativa) {
            bombas.itens[totalBombas] = bombas.itens[i];
            totalBombas++;
        }
    }
    while (bombas.tamanho() > totalBombas) {
        bombas.itens.pop_back();
    }
}

void atualizarExplosao() {
    if (!explosaoAtiva) return;

    frameExplosao++;

    if (frameExplosao < FRAMES_EXPLOSAO) return;

    frameExplosao = 0;
    contadorExplosao--;

    if (contadorExplosao <= 0) {
        explosaoAtiva = false;
        limparAreaExplosaoArr();
    }
}

void causarDanoJogador(Jogador& j) {
    if (!j.vivo) return;
    if (!explosaoAtiva) return;
    if (!areaExplosao[j.x][j.y]) return;
    if (j.tomouDanoExplosao) return;

    j.tomouDanoExplosao = true;

    if (j.temEscudo) {
        j.temEscudo = false;
    } else {
        j.vidas--;

        if (j.vidas <= 0) {
            j.vivo = false;
        } else {
        if (j.id == 1) {
                j.x = 1;
                j.y = 1;
            } else {
                j.x = MAP_H - 2;
                j.y = MAP_W - 2;
            }
        }
    }
}

void aplicarDanoExplosao() {
    causarDanoJogador(j1);

    if (doisJogadores) {
        causarDanoJogador(j2);
    }

    for (int _ii = 0; _ii < inimigos.tamanho(); _ii++) { Inimigo& in = inimigos[_ii];
        if (!in.vivo) continue;
        if (!areaExplosao[in.x][in.y]) continue;
        if (in.tomouDano) continue;

        in.tomouDano = true;

        if (in.escudos > 0) {
            // Boss ainda tem escudo: absorve o dano, continua vivo
            in.escudos--;
        } else {
            // Sem escudo: morre
            in.vivo = false;

            int d1 = abs(in.x - j1.x) + abs(in.y - j1.y);
            int d2 = abs(in.x - j2.x) + abs(in.y - j2.y);

            if (doisJogadores && d2 < d1) {
                j2.inimigosAbatidos++;
            } else {
                j1.inimigosAbatidos++;
            }
        }
    }
}

void moverInimigos() {
    int dificuldade = j1.dificuldade;

    // --- Intervalo dos inimigos comuns por dificuldade ---
    int intervalo;
    if (dificuldade == 1) intervalo = FRAMES_INIMIGO_FACIL;
    else if (dificuldade == 2) intervalo = FRAMES_INIMIGO_MEDIO;
    else intervalo = FRAMES_INIMIGO_DIFICIL;

    // --- Intervalo do boss: mais rapido por dificuldade ---
    int intervaloBoss;
    if (dificuldade == 1) intervaloBoss = FRAMES_INIMIGO_BOSS + 4; // facil: 9
    else if (dificuldade == 2) intervaloBoss = FRAMES_INIMIGO_BOSS + 2; // medio: 7
    else intervaloBoss = FRAMES_INIMIGO_BOSS; // dificil: 5

    frameInimigo++;
    frameBoss++;

    bool moverComuns = (frameInimigo >= intervalo);
    bool moverBoss   = (frameBoss   >= intervaloBoss);

    if (!moverComuns && !moverBoss) return;

    if (moverComuns) frameInimigo = 0;
    if (moverBoss)   frameBoss    = 0;

    for (int _ii = 0; _ii < inimigos.tamanho(); _ii++) { Inimigo& in = inimigos[_ii];
        if (!in.vivo) continue;

        if (in.inteligente) {
            if (!moverBoss) continue;

            // --- Logica exclusiva do boss ---

            // Escolhe jogador vivo mais proximo por Manhattan
            int alvX = j1.x;
            int alvY = j1.y;

            if (doisJogadores) {
                if (!j1.vivo && j2.vivo) {
                    alvX = j2.x;
                    alvY = j2.y;
                } else if (j1.vivo && j2.vivo) {
                    int d1 = abs(in.x - j1.x) + abs(in.y - j1.y);
                    int d2 = abs(in.x - j2.x) + abs(in.y - j2.y);
                    if (d2 < d1) { alvX = j2.x; alvY = j2.y; }
                }
            }

            // Verifica se posicao atual esta no alcance de alguma bomba
            bool emPerigo = false;
            for (int bi = 0; bi < bombas.tamanho(); bi++) {
                if (explosaoDaBombaAlcancaPosicao(bombas[bi], in.x, in.y)) {
                    emPerigo = true;
                    break;
                }
            }

            int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

            if (emPerigo) {
                // Foge para casa segura mais afastada das bombas
                int melhor = -1;
                int melhorDist = -1;

                for (int k = 0; k < 4; k++) {
                    int nx = in.x + dirs[k][0];
                    int ny = in.y + dirs[k][1];

                    if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) continue;
                    if (mapa[nx][ny] == CELULA_SOLIDA) continue;
                    if (mapa[nx][ny] == CELULA_FRAGIL) continue;
                    if (temBombaNaPos(nx, ny)) continue;

                    bool seguro = true;
                    for (int bi = 0; bi < bombas.tamanho(); bi++) {
                        if (explosaoDaBombaAlcancaPosicao(bombas[bi], nx, ny)) {
                            seguro = false;
                            break;
                        }
                    }

                    int distBombas = 0;
                    for (int bi = 0; bi < bombas.tamanho(); bi++) {
                        if (bombas[bi].ativa) {
                            distBombas += abs(nx - bombas[bi].x) + abs(ny - bombas[bi].y);
                        }
                    }

                    if (seguro && distBombas > melhorDist) {
                        melhorDist = distBombas;
                        melhor = k;
                    }
                }

                // Se nao encontrou casa segura, qualquer casa livre serve
                if (melhor == -1) {
                    for (int k = 0; k < 4; k++) {
                        int nx = in.x + dirs[k][0];
                        int ny = in.y + dirs[k][1];
                        if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) continue;
                        if (mapa[nx][ny] == CELULA_SOLIDA) continue;
                        if (mapa[nx][ny] == CELULA_FRAGIL) continue;
                        if (temBombaNaPos(nx, ny)) continue;
                        melhor = k;
                        break;
                    }
                }

                if (melhor != -1) {
                    in.x += dirs[melhor][0];
                    in.y += dirs[melhor][1];
                }

            } else {
                // Perseguicao inteligente: tenta reduzir distancia Manhattan
                // Se o caminho direto esta bloqueado por fragil, tenta desvio lateral
                int melhor = -1;
                int melhorScore = 9999;

                for (int k = 0; k < 4; k++) {
                    int nx = in.x + dirs[k][0];
                    int ny = in.y + dirs[k][1];

                    if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) continue;
                    if (mapa[nx][ny] == CELULA_SOLIDA) continue;
                    if (mapa[nx][ny] == CELULA_FRAGIL) continue;
                    if (temBombaNaPos(nx, ny)) continue;

                    int dist = abs(nx - alvX) + abs(ny - alvY);

                    // Penaliza posicoes que ainda estao em linha de bomba
                    for (int bi = 0; bi < bombas.tamanho(); bi++) {
                        if (explosaoDaBombaAlcancaPosicao(bombas[bi], nx, ny)) {
                            dist += 20;
                            break;
                        }
                    }

                    if (dist < melhorScore) {
                        melhorScore = dist;
                        melhor = k;
                    }
                }

                // Se ainda assim travou, tenta qualquer casa livre (desvio de emergencia)
                if (melhor == -1) {
                    for (int k = 0; k < 4; k++) {
                        int nx = in.x + dirs[k][0];
                        int ny = in.y + dirs[k][1];
                        if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) continue;
                        if (mapa[nx][ny] == CELULA_SOLIDA) continue;
                        if (mapa[nx][ny] == CELULA_FRAGIL) continue;
                        if (temBombaNaPos(nx, ny)) continue;
                        melhor = k;
                        break;
                    }
                }

                if (melhor != -1) {
                    in.x += dirs[melhor][0];
                    in.y += dirs[melhor][1];
                }
            }

            continue; // boss processado
        }

        // --- Logica dos inimigos comuns ---
        if (!moverComuns) continue;

        int dx = 0;
        int dy = 0;

        int chancePerseguir;

        if (dificuldade == 1) chancePerseguir = 10;
        else if (dificuldade == 2) chancePerseguir = 50;
        else chancePerseguir = 75;

        Jogador alvo = j1;

        if (doisJogadores && j2.vivo) {
            int d1 = abs(in.x - j1.x) + abs(in.y - j1.y);
            int d2 = abs(in.x - j2.x) + abs(in.y - j2.y);

            if (d2 < d1) {
                alvo = j2;
            }
        }

        if (rand() % 100 < chancePerseguir) {
            if (in.x < alvo.x) dx = 1;
            else if (in.x > alvo.x) dx = -1;
            else if (in.y < alvo.y) dy = 1;
            else if (in.y > alvo.y) dy = -1;
        } else {
            int dir = rand() % 4;

            if (dir == 0) dx = -1;
            else if (dir == 1) dx = 1;
            else if (dir == 2) dy = -1;
            else dy = 1;
        }

        int nx = in.x + dx;
        int ny = in.y + dy;

        if (nx >= 0 && nx < MAP_H && ny >= 0 && ny < MAP_W) {
            bool bossIgnorado = false;
            if (mapa[nx][ny] != CELULA_SOLIDA &&
                mapa[nx][ny] != CELULA_FRAGIL &&
                !temBombaNaPos(nx, ny) &&
                !temInimigoNaPos(nx, ny, bossIgnorado)) {
                in.x = nx;
                in.y = ny;
            }
        }
    }
}

void verificarColisaoInimigos(Jogador& j) {
    if (!j.vivo) return;

    for (int _ii = 0; _ii < inimigos.tamanho(); _ii++) { Inimigo& in = inimigos[_ii];
        if (!in.vivo) continue;

        if (in.x == j.x && in.y == j.y) {
            if (j.temEscudo) {
                j.temEscudo = false;
                break; // escudo absorveu o golpe, para de checar outros inimigos
            } else {
                j.vidas--;

                if (j.vidas <= 0) {
                    j.vivo = false;
                } else {
                    if (j.id == 1) {
                        j.x = 1;
                        j.y = 1;
                    } else {
                        j.x = MAP_H - 2;
                        j.y = MAP_W - 2;
                    }
                }
                break; // ja tomou dano, nao precisa checar mais inimigos no mesmo frame
            }
        }
    }
}

void verificarItem(Jogador& j) {
    if (!j.vivo) return;

    for (int _iti = 0; _iti < itens.tamanho(); _iti++) { Item& it = itens[_iti];
        if (!it.visivel) continue;

        if (it.x == j.x && it.y == j.y) {
            it.visivel = false;

            switch (it.tipo) {
                case ITEM_FOGO:
                    j.raioFogo++;
                    break;

                case ITEM_BOMBA:
                    j.maxBombas++;
                    break;

                case ITEM_VIDA:
                    j.vidas++;
                    break;

                case ITEM_RELOGIO:
                    j.temRelogio = true;
                    break;

                case ITEM_ESCUDO:
                    j.temEscudo = true;
                    break;

                case ITEM_FANTASMA:
                    j.temFantasma = true;
                    break;
            }
        }
    }

    int totalItens = 0;
    for (int i = 0; i < itens.tamanho(); i++) {
        if (itens.itens[i].visivel) {
            itens.itens[totalItens] = itens.itens[i];
            totalItens++;
        }
    }
    while (itens.tamanho() > totalItens) {
        itens.itens.pop_back();
    }
}

void desenharJogo() {
    moverCursor(0, 0);

    cout << "P1: " << j1.nome
         << " | Vidas: " << j1.vidas
         << " | Tempo: " << j1.tempoJogo << "s"
         << " | Pontos: " << j1.pontuacao
         << " | Mov: " << j1.movimentos
         << " | Bombas usadas: " << j1.bombasUsadas
         << " | MaxBombas: " << j1.maxBombas
         << " | Fogo: " << j1.raioFogo
         << " | Fase: " << j1.faseAtual << "       \n";

    imprimirBuffs(j1, "Itens ativos P1: ");

    if (doisJogadores) {
        cout << "P2: " << j2.nome
             << " | Vidas: " << j2.vidas
             << " | Tempo: " << j2.tempoJogo << "s"
             << " | Pontos: " << j2.pontuacao
             << " | Mov: " << j2.movimentos
             << " | Bombas usadas: " << j2.bombasUsadas
             << " | MaxBombas: " << j2.maxBombas
             << " | Fogo: " << j2.raioFogo << "       \n";

        imprimirBuffs(j2, "Itens ativos P2: ");
    }

    for (int i = 0; i < MAP_H; i++) {
        for (int j = 0; j < MAP_W; j++) {
            if (explosaoAtiva && areaExplosao[i][j]) {
                cout << EMOJI_EXPLOSAO;
                continue;
            }

            if (j1.vivo && i == j1.x && j == j1.y) {
                cout << EMOJI_JOGADOR1;
                continue;
            }

            if (doisJogadores && j2.vivo && i == j2.x && j == j2.y) {
                cout << EMOJI_JOGADOR2;
                continue;
            }

            bool boss = false;

            if (temInimigoNaPos(i, j, boss)) {
                if (boss) cout << EMOJI_BOSS;
                else cout << EMOJI_INIMIGO;
                continue;
            }

            if (temBombaNaPos(i, j)) {
                cout << EMOJI_BOMBA;
                continue;
            }

            int tipoItem = ITEM_NENHUM;

            if (temItemNaPos(i, j, tipoItem)) {
                cout << emojiItem(tipoItem);
                continue;
            }

            if (mapa[i][j] == CELULA_SOLIDA) {
                cout << EMOJI_PAREDE_SOLIDA;
            } else if (mapa[i][j] == CELULA_FRAGIL) {
                cout << EMOJI_PAREDE_FRAGIL;
            } else if (mapa[i][j] == CELULA_PORTAL) {
                cout << EMOJI_PORTAL;
            } else {
                cout << EMOJI_VAZIO;
            }
        }

        cout << "\n";
    }

    cout << "P1: WASD move | ESPACO bomba | ESC sai        \n";

    if (doisJogadores) {
        cout << "P2: Setas move | DELETE bomba                 \n";
    } else {
        cout << "                                             \n";
    }
}

int bombasAtivasDoJogador(int dono) {
    int cont = 0;

    for (int _bi = 0; _bi < bombas.tamanho(); _bi++) { Bomba& b = bombas[_bi];
        if (b.ativa && b.dono == dono) {
            cont++;
        }
    }

    return cont;
}

void colocarBomba(Jogador& j, int dono) {
    if (!j.vivo) return;

    if (bombasAtivasDoJogador(dono) >= j.maxBombas) {
        return;
    }

    if (temBombaNaPos(j.x, j.y)) {
        return;
    }

    Bomba b;
    b.x = j.x;
    b.y = j.y;
    b.raio = j.raioFogo;
    b.dono = dono;
    b.ativa = true;

    if (j.temRelogio) {
        b.relogio = true;
        b.contador = 60;
        j.temRelogio = false;
    } else {
        b.relogio = false;
        b.contador = 120;
    }

    bombas.adicionar(b);
    j.bombasUsadas++;
}

bool podeMoverPara(Jogador& j, int nx, int ny) {
    if (nx < 0 || nx >= MAP_H || ny < 0 || ny >= MAP_W) {
        return false;
    }

    if (temBombaNaPos(nx, ny)) {
        return false;
    }

    if (mapa[nx][ny] == CELULA_VAZIO || mapa[nx][ny] == CELULA_PORTAL) {
        return true;
    }

    if (j.temFantasma && mapa[nx][ny] == CELULA_FRAGIL) {
        return true;
    }

    return false;
}

bool processarEntradaJogadores() {
    if (!_kbhit()) return false;

    char tecla = getch();

    if (tecla == 27) {
        return true;
    }

    if (tecla != 0 && tecla != (char)224) {
        int nx = j1.x;
        int ny = j1.y;
        bool moveu = false;

        switch (tecla) {
            case 'w':
            case 'W':
                nx--;
                moveu = true;
                break;

            case 's':
            case 'S':
                nx++;
                moveu = true;
                break;

            case 'a':
            case 'A':
                ny--;
                moveu = true;
                break;

            case 'd':
            case 'D':
                ny++;
                moveu = true;
                break;

            case ' ':
                colocarBomba(j1, DONO_J1);
                break;
        }

        if (moveu) {
            bool bateuNoJ2 = doisJogadores && j2.vivo && nx == j2.x && ny == j2.y;

            if (!bateuNoJ2 && podeMoverPara(j1, nx, ny)) {
                j1.x = nx;
                j1.y = ny;
                j1.movimentos++;
            }
        }

        return false;
    }

    char especial = getch();

    if (!doisJogadores || !j2.vivo) {
        return false;
    }

    int nx = j2.x;
    int ny = j2.y;
    bool moveu = false;

    switch (especial) {
        case 72:
            nx--;
            moveu = true;
            break;

        case 80:
            nx++;
            moveu = true;
            break;

        case 75:
            ny--;
            moveu = true;
            break;

        case 77:
            ny++;
            moveu = true;
            break;

        case 83:
            colocarBomba(j2, DONO_J2);
            break;
    }

    if (moveu) {
        bool bateuNoJ1 = j1.vivo && nx == j1.x && ny == j1.y;

        if (!bateuNoJ1 && podeMoverPara(j2, nx, ny)) {
            j2.x = nx;
            j2.y = ny;
            j2.movimentos++;
        }
    }

    return false;
}

bool todosInimigosMortos() {
    for (int _ii = 0; _ii < inimigos.tamanho(); _ii++) { Inimigo& in = inimigos[_ii];
        if (in.vivo) return false;
    }

    return true;
}

bool jogadorNoPortal(Jogador& j) {
    if (!j.vivo) return false;

    return mapa[j.x][j.y] == CELULA_PORTAL && todosInimigosMortos();
}

void inicializarFase(int fase, int dificuldade) {
    itens.limpar();
    bombas.limpar();
    limparAreaExplosaoArr();

    explosaoAtiva = false;
    contadorExplosao = 0;
    frameInimigo = 0;
    frameBoss    = 0;
    frameExplosao = 0;
    frameBot = 0;
    itensGeradosNaFase = 0;
    botFugindo = false;
    botUltimaBombaX = -1;
    botUltimaBombaY = -1;
    for (int i = 0; i < HIST_TAM; i++) { botHistX[i] = -1; botHistY[i] = -1; }
    botHistIdx = 0;

    gerarMapa(fase, dificuldade);
    inicializarInimigos(fase, dificuldade);

    j1.x = 1;
    j1.y = 1;
    j1.vivo = true;
    j1.temEscudo = false;
    j1.temFantasma = false;
    j1.temRelogio = false;
    j1.raioFogo = 1;
    j1.maxBombas = 1;
    j1.tomouDanoExplosao = false;

    if (doisJogadores) {
        j2.x = MAP_H - 2;
        j2.y = MAP_W - 2;
        j2.vivo = true;
        j2.temEscudo = false;
        j2.temFantasma = false;
        j2.temRelogio = false;
        j2.raioFogo = 1;
        j2.maxBombas = 1;
        j2.tomouDanoExplosao = false;
    }
}

void exibirRanking() {
    system("cls");

    cout << "======= RANKING TOP 10 =======\n\n";

    ifstream arq("ranking.txt");

    if (!arq.is_open()) {
        cout << "Nenhum registro encontrado.\n";
    } else {
        vector<Registro> lista;
        Registro r;

        while (arq >> r.nome >> r.pontos >> r.data) {
            lista.push_back(r);
        }

        arq.close();

        ordenarRanking(lista, (int)lista.size());

        int pos = 1;

        for (int _ri = 0; _ri < (int)lista.size(); _ri++) { Registro& reg = lista[_ri];
            cout << pos << ". " << reg.nome << " - " << reg.pontos << " pts (" << reg.data << ")\n";
            pos++;
        }
    }

    cout << "\nPressione qualquer tecla para voltar...";
    getch();
}

void exibirComoJogar() {
    system("cls");

    cout << "======= COMO JOGAR =======\n\n";
    cout << "Objetivo: elimine todos os inimigos e entre no portal.\n\n";
    cout << "P1: WASD para mover, ESPACO para bomba.\n";
    cout << "P2: SETAS para mover, DELETE para bomba.\n";
    cout << "Modo Bot: o computador controla o Jogador 1 sozinho.\n\n";
    cout << "Itens:\n";
    cout << "Fogo aumenta o raio da bomba.\n";
    cout << "Presente aumenta quantidade maxima de bombas.\n";
    cout << "Vida aumenta vidas.\n";
    cout << "Relogio deixa a proxima bomba com raio dobrado.\n";
    cout << "Escudo protege de um dano.\n";
    cout << "Fantasma atravessa uma parede fragil.\n\n";
    cout << "Pressione qualquer tecla para voltar...";
    getch();
}

void exibirDificuldades() {
    system("cls");

    cout << "======= DIFICULDADES =======\n\n";
    cout << "1. Facil: 3 inimigos, mais lentos.\n";
    cout << "2. Medio: 5 inimigos, perseguem mais.\n";
    cout << "3. Dificil: 7 inimigos, perseguem bastante.\n";
    cout << "Na fase 3 aparece um boss.\n\n";

    cout << "Pressione qualquer tecla para voltar...";
    getch();
}

void exibirPontuacao() {
    system("cls");

    cout << "======= PONTUACAO =======\n\n";
    cout << "+100 por inimigo morto\n";
    cout << "+20 por caixa destruida\n";
    cout << "-1 a cada 10 movimentos\n";
    cout << "-5 por bomba usada\n";
    cout << "+bonus ao passar de fase\n\n";

    cout << "Pressione qualquer tecla para voltar...";
    getch();
}

int menuPrincipal() {
    system("cls");

    cout << "==============================\n";
    cout << "          BOMBERMAN\n";
    cout << "==============================\n\n";
    cout << "1. Jogar 1 jogador\n";
    cout << "2. Jogar 2 jogadores\n";
    cout << "3. Computador jogar sozinho\n";
    cout << "4. Como jogar\n";
    cout << "5. Dificuldades\n";
    cout << "6. Pontuacao\n";
    cout << "7. Ranking\n";
    cout << "8. Sair\n\n";
    cout << "Escolha: ";

    int op;
    cin >> op;

    return op;
}

int menuDificuldade() {
    int d;

    while (true) {
        system("cls");

        cout << "======= DIFICULDADE =======\n\n";
        cout << "1. Facil\n";
        cout << "2. Medio\n";
        cout << "3. Dificil\n\n";
        cout << "Escolha: ";

        cin >> d;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');

            cout << "\nOpcao invalida! Digite apenas 1, 2 ou 3.";
            Sleep(1200);
            continue;
        }

        if (d >= 1 && d <= 3) {
            return d;
        }

        cout << "\nOpcao invalida! Escolha 1, 2 ou 3.";
        Sleep(1200);
    }
}

void salvarRankingFinal() {
    calcularPontos(j1);
    salvarRanking(j1.nome, j1.pontuacao);

    if (doisJogadores) {
        calcularPontos(j2);
        salvarRanking(j2.nome, j2.pontuacao);
    }
}

void rodarJogo() {
    int dificuldade = menuDificuldade();

    system("cls");

    cout << "Nome do Jogador 1: ";
    cin >> j1.nome;

    j1.vidas = 3;
    j1.id = 1;
    j1.dificuldade = dificuldade;
    j1.movimentos = 0;
    j1.bombasUsadas = 0;
    j1.inimigosAbatidos = 0;
    j1.caixasDestruidas = 0;
    j1.pontuacao = 0;

    if (doisJogadores) {
        cout << "Nome do Jogador 2: ";
        cin >> j2.nome;

        j2.vidas = 3;
        j2.id = 2;
        j2.dificuldade = dificuldade;
        j2.movimentos = 0;
        j2.bombasUsadas = 0;
        j2.inimigosAbatidos = 0;
        j2.caixasDestruidas = 0;
        j2.pontuacao = 0;
    }

    time_t inicio = time(NULL);

    int faseInicial = 1;
    if (j1.nome == "boss" || j1.nome == "BOSS") {
        faseInicial = 3;
    }

    for (int fase = faseInicial; fase <= 3; fase++) {
        j1.faseAtual = fase;
        j2.faseAtual = fase;

        inicializarFase(fase, dificuldade);

        system("cls");

        bool faseRodando = true;

        while (faseRodando) {
            j1.tempoJogo = (int)(time(NULL) - inicio);
            j2.tempoJogo = j1.tempoJogo;

            calcularPontos(j1);

            if (doisJogadores) {
                calcularPontos(j2);
            }

            if (!modoBot) {
                if (processarEntradaJogadores()) {
                    salvarRankingFinal();
                    return;
                }
            } else {
                moverBot(j1, DONO_J1);

                if (_kbhit()) {
                    char tecla = getch();

                    if (tecla == 27) {
                        salvarRankingFinal();
                            return;
                    }
                }
            }

            processarBombas();
            atualizarExplosao();
            aplicarDanoExplosao();
            moverInimigos();

            verificarColisaoInimigos(j1);
            verificarItem(j1);

            if (doisJogadores) {
                verificarColisaoInimigos(j2);
                verificarItem(j2);
            }

            desenharJogo();

            bool alguemVivo = j1.vivo || (doisJogadores && j2.vivo);

            if (!alguemVivo) {
                moverCursor(0, MAP_H + 5);
                salvarRankingFinal();

                if (perguntarReinicio("GAME OVER! Todos os jogadores morreram.")) {
                    rodarJogo();
                    return;
                }

                return;
            }

            if (jogadorNoPortal(j1) || (doisJogadores && jogadorNoPortal(j2))) {
                int bonus = 200 * fase;

                calcularPontos(j1, bonus);

                if (doisJogadores) {
                    calcularPontos(j2, bonus);
                }

                moverCursor(0, MAP_H + 5);
                cout << "Fase " << fase << " concluida! Bonus +" << bonus << " pontos.\n";
                cout << "Pressione qualquer tecla para continuar...";
                getch();

                faseRodando = false;
            }

            Sleep(6);
        }
    }

    system("cls");

    calcularPontos(j1, 500);

    cout << "======= FIM DE JOGO =======\n\n";
    cout << j1.nome << " terminou com " << j1.pontuacao << " pontos.\n";
    cout << "Inimigos abatidos: " << j1.inimigosAbatidos << "\n";
    cout << "Caixas destruidas: " << j1.caixasDestruidas << "\n";
    cout << "Movimentos: " << j1.movimentos << "\n";
    cout << "Bombas usadas: " << j1.bombasUsadas << "\n\n";

    salvarRanking(j1.nome, j1.pontuacao);

    if (doisJogadores) {
        calcularPontos(j2, 500);

        cout << j2.nome << " terminou com " << j2.pontuacao << " pontos.\n";
        cout << "Inimigos abatidos: " << j2.inimigosAbatidos << "\n";
        cout << "Caixas destruidas: " << j2.caixasDestruidas << "\n";
        cout << "Movimentos: " << j2.movimentos << "\n";
        cout << "Bombas usadas: " << j2.bombasUsadas << "\n\n";

        salvarRanking(j2.nome, j2.pontuacao);
    }

    if (perguntarReinicio("Fim de jogo! Deseja jogar novamente?")) {
        rodarJogo();
        return;
    }

}

int main() {
    srand((unsigned)time(NULL));

    SetConsoleOutputCP(CP_UTF8);
    ocultarCursor();

    iniciarMusicaGameplay();

    int op;

    do {
        op = menuPrincipal();

        switch (op) {
            case 1:
                doisJogadores = false;
                modoBot = false;
                rodarJogo();
                break;

            case 2:
                doisJogadores = true;
                modoBot = false;
                rodarJogo();
                break;

            case 3:
                doisJogadores = false;
                modoBot = true;
                rodarJogo();
                break;

            case 4:
                exibirComoJogar();
                break;

            case 5:
                exibirDificuldades();
                break;

            case 6:
                exibirPontuacao();
                break;

            case 7:
                exibirRanking();
                break;

            case 8:
                system("cls");
                cout << "Saindo...\n";
                break;

            default:
                break;
        }

    } while (op != 8);

    pararMusica();
    return 0;
}
