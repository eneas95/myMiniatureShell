//Includes necessários:
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>

//Defines:
#define MAX_CMD_LINE 1024 // TAMANHO MÁXIMO DA LINHA DE COMMAND
#define MAX_ARG 100 // NUMERO MAXIMO DE ARGUMENTOS
#define FILE_SIZE 1024 // TAMANHO MÁXIMO DO ARQUIVO
#define CMD_NUM 100 // NUMERO MÁXIMO DE COMANDOS

//struct que representa o comando a ser executado
struct CMD {
	char * argv[MAX_ARG + 1]; // será usado no execvp (comando)
	int infd; // input file descriptor
	int outfd;// output file descriptor
};

char infile[FILE_SIZE]; //arquivo de input
char outfile[FILE_SIZE]; //arquivo de output

struct CMD cmds[CMD_NUM]; //struct que representa o comando a ser executado
char cmdLine[MAX_CMD_LINE + 1]; //linha de comando
char avLine[MAX_CMD_LINE + 1];
int cmd_num;
int backGroundExec = 0; // backGroundExec is false
int append = 0; //append is false

char * lineptr; //ponteiro que aponta para a string linha de comando
char * avptr; //ponteiro auxiliar
int cmd_counter = 0; //command counter
int ultimo_pid = 0; //último Process ID

int shell_process(); //processo do shell
int ler_cmd(); //Função que lê a linha de comando
int parse_cmd(); // executa o parse da linha de comando e define o que será input ou output
int exec_cmd(); // executa o comando a partir do fork2 e faz o redirecionamento, se necessário
int init(); //inicializa o shell
void handler(int sig); //Função auxiliar que será usada no signal handler
int check_tokens(const char*); //Função que analisa os tokens ('\0', '\n', '|', '>', '<', etc)
void getNomeArquivo(char *); //Função que obtem o nome do arquivo, tanto de saida quanto de entrada
void get_cmd(int ); //Função que guarda apenas os comandos a serem executados, desconsiderando tokens 
void fork2(struct CMD * p, int n); // Função que realiza o fork() e o execvp(2)

// FUNÇÃO MAIN //
int main(int argc, char ** argv) {
    init();
    printf("Bem vindo ao meu miniature shell\n");
	shell_process();
	return 0;
}

//Função auxiliar signal handler
void handler(int sig) {
	printf("\ncmd>"); //pede o comando
	fflush(stdout); //limpa o output buffer e move a info para o console
}

//Função que inicializa o shell
int init() {
    //Diferente de malloc, que apenas aloca um bloco de memória,
    //memset seta os bytes de um bloco de memória para um valor específico, no caso, zero.
	memset(cmdLine, 0, sizeof(cmdLine));
	memset(cmds, 0, sizeof(cmds));
	for (int i = 0; i < CMD_NUM; ++i) {
		cmds[i].infd = 0;
		cmds[i].outfd = 1;
	}
	memset(&avLine, 0, sizeof(avLine));
	memset(infile, 0, sizeof(infile));
	memset(outfile, 0, sizeof(outfile));

    //signal handler
	signal(SIGINT, handler);
	signal(SIGQUIT, SIG_IGN);

	lineptr = cmdLine;
	avptr = avLine;
	ultimo_pid = 0;
}

// Processo da shell
int shell_process() {
	while (1) {
		printf("cmd>");
		fflush(stdout);
		init();
		if (ler_cmd() == -1) {
			printf("Erro na função ler_cmd\n");
			exit(1);
		}
		if (parse_cmd() == -1) {
			printf("Erro na função parse_cmd\n");
			exit(1);
		}
		
		if (exec_cmd() == -1) {
			printf("Erro na função exec_cmd\n");
			exit(1);
		}
	}
	return 0;
}

//Função que lê a linha de comando
int ler_cmd() {
	if (fgets(cmdLine, MAX_CMD_LINE, stdin) == NULL) {
        // SEM COMANDO
		return -1;
	}	
	return 0;
}

//Função que guarda apenas os comandos a serem executados, desconsiderando tokens 
void get_cmd(int i) {
	int numero = 0;
	int palavra = 0; // bool: verifica se é uma palavra
	while (*lineptr != '\0') {
        //não chegou ainda no fim da string de comando
		while (*lineptr == ' ' || *lineptr == '\t') ++lineptr; //pula espaços em branco
		if (*lineptr == '\n' || *lineptr == '\0') break; //chegou no fim
		cmds[i].argv[numero] = avptr;
		char token = *lineptr;
		while (token != '\0'&& token != '\n' && token != ' ' && token != '\t' && token != '&' && token != '<' && token != '>' && token != '|') {
			*avptr = token;
			avptr++;
			lineptr++;
			token = *lineptr;
			palavra = 1; //token constituirá uma palavra
		}
		*avptr++ = '\0';
		switch (*lineptr) {
			case ' ':
			case '\t':
				numero++;
				palavra = 0;
				break;
			case '|':
			case '&':
			case '\n':
			case '<':
			case '>':
				if (palavra == 0)
					cmds[i].argv[numero] = NULL;
			default :
				return ;
		}
	}
}

