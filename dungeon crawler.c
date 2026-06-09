
/*
 * ═══════════════════════════════════════════════════════
 * DUNGEON OF SHADOWS — Dungeon Crawler (Projeto Acadêmico)
 * Implementado conforme lauda do projeto
 *
 * Compilar: gcc -o dungeon dungeon_crawler.c
 * Executar: ./dungeon
 * ═══════════════════════════════════════════════════════
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ─── Captura de Teclado (Cross-Platform) ──────────── */
#ifdef _WIN32
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
    char _getch(void) {
        struct termios oldt, newt;
        char ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif

/* ─── Dimensões dos mapas ───────────────────── */
#define VILA_W   10
#define VILA_H   10
#define F1_W     10
#define F1_H     10
#define F2_W     15
#define F2_H     15
#define F3_W     25
#define F3_H     25
#define MAX_W    25
#define MAX_H    25

/* ─── Limites de entidades ──────────────────── */
#define MAX_MONSTERS  20
#define MAX_KEYS       3
#define MAX_DOORS      3
#define MAX_BOXES     10
#define MAX_BUTTONS    3

/* ─── Armas ─────────────────────────────────── */
#define ARMA_NENHUMA  0
#define ARMA_ESPADA   1
#define ARMA_ARCO     2
#define ARMA_CAJADO   3

/* ─── Direções ──────────────────────────────── */
#define DIR_CIMA     0
#define DIR_DIREITA  1
#define DIR_BAIXO    2
#define DIR_ESQUERDA 3

/* ─── Fases ─────────────────────────────────── */
#define FASE_MENU     0
#define FASE_VILA     1
#define FASE_ANDAR1   2
#define FASE_ANDAR2   3
#define FASE_ANDAR3   4
#define FASE_VITORIA  5
#define FASE_GAMEOVER 6
#define FASE_TUTORIAL 7

/* ══════════════════════════════════════════════
   ESTRUTURAS
   ══════════════════════════════════════════════ */

typedef struct {
    int x, y;
    int direcao;
    int arma;
    int vidas;
    int falou_npc;
} Jogador;

typedef struct {
    int x, y;
    int tipo;        /* 1=X, 2=Y, 3=Z */
    int vivo;
    int boss_timer;
} Monstro;

typedef struct {
    int x, y;
    int aberta;
} Porta;

typedef struct {
    int x, y;
    int coletada;
} Chave;

typedef struct {
    int x, y;
    int destruida;
} Caixa;

typedef struct {
    int x, y;
    int ativado;
} Botao;

/* ══════════════════════════════════════════════
   ESTADO GLOBAL
   ══════════════════════════════════════════════ */

int     fase_atual;
int     mapa_w, mapa_h;
char    mapa[MAX_H][MAX_W];
char    mapa_base[MAX_H][MAX_W];

Jogador jogador;

Monstro monstros[MAX_MONSTERS];
int     n_monstros;

Porta   portas[MAX_DOORS];
int     n_portas;

Chave   chaves[MAX_KEYS];
int     n_chaves;

Caixa   caixas[MAX_BOXES];
int     n_caixas;

Botao   botoes[MAX_BUTTONS];
int     n_botoes;

int escada_x, escada_y;
int chaves_coletadas;
int escada_liberada;

/* ══════════════════════════════════════════════
   UTILITÁRIOS
   ══════════════════════════════════════════════ */

void limpar_tela(void) { printf("\033[2J\033[H"); }

void aguardar_tecla(void) {
    printf("\n  [ Pressione qualquer tecla para continuar... ]");
    _getch();
}

char direcao_char(void) {
    switch (jogador.direcao) {
        case DIR_CIMA:     return '^';
        case DIR_DIREITA:  return '>';
        case DIR_BAIXO:    return 'v';
        case DIR_ESQUERDA: return '<';
    }
    return '^';
}

int rand_range(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + rand() % (hi - lo + 1);
}

int abs_val(int x) { return x < 0 ? -x : x; }

/* ══════════════════════════════════════════════
   ACESSO AO MAPA
   ══════════════════════════════════════════════ */

char tile_em(int x, int y) {
    if (x < 0 || x >= mapa_w || y < 0 || y >= mapa_h) return '*';
    return mapa[y][x];
}

void set_tile(int x, int y, char t) {
    if (x < 0 || x >= mapa_w || y < 0 || y >= mapa_h) return;
    mapa[y][x] = t;
    mapa_base[y][x] = t;
}

int eh_solido(char t) {
    return (t == '*' || t == 'D' || t == 'k');
}

int monstro_em(int x, int y) {
    for (int i = 0; i < n_monstros; i++)
        if (monstros[i].vivo && monstros[i].x == x && monstros[i].y == y)
            return i;
    return -1;
}

/* ══════════════════════════════════════════════
   RENDERIZAÇÃO
   ══════════════════════════════════════════════ */

void imprimir_mapa(void) {
    char vis[MAX_H][MAX_W];
    for (int y = 0; y < mapa_h; y++)
        for (int x = 0; x < mapa_w; x++)
            vis[y][x] = mapa[y][x];

    for (int i = 0; i < n_monstros; i++) {
        if (!monstros[i].vivo) continue;
        char c = (monstros[i].tipo == 1) ? 'X' :
                 (monstros[i].tipo == 2) ? 'Y' : 'Z';
        vis[monstros[i].y][monstros[i].x] = c;
    }

    vis[jogador.y][jogador.x] = direcao_char();

    printf("\n");
    for (int y = 0; y < mapa_h; y++) {
        printf("  ");
        for (int x = 0; x < mapa_w; x++)
            printf("%c", vis[y][x]);
        printf("\n");
    }
}

void mostrar_hud(void) {
    const char *arma_nome[] = {"Nenhuma","Espada","Arco e Flecha","Cajado"};
    printf("\n  Vidas: ");
    for (int i = 0; i < jogador.vidas; i++) printf("* ");
    printf(" | Arma: %-14s", arma_nome[jogador.arma]);
    printf(" | Chaves: %d/%d\n", chaves_coletadas, n_chaves);
    printf("  [ W/A/S/D=mover  O=atacar  I=interagir  Q=menu ]\n");
}

/* ══════════════════════════════════════════════
   MENU PRINCIPAL
   ══════════════════════════════════════════════ */

void menu_principal(void) {
    while (1) {
        limpar_tela();
        printf("\n");
        printf("  +========================================+\n");
        printf("  |       DUNGEON  OF  SHADOWS             |\n");
        printf("  +========================================+\n");
        printf("  |  1. Jogar                              |\n");
        printf("  |  2. Tutorial                           |\n");
        printf("  |  3. Sair (Creditos)                    |\n");
        printf("  +========================================+\n");
        printf("\n  Escolha: ");

        char buf = _getch();

        if (buf == '1') { fase_atual = FASE_VILA;     return; }
        if (buf == '2') { fase_atual = FASE_TUTORIAL; return; }
        if (buf == '3') {
            limpar_tela();
            printf("\n  ====== CREDITOS ======\n\n");
            printf("  Desenvolvido por:\n");
            printf("    [Joao Gabriel Vitor Araujo]\n");
            printf("    [Laila Saraiva Nascimento]\n");
            printf("    [Gabriel dos Anjos Figueiredo]\n\n");
            printf("  Obrigado por jogar DUNGEON OF SHADOWS!\n\n");
            exit(0);
        }
    }
}

/* ══════════════════════════════════════════════
   TUTORIAL
   ══════════════════════════════════════════════ */

void tela_tutorial(void) {
    limpar_tela();
    printf("\n  ====== HISTORIA ======\n\n");
    printf("  Uma sombra antiga desperta nas profundezas da masmorra.\n");
    printf("  Voce, o ultimo heroi da vila, deve descer os tres andares\n");
    printf("  e derrotar o Lorde das Sombras antes que o mal consuma\n");
    printf("  o reino.\n\n");

    printf("  ====== SIMBOLOS ======\n\n");
    printf("  ^ < > v  Jogador (direcao em que olha)\n");
    printf("  * Parede (intransponivel)\n");
    printf("  #        Espinho (mata ao tocar!)\n");
    printf("  k        Caixa (destrutivel com ataque)\n");
    printf("  O        Botao (interaja com I)\n");
    printf("  D        Porta fechada\n");
    printf("  @        Chave (interaja com I para coletar)\n");
    printf("  =        Porta aberta\n");
    printf("  L        Escada (proxima fase)\n");
    printf("  X        Monstro Tipo 1 (movimento aleatorio)\n");
    printf("  Y        Monstro Tipo 2 (persegue voce)\n");
    printf("  Z        Boss Final\n");
    printf("  N        NPC (fale com I)\n");
    printf("  M        Entrada da masmorra\n\n");

    printf("  ====== CONTROLES ======\n\n");
    printf("  W/A/S/D  Mover (agora em tempo real!)\n");
    printf("  O        Atacar na direcao em que olha\n");
    printf("  I        Interagir com objeto a frente\n");
    printf("  Q        Voltar ao menu\n\n");

    printf("  ====== ARMAS ======\n\n");
    printf("  Espada        3x2 celulas a frente\n");
    printf("  Arco          4 celulas em linha reta\n");
    printf("  Cajado        8 celulas ao redor\n\n");

    printf("  ====== REGRAS ======\n\n");
    printf("  - Voce tem 3 vidas\n");
    printf("  - Espinhos e monstros tiram 1 vida (fase reinicia)\n");
    printf("  - 0 vidas = Game Over\n\n");

    aguardar_tecla();
    fase_atual = FASE_MENU;
}

/* ══════════════════════════════════════════════
   TELA DE VITORIA / GAME OVER
   ══════════════════════════════════════════════ */

void tela_vitoria(void) {
    limpar_tela();
    printf("\n");
    printf("  +==========================================+\n");
    printf("  |           VITORIA!!!                     |\n");
    printf("  +==========================================+\n");
    printf("  |  O Lorde das Sombras foi derrotado!      |\n");
    printf("  |                                          |\n");
    printf("  |  Com o boss caido, a maldicao se         |\n");
    printf("  |  desfez. A luz voltou a vila e o         |\n");
    printf("  |  reino celebrou o heroi que ousou        |\n");
    printf("  |  descer ate as profundezas.              |\n");
    printf("  |                                          |\n");
    printf("  |       Parabens, corajoso(a)!             |\n");
    printf("  +==========================================+\n\n");
    aguardar_tecla();
    fase_atual = FASE_MENU;
}

void tela_gameover(void) {
    limpar_tela();
    printf("\n");
    printf("  +======================================+\n");
    printf("  |           GAME  OVER                 |\n");
    printf("  |                                      |\n");
    printf("  |  Voce perdeu todas as vidas...       |\n");
    printf("  |  A escuridao venceu desta vez.       |\n");
    printf("  +======================================+\n\n");
    aguardar_tecla();
    fase_atual = FASE_MENU;
}

/* ══════════════════════════════════════════════
   NPC
   ══════════════════════════════════════════════ */

void dialogo_npc(void) {
    limpar_tela();
    printf("\n  ====== NPC: Ferreiro Aldric ======\n\n");
    printf("  \"Heroi, a masmorra e perigosa. Escolha sua arma:\"\n\n");
    printf("  1. Espada       - ataca 3x2 celulas a frente\n");
    printf("  2. Arco         - ataca 4 celulas em linha reta\n");
    printf("  3. Cajado       - ataca as 8 celulas ao redor\n\n");
    printf("  Sua escolha: ");

    char buf;
    while (1) {
        buf = _getch();
        if (buf == '1') { jogador.arma = ARMA_ESPADA; break; }
        if (buf == '2') { jogador.arma = ARMA_ARCO;   break; }
        if (buf == '3') { jogador.arma = ARMA_CAJADO; break; }
    }
    const char *nomes[] = {"","Espada","Arco e Flecha","Cajado"};
    printf("%c\n\n  \"Boa sorte com a %s!\"\n", buf, nomes[jogador.arma]);
    jogador.falou_npc = 1;
    aguardar_tecla();
}

/* ══════════════════════════════════════════════
   DEFINIÇÃO DOS MAPAS
   ══════════════════════════════════════════════ */

/* VILA 10x10 */
static const char *MAPA_VILA[VILA_H] = {
    "**********",
    "*........*",
    "*..N.....*",
    "*........*",
    "*........*",
    "*........*",
    "*........*",
    "*........*",
    "*....M...*",
    "**********"
};

/* ANDAR 1 — 10x10: chave, caixas bloqueando, porta, escada */
static const char *MAPA_ANDAR1[F1_H] = {
    "**********",
    "*........*",
    "*..@.....*",
    "*..kk....*",
    "*........*",
    "*...D....*",
    "*........*",
    "*........*",
    "*.......L*",
    "**********"
};

/* ANDAR 2 — 15x15: espinhos, 2 chaves, 2 portas, 1 botão, monstro X */
static const char *MAPA_ANDAR2[F2_H] = {
    "***************",
    "*....@........*",
    "*..###........*",
    "*.............*",
    "*.....D.......*",
    "*.............*",
    "*...O.........*",
    "*.............*",
    "*.....@.......*",
    "*.............*",
    "*......D......*",
    "*..###........*",
    "*.............*",
    "*...........L.*",
    "***************"
};

/* ANDAR 3 — 25x25: 3 chaves, 3 portas, monstros Y, boss Z */
static const char *MAPA_ANDAR3[F3_H] = {
    "*************************",
    "*....@...................*",
    "*........................*",
    "*...***......***.........*",
    "*...*.D......D.*.........*",
    "*...*..........*.....@...*",
    "*........................*",
    "*....###.....###.........*",
    "*........................*",
    "*........................*",
    "*............Z...........*",
    "*........................*",
    "*........................*",
    "*....###.....###.........*",
    "*........................*",
    "*...*...............*....*",
    "*...*.D............D.*...*",
    "*...*..............*.@...*",
    "*...***............***...*",
    "*........................*",
    "*........................*",
    "*........................*",
    "*........................*",
    "*........................*",
    "*************************"
};

/* ══════════════════════════════════════════════
   CARREGAMENTO DE FASE
   ══════════════════════════════════════════════ */

void carregar_mapa_str(const char **linhas, int h, int w) {
    mapa_h = h; mapa_w = w;
    n_monstros = 0; n_portas = 0; n_chaves = 0;
    n_caixas = 0; n_botoes = 0;
    chaves_coletadas = 0;
    escada_liberada  = 0;
    escada_x = -1; escada_y = -1;

    for (int y = 0; y < h; y++) {
        int len = (int)strlen(linhas[y]);
        for (int x = 0; x < w; x++) {
            char c = (x < len) ? linhas[y][x] : ' ';

            if (c == 'X' || c == 'Y' || c == 'Z') {
                if (n_monstros < MAX_MONSTERS) {
                    monstros[n_monstros].x = x;
                    monstros[n_monstros].y = y;
                    monstros[n_monstros].tipo = (c=='X') ? 1 : (c=='Y') ? 2 : 3;
                    monstros[n_monstros].vivo = 1;
                    monstros[n_monstros].boss_timer = 0;
                    n_monstros++;
                }
                c = '.';
            }
            if (c == 'D') {
                if (n_portas < MAX_DOORS) {
                    portas[n_portas].x = x; portas[n_portas].y = y;
                    portas[n_portas].aberta = 0; n_portas++;
                }
            }
            if (c == '@') {
                if (n_chaves < MAX_KEYS) {
                    chaves[n_chaves].x = x; chaves[n_chaves].y = y;
                    chaves[n_chaves].coletada = 0; n_chaves++;
                }
            }
            if (c == 'k') {
                if (n_caixas < MAX_BOXES) {
                    caixas[n_caixas].x = x; caixas[n_caixas].y = y;
                    caixas[n_caixas].destruida = 0; n_caixas++;
                }
            }
            if (c == 'O') {
                if (n_botoes < MAX_BUTTONS) {
                    botoes[n_botoes].x = x; botoes[n_botoes].y = y;
                    botoes[n_botoes].ativado = 0; n_botoes++;
                }
            }
            if (c == 'L') { escada_x = x; escada_y = y; }

            mapa[y][x] = c;
            mapa_base[y][x] = c;
        }
    }

    if (fase_atual == FASE_ANDAR3 && escada_x >= 0) {
        mapa[escada_y][escada_x] = '.';
        mapa_base[escada_y][escada_x] = '.';
        escada_liberada = 0;
    }
}

void iniciar_vila(void) {
    fase_atual = FASE_VILA;
    carregar_mapa_str(MAPA_VILA, VILA_H, VILA_W);
    jogador.x = 3; jogador.y = 4;
    jogador.direcao = DIR_BAIXO;
    escada_x = 5; escada_y = 8;
    escada_liberada = 1;
}

void iniciar_andar1(void) {
    fase_atual = FASE_ANDAR1;
    carregar_mapa_str(MAPA_ANDAR1, F1_H, F1_W);
    jogador.x = 2; jogador.y = 1;
    jogador.direcao = DIR_BAIXO;
    escada_liberada = 0;
}

void iniciar_andar2(void) {
    fase_atual = FASE_ANDAR2;
    carregar_mapa_str(MAPA_ANDAR2, F2_H, F2_W);
    jogador.x = 2; jogador.y = 1;
    jogador.direcao = DIR_BAIXO;
    if (n_monstros < MAX_MONSTERS) {
        monstros[n_monstros].x = 9; monstros[n_monstros].y = 4;
        monstros[n_monstros].tipo = 1; monstros[n_monstros].vivo = 1;
        monstros[n_monstros].boss_timer = 0; n_monstros++;
    }
    escada_liberada = 0;
}

void iniciar_andar3(void) {
    fase_atual = FASE_ANDAR3;
    carregar_mapa_str(MAPA_ANDAR3, F3_H, F3_W);
    jogador.x = 2; jogador.y = 1;
    jogador.direcao = DIR_BAIXO;
    int pos[][2] = {{3,7},{20,7},{3,15},{20,15}};
    for (int i = 0; i < 4 && n_monstros < MAX_MONSTERS; i++) {
        if (tile_em(pos[i][0], pos[i][1]) != '*') {
            monstros[n_monstros].x = pos[i][0];
            monstros[n_monstros].y = pos[i][1];
            monstros[n_monstros].tipo = 2;
            monstros[n_monstros].vivo = 1;
            monstros[n_monstros].boss_timer = 0;
            n_monstros++;
        }
    }
    escada_liberada = 0;
}

/* ══════════════════════════════════════════════
   REINÍCIO DE FASE
   ══════════════════════════════════════════════ */

void reiniciar_fase(void) {
    int arma  = jogador.arma;
    int vidas = jogador.vidas;
    int falou = jogador.falou_npc;
    int fase  = fase_atual;

    switch (fase) {
        case FASE_VILA:   iniciar_vila();   break;
        case FASE_ANDAR1: iniciar_andar1(); break;
        case FASE_ANDAR2: iniciar_andar2(); break;
        case FASE_ANDAR3: iniciar_andar3(); break;
        default: break;
    }
    jogador.arma = arma; jogador.vidas = vidas; jogador.falou_npc = falou;
}

/* ══════════════════════════════════════════════
   VERIFICAÇÃO DE MORTE
   ══════════════════════════════════════════════ */

void verificar_espinho(int x, int y) {
    if (fase_atual == FASE_GAMEOVER) return;
    if (tile_em(x, y) == '#') {
        printf("\n  Voce pisou em um espinho! Perdeu uma vida.\n");
        aguardar_tecla();
        jogador.vidas--;
        if (jogador.vidas <= 0) { fase_atual = FASE_GAMEOVER; return; }
        reiniciar_fase();
    }
}

void verificar_contato_monstros(void) {
    if (fase_atual == FASE_GAMEOVER) return;
    for (int i = 0; i < n_monstros; i++) {
        if (!monstros[i].vivo) continue;
        if (monstros[i].x == jogador.x && monstros[i].y == jogador.y) {
            printf("\n  Um monstro tocou em voce! Perdeu uma vida.\n");
            aguardar_tecla();
            jogador.vidas--;
            if (jogador.vidas <= 0) { fase_atual = FASE_GAMEOVER; return; }
            reiniciar_fase();
            return;
        }
    }
}

/* ══════════════════════════════════════════════
   ATAQUE
   ══════════════════════════════════════════════ */

void aplicar_ataque_cell(int x, int y) {
    for (int i = 0; i < n_caixas; i++) {
        if (!caixas[i].destruida && caixas[i].x == x && caixas[i].y == y) {
            caixas[i].destruida = 1;
            mapa[y][x] = '.'; mapa_base[y][x] = '.';
        }
    }
    int mi = monstro_em(x, y);
    if (mi >= 0) { monstros[mi].vivo = 0; }
}

void verificar_vitoria_boss(void) {
    if (fase_atual != FASE_ANDAR3) return;
    for (int i = 0; i < n_monstros; i++)
        if (monstros[i].vivo && monstros[i].tipo == 3) return;
    if (!escada_liberada) {
        escada_liberada = 1;
        int sx = mapa_w / 2, sy = mapa_h - 2;
        escada_x = sx; escada_y = sy;
        mapa[sy][sx] = 'L'; mapa_base[sy][sx] = 'L';
        printf("\n  O Boss foi derrotado! Uma escada apareceu!\n");
        aguardar_tecla();
    }
}

void atacar(void) {
    if (jogador.arma == ARMA_NENHUMA) {
        printf("  Voce nao tem arma! Fale com o NPC primeiro.\n");
        aguardar_tecla(); return;
    }
    int px = jogador.x, py = jogador.y, dir = jogador.direcao;
    int fx=0,fy=0,lx=0,ly=0;
    
    switch (dir) {
        case DIR_CIMA:     fy=-1; lx=1; break;
        case DIR_BAIXO:    fy= 1; lx=1; break;
        case DIR_ESQUERDA: fx=-1; ly=1; break;
        case DIR_DIREITA:  fx= 1; ly=1; break;
    }

    switch (jogador.arma) {
        case ARMA_ESPADA:
            for (int d=1; d<=2; d++)
                for (int s=-1; s<=1; s++)
                    aplicar_ataque_cell(px+fx*d+lx*s, py+fy*d+ly*s);
            break;
        case ARMA_ARCO:
            for (int d=1; d<=4; d++) {
                int ax=px+fx*d, ay=py+fy*d;
                aplicar_ataque_cell(ax, ay);
                if (tile_em(ax,ay) == '*') break;
            }
            break;
        case ARMA_CAJADO:
            for (int dy=-1; dy<=1; dy++)
                for (int dx=-1; dx<=1; dx++)
                    if (dx!=0||dy!=0)
                        aplicar_ataque_cell(px+dx, py+dy);
            break;
    }
    verificar_vitoria_boss();
}

/* ══════════════════════════════════════════════
   INTERAÇÃO
   ══════════════════════════════════════════════ */

void interagir(void) {
    int ix=jogador.x, iy=jogador.y;
    switch (jogador.direcao) {
        case DIR_CIMA:     iy--; break;
        case DIR_BAIXO:    iy++; break;
        case DIR_ESQUERDA: ix--; break;
        case DIR_DIREITA:  ix++; break;
    }
    char t = tile_em(ix, iy);

    if (t == 'N') { dialogo_npc(); return; }

    if (t == '@') {
        for (int i=0; i<n_chaves; i++) {
            if (!chaves[i].coletada && chaves[i].x==ix && chaves[i].y==iy) {
                chaves[i].coletada = 1;
                mapa[iy][ix] = '.'; mapa_base[iy][ix] = '.';
                chaves_coletadas++;
                printf("  Voce coletou uma chave! (%d/%d)\n", chaves_coletadas, n_chaves);
                aguardar_tecla(); return;
            }
        }
    }

    if (t == 'D') {
        if (chaves_coletadas > 0) {
            for (int i=0; i<n_portas; i++) {
                if (!portas[i].aberta && portas[i].x==ix && portas[i].y==iy) {
                    portas[i].aberta = 1;
                    mapa[iy][ix] = '='; mapa_base[iy][ix] = '=';
                    chaves_coletadas--;
                    printf("  Porta aberta!\n");
                    
                    if (fase_atual == FASE_ANDAR1 || fase_atual == FASE_ANDAR2) {
                        int todas = 1;
                        for (int j=0; j<n_portas; j++)
                            if (!portas[j].aberta) todas=0;
                        if (todas) {
                            escada_liberada = 1;
                            printf("  A escada esta acessivel!\n");
                        }
                    }
                    aguardar_tecla(); return;
                }
            }
        } else {
            printf("  Voce precisa de uma chave.\n");
            aguardar_tecla();
        }
        return;
    }

    if (t == 'O') {
        for (int i=0; i<n_botoes; i++) {
            if (!botoes[i].ativado && botoes[i].x==ix && botoes[i].y==iy) {
                botoes[i].ativado = 1;
                printf("  *CLIQUE* Botao ativado!\n");
                if (fase_atual == FASE_ANDAR2) {
                    for (int x=1; x<mapa_w-1; x++) {
                        if (mapa[11][x]=='#') { mapa[11][x]='.'; mapa_base[11][x]='.'; }
                    }
                    printf("  Os espinhos da passagem desapareceram!\n");
                }
                aguardar_tecla(); return;
            }
        }
    }

    if (t == 'M' || t == 'L') {
        printf("  Caminhe ate a escada para usa-la.\n");
        aguardar_tecla(); return;
    }

    printf("  Nada para interagir aqui.\n");
    aguardar_tecla();
}

/* ══════════════════════════════════════════════
   MOVIMENTO DO JOGADOR
   ══════════════════════════════════════════════ */

void mover_jogador(int dx, int dy, int nova_dir) {
    jogador.direcao = nova_dir;
    int nx = jogador.x + dx;
    int ny = jogador.y + dy;

    if (nx < 0 || nx >= mapa_w || ny < 0 || ny >= mapa_h) return;
    char t = tile_em(nx, ny);

    if (eh_solido(t)) return;
    if (monstro_em(nx, ny) >= 0) return;

    if (fase_atual == FASE_VILA && t == 'M') {
        if (!jogador.falou_npc) {
            printf("\n  Fale com o ferreiro (N) antes de entrar!\n");
            aguardar_tecla(); return;
        }
        printf("\n  Entrando na masmorra...\n");
        aguardar_tecla();
        iniciar_andar1(); return;
    }

    if (t == 'L') {
        if (!escada_liberada) {
            printf("  A escada esta bloqueada.\n");
            aguardar_tecla(); return;
        }
        if (fase_atual == FASE_ANDAR1) {
            printf("\n  Descendo para o 2o andar...\n"); aguardar_tecla();
            iniciar_andar2(); return;
        }
        if (fase_atual == FASE_ANDAR2) {
            printf("\n  Descendo para o 3o andar...\n"); aguardar_tecla();
            iniciar_andar3(); return;
        }
        if (fase_atual == FASE_ANDAR3) {
            fase_atual = FASE_VITORIA; return;
        }
    }

    jogador.x = nx; jogador.y = ny;
    verificar_espinho(nx, ny);
}

/* ══════════════════════════════════════════════
   MOVIMENTO DOS MONSTROS
   ══════════════════════════════════════════════ */

void mover_monstros(void) {
    if (fase_atual == FASE_GAMEOVER) return;

    for (int i=0; i<n_monstros; i++) {
        if (!monstros[i].vivo) continue;
        int mx=monstros[i].x, my=monstros[i].y;
        int nx=mx, ny=my;

        if (monstros[i].tipo == 1) {
            int ddx[]={0,0,-1,1}, ddy[]={-1,1,0,0};
            int d = rand()%4;
            nx=mx+ddx[d]; ny=my+ddy[d];

        } else if (monstros[i].tipo == 2) {
            int dx=jogador.x-mx, dy=jogador.y-my;
            if (abs_val(dx) >= abs_val(dy))
                nx = mx + (dx>0 ? 1 : -1);
            else
                ny = my + (dy>0 ? 1 : -1);

        } else if (monstros[i].tipo == 3) {
            monstros[i].boss_timer++;
            int bt = monstros[i].boss_timer;
            if (bt <= 4) {
                int dx=jogador.x-mx;
                if (dx!=0) nx=mx+(dx>0?1:-1);
            } else if (bt <= 8) {
                int dy=jogador.y-my;
                if (dy!=0) ny=my+(dy>0?1:-1);
            } else {
                monstros[i].boss_timer = 0;
                for (int tentativa=0; tentativa<10; tentativa++) {
                    int tx=jogador.x+rand_range(-4,4);
                    int ty=jogador.y+rand_range(-4,4);
                    if (tx < 1) tx = 1;
                    if (tx >= mapa_w-1) tx = mapa_w-2;
                    if (ty < 1) ty = 1;
                    if (ty >= mapa_h-1) ty = mapa_h-2;
                    if (!eh_solido(tile_em(tx,ty)) && monstro_em(tx,ty)<0
                        && tile_em(tx,ty)!='#') {
                        monstros[i].x=tx; monstros[i].y=ty; break;
                    }
                }
                verificar_contato_monstros();
                continue;
            }
        }

        if (nx<0||nx>=mapa_w||ny<0||ny>=mapa_h) continue;
        char t = tile_em(nx,ny);
        if (eh_solido(t)) continue;
        if (monstro_em(nx,ny)>=0) continue;
        if (t=='#') continue;

        monstros[i].x=nx; monstros[i].y=ny;
    }

    verificar_contato_monstros();
}

/* ══════════════════════════════════════════════
   PROCESSAMENTO DE INPUT
   ══════════════════════════════════════════════ */

int processar_input(char c) {
    switch (c) {
        case 'w': case 'W': mover_jogador( 0,-1, DIR_CIMA);     return 1;
        case 's': case 'S': mover_jogador( 0, 1, DIR_BAIXO);    return 1;
        case 'a': case 'A': mover_jogador(-1, 0, DIR_ESQUERDA); return 1;
        case 'd': case 'D': mover_jogador( 1, 0, DIR_DIREITA);  return 1;
        case 'o': case 'O': atacar();                            return 1;
        case 'i': case 'I': interagir();                         return 0;
        case 'q': case 'Q': fase_atual = FASE_MENU;              return 0;
        default: return 0;
    }
}

/* ══════════════════════════════════════════════
   LOOP DA FASE
   ══════════════════════════════════════════════ */

void loop_fase(void) {
    char buf;
    while (fase_atual==FASE_VILA  || fase_atual==FASE_ANDAR1 ||
           fase_atual==FASE_ANDAR2 || fase_atual==FASE_ANDAR3) {

        limpar_tela();
        const char *tit[]={"","VILA","ANDAR 1 : Tutorial","ANDAR 2","ANDAR 3 : BOSS FINAL"};
        printf("  ====== %s ======\n", tit[fase_atual]);
        imprimir_mapa();
        mostrar_hud();
        printf("  > ");

        buf = _getch();
        int move = processar_input(buf);
        
        if (move && fase_atual != FASE_GAMEOVER && fase_atual != FASE_VITORIA
                 && fase_atual != FASE_MENU)
            mover_monstros();

        if (fase_atual==FASE_GAMEOVER || fase_atual==FASE_VITORIA ||
            fase_atual==FASE_MENU) break;
    }
}

/* ══════════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════════ */

int main(void) {
    srand((unsigned)time(NULL));

    fase_atual = FASE_MENU;

    while (1) {
        if (fase_atual == FASE_MENU) {
            jogador.vidas=3; jogador.arma=ARMA_NENHUMA;
            jogador.falou_npc=0; jogador.direcao=DIR_BAIXO;
        }

        switch (fase_atual) {
            case FASE_MENU:     menu_principal();    break;
            case FASE_TUTORIAL: tela_tutorial();     break;
            case FASE_VILA:     iniciar_vila();      loop_fase(); break;
            case FASE_ANDAR1:   iniciar_andar1();    loop_fase(); break;
            case FASE_ANDAR2:   iniciar_andar2();    loop_fase(); break;
            case FASE_ANDAR3:   iniciar_andar3();    loop_fase(); break;
            case FASE_VITORIA:  tela_vitoria();      break;
            case FASE_GAMEOVER: tela_gameover();     break;
            default: fase_atual = FASE_MENU; break;
        }
    }
    return 0;}