//Função que analisa os tokens ('\0', '\n', '|', '>', '<', etc)
int check_tokens(const char * s) {
	
	char * p;
	while (*lineptr == ' ' || *lineptr == '\t') ++lineptr; //pula espaços
	p = lineptr;
	while (*s != '\0' && *s == *p) {
        //*s será um token
        //se não estiver no fim da string
        //e *p for o token analisado
        //incrementa para verificar toda a string
		++s;
		++p;
	}
	if (*s == '\0') {
        //chegou no fim da string
		lineptr = p;
		return 1; //true
	} else {
		return 0; //false
	}
}

//Função que obtem o nome do arquivo, tanto de saida quanto de entrada
void getNomeArquivo(char * name) {
	while (*lineptr == ' ' || *lineptr == '\t') ++lineptr; //pula espaços em branco
	while (*lineptr != ' ' && *lineptr != '\t' && *lineptr != '\n'
			&& *lineptr != '\0' && *lineptr != '|' && *lineptr != '&' 
			&& *lineptr != '<' && *lineptr != '>') {
		*name++ = *lineptr++; //se não for tokens, lê o nome do arquivo
	}
	*name = '\0'; //leu até o fim
}


// Função que executa o parse da linha de comando e define o que será input ou output
int parse_cmd() {
	if (check_tokens("\n")) return 0;
	get_cmd(0);
	
	if (check_tokens("<")) {
		getNomeArquivo(infile);
        //obtem arquivo de input
	}
	int i;
    for (i = 1; i < CMD_NUM; ++i) {
		if (check_tokens("|")) {
            //comandos complexos (2 ou mais comandos)
			get_cmd(i);
		} else {
			break;
		}
	}
	if (check_tokens(">")) {
		if (check_tokens(">")) append = 1; //append = true
		getNomeArquivo(outfile);
        //obtem arquivo de saida
	}
	if (check_tokens("&")) {
		backGroundExec = 1;
	}
	if (check_tokens("\n")) {
		cmd_counter = i;
		return i; 
	} else {
		return 0;
	}
}

// Função que realiza o fork() e o execvp(2)
void fork2(struct CMD * p, int n) {
	//n: número de comandos
	pid_t pid; //Process ID
	pid = fork();
	if (pid == 0) {
		if (p->infd != 0) {
            //Apenas escreve
            //dup2() cria uma cópia de um file descriptor, utilizando o descriptor number especificado
            //Aqui o novo file descriptor será o file descriptor do stdint (isto é, 0)
			dup2(p->infd, STDIN_FILENO); //STDIN_FILENO = 0
		}
		if (p->outfd != 1) {
            //Apenas lê
            //Aqui o novo file descriptor será o file descriptor do stdout (isto é, 1)
			dup2(p->outfd, STDOUT_FILENO); //STDOUT_FILENO = 1
		}
	   
	    if (n == 1) {
			char *buf = (char*) malloc((1024)*sizeof(char));
			memset(buf, 0, sizeof(buf));
			if (read(p->infd, buf, 1024) != 0) {
                //read() retorna 0 somente se não há processo algum com o pipe aberto para write, a fim de indicar end-of-file
                //Ler 1024 bytes do arquivo associado ao file descriptor p->infd no buffer apontado por buf
				printf("	%s\n", buf);
			}
			lseek(p->infd, 0, SEEK_SET);
            //lseek() altera a posição do write/read pointer de um file descriptor
            //O offset do ponteiro, mensurado em bytes, será 0 nesse caso
            //SEEK_SET move o file pointer para o começo do arquivo.
		}
		execvp(p->argv[0], p->argv);
		exit(EXIT_FAILURE);
	} else if (pid < 0) {
		perror("Could not fork\n");
		exit(1);
	} else {
		ultimo_pid = pid;
	}
}

// executa o comando a partir do fork2 e faz o redirecionamento, se necessário
int exec_cmd() {
    // redirecionamento do input
	if (infile[0] != '\0') {
        //Haverá um arquivo input
		//Flag:
        //O_RDONLY: abertura de arquivo somente em modo leitura
		cmds[0].infd = open(infile, O_RDONLY);
		
	}
    // redirecionamento do output
	if (outfile[0] != '\0') {
		//Haverá um arquivo output
        //Flags:
        //O_WRONLY: abertura de arquivo somente em modo escrita
        //O_CREAT: criação do arquivo, caso ele ainda não exista (único caso de utilização do argumento flag)
        //O_APPEND: abertura em escrita no fim do arquivo
        //O_TRUNC: Truncar o tamanho do arquivo em 0 caso o arquivo exista
        //mode_t mode: 0666 (or 0777)
		if (append) {
			cmds[cmd_counter-1].outfd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0666);
		} else {
			cmds[cmd_counter-1].outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		}
	}
    // Criar um pipe que gera um par de file descriptors (fd)
	int pfds[2];
	for (int i = 0; i < cmd_counter; ++i) {
		if (i < cmd_counter - 1) {
			pipe(pfds);
			cmds[i].outfd = pfds[1];
			cmds[i+1].infd = pfds[0];
		}
        //executar os forks
		fork2(&cmds[i], i);
        //Agora, limpar os pipes
	    int pfd;	
		if ((pfd = cmds[i].infd) != 0) close(pfd); 
		if ((pfd = cmds[i].outfd) != 1) close(pfd);
	}
    //Espera os processos dos filhos terminarem
	while (wait(NULL) != ultimo_pid) ;
	return 0;
}
