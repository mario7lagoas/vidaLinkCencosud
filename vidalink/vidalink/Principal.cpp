//============================================================================
// Name        : VidaLink.cpp
// Author      : Kerley Leite <kerley@remaqbh.com.br>
// Review      : Mario Sergio <mario@remaqbh.com.br>
// Version     : 1.2
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
// IDE         : Visual studio 7
//============================================================================

#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <ctime>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#pragma warning(disable : 4996)

#pragma comment(lib,"ws2_32")


#define MAX_MSG 1024
#define BUFFER_SIZE 128

using namespace std;
int socket_desc, serverPort, storeId;
char respostaServidor[MAX_MSG];
char startTime[30];
char message[BUFFER_SIZE], delimit[] = "|", delimit2[] = "=";;
string serverIp;
ofstream arquivoProduto, arquivoControle, logdir, arqIni;
FILE *arquivoIni;
//ifstream arquivoIni;
BOOL DEBUG = FALSE;
typedef void (WINAPI*cfunc)();

time_t rawtime;
struct tm *local;

struct Produto {
	string ean;// [13] ;
	string descricao; // [90];
	string laboratorio; // [25];
	string precoPromocional; // [12];
	string bloqueado = "0";
	string pmc; //   [12];

};

struct Produto produto;

int		pesquisaProduto(char *codigo);
int 	socketClient(string ipServidor, int portaServidor);
int		enviaMsgSocket(string mensagem);
int		recebeMsgSocket(void);
void	closeSocket(void);
void 	vTrace(string texto);
int 	leParametrosConfiguracao(void);


int main(int argc, char *argv[]) {

	time(&rawtime);
	local = localtime(&rawtime);

	logdir.open("EmpVidalink.log");
	if (!logdir.is_open()) {
		printf("Nao foi possivel abrir o arquivo de log!\n");
		logdir.clear();
	}
	else {
		DEBUG = true;
		if (DEBUG) { vTrace("Inicio"); }
	}

	if (argc != 2) {
		printf("Parametro invalido.\n");
		if (DEBUG) { vTrace("Parametro Invalido"); }
		if (DEBUG) { vTrace("Fim."); }
		return 1;
	}

	arquivoControle.open("ctr.txt");

	if (!arquivoControle.is_open()) {
		if (DEBUG) { vTrace("Nao foi possivel abrir o arquivo Controle.txt!"); }
		arquivoControle.clear();
		return 1;
	}

	if ((leParametrosConfiguracao()) != 0) {
		if (DEBUG) { vTrace("Fim."); }

		return 1;
	}

	if (strcmp( argv[1],"-V")== 0) {
		printf("============================================================================\n");
		printf(" Name        : %s\n", argv[0]);
		printf(" Author      : Kerley Leite <kerley@remaqbh.com.br>\n");
		printf(" Review      : Mario Sergio <mario@remaqbh.com.br>\n");
		printf(" Version     : 1.2\n");
		printf("============================================================================\n");
		return 1;

	}

	if ((pesquisaProduto(argv[1])) != 0) {
		if (DEBUG) { vTrace("Fim."); }

		return 1;
	}
	if (DEBUG) { vTrace("Fim."); }

	return 0;

}

void vTrace(string texto) {

	strftime(startTime, 30, "%d-%m-%Y %H:%M:%S - ", local);

	logdir << startTime << texto << endl;

}

int pesquisaProduto(char *codigo) {
	if (DEBUG) { vTrace("Iniciando pesquisa de produto"); }
	char store[5];
	string codigoEan = codigo;
	strftime(startTime, 30, "%Y-%m-%d %H:%M:%S", local);
	string data = startTime;
	sprintf(store, "%i", storeId);
	string mensagemInicio = "execute_query  --delimiter=# --query=\"select sku.sku_id, plu.commercial_description, pricing.price from sku inner join plu on sku.plu_key = plu.plu_key inner join pricing on pricing.plu_key = plu.plu_key where  pricing.type_price = 1 and pricing.store_key =";
	string mensagemMeio = " and sku.sku_id ='";
	string mensagemFim = "' and pricing.start >= IFNULL( ( select max( start) from pricing p2 where p2.store_key = pricing.store_key and p2.type_price = pricing.type_price and p2.plu_key = pricing.plu_key and p2.start <= '" + data + "' ) , '" + data + "' )\"";
	string mensagem = mensagemInicio + store + mensagemMeio + codigoEan + mensagemFim;
	if (DEBUG) { vTrace("Codigo Ean: " + codigoEan + " - Loja: " + store); }
		if(DEBUG){ vTrace("Query de perquisa: " + mensagem);}

	if (socketClient(serverIp, serverPort) == 0) {
		if (DEBUG) { vTrace("Socket OK!"); }
		if (enviaMsgSocket(mensagem) == 0) {
			if (recebeMsgSocket() == 0) {
			}
			else {
				if (DEBUG) { vTrace("Falhou!"); }
				return 1;
			}
		}
	}
	else {
		if (DEBUG) { vTrace("Falhou!"); }
		return 1;
	}
	return 0;
}

int leParametrosConfiguracao() {
	char  linha[128], *token, delimit[] = "=";
	
	if (DEBUG) { vTrace("Parse de parametros."); }
	
	if ((arquivoIni = fopen("EmpVidalink.ini", "r")) != NULL) {
		while (fgets(linha, sizeof(linha), arquivoIni))
		{
			//if(DEBUG){ vTrace(linha);}
			token = strtok(linha, delimit);
			int j = 0;
			while (token != NULL) {
				switch (j) {
				case 0:
					if (strcmp(token, "STORE") == 0) {
						token = strtok(NULL, delimit);
						storeId = atoi(token);
						//	if(DEBUG){ vTrace(token);}

					}
					else if (strcmp(token, "SERVER_IP") == 0) {
						token = strtok(NULL, delimit);
						serverIp = token;
						//	if(DEBUG){ vTrace(token);}

					}
					else if (strcmp(token, "SERVER_PORT") == 0) {
						token = strtok(NULL, delimit);
						serverPort = atoi(token);
						//	if(DEBUG){ vTrace(token);}
					}
					break;
				}
				token = strtok(NULL, delimit);
				j++;

			}
		}
		fclose(arquivoIni);

		return 0;
	}
	else {
		if (DEBUG) { vTrace("Nao foi possivel abrir o arquivo de configuracao."); }
		return 1;

	}
}
int avaliarRetornoServidor() {
	char *token, *retorno, linha[255], delimit[] = "|";
	FILE *arquivoControle;

	arquivoControle = fopen("ctr.txt", "r");

	if (!arquivoControle) {
		if (DEBUG) { vTrace("Nao foi possivel abrir o arquivo de controle."); }
		return 1;
	}
	else {
		if (fgets(linha, 30, arquivoControle) != NULL) {
			if (DEBUG) { vTrace(linha); }
			token = strstr(linha, "OK");
			if (token) {
				if (DEBUG) { vTrace("Produto encontrado."); }
				if (fgets(linha, 255, arquivoControle) != NULL) {
					if (DEBUG) { vTrace(linha); }
					token = strtok(linha, delimit);
					int j = 0;
					while (token != NULL) {
						switch (j) {
						case 0:
							if (strlen(token) > 0) {
								produto.ean = token;
							}
							break;
						case 1:
							if (strlen(token) > 0) {
								produto.descricao = token;
								produto.laboratorio = "                         ";
							}
							break;
						case 2:
							if (strlen(token) > 0) {
								produto.precoPromocional = token;
								produto.pmc = token;
							}
							break;
						}
						token = strtok(NULL, delimit);
						j++;
					}
				}
				else {
					if (DEBUG) { vTrace("Nao foi possivel ler dados do produto."); }
					return 1;
				}
			}
			else {
				if (DEBUG) { vTrace("Produto NAO encontrado."); }
				return 1;
			}
		}
		else {
			if (DEBUG) { vTrace("Nao foi possivel ler o retorno."); }
			return 1;
		}
		fclose(arquivoControle);
		if (DEBUG) { vTrace("Arquivo de controle lido e fechado."); }
	}
	return 0;
}

int enviaMsgSocket(string mensagem) {

	if (DEBUG) { vTrace("Enviando mensagem - Inicio."); }
	if (send(socket_desc, mensagem.c_str(), strlen(mensagem.c_str()), 0) < 0) {

		if (DEBUG) { vTrace("Falha ao enviar mensagem."); }
		return 1;
	}
	//if(DEBUG){ vTrace("Dados enviados: " + mensagem );}
	if (DEBUG) { vTrace("Enviar mensagem - Fim."); }
	return 0;
}

int recebeMsgSocket() {
	if (DEBUG) { vTrace("Receber mensagem - Inicio."); }
	string real, centavos;
	int k;
	
	memset(respostaServidor, 0, 1024);

	recv(socket_desc, respostaServidor, sizeof(respostaServidor), 0);
		

	closeSocket();

	if (DEBUG) { vTrace("Resposta servidor:"); }
	if (DEBUG) { vTrace(respostaServidor); }

	arquivoControle << respostaServidor << endl;
	arquivoControle.close();

	if (DEBUG) { vTrace("Recebe mensagem - Fim."); }

	if (avaliarRetornoServidor() == 0) {
		if (DEBUG) { vTrace("Formatando campos."); }
		while (produto.ean.size() < 13) {
			produto.ean = produto.ean + " ";
		}
		while (produto.descricao.size() < 90) {
			produto.descricao = produto.descricao + " ";
		}
		while (produto.laboratorio.size() < 25) {
			produto.laboratorio = produto.laboratorio + " ";
		}
		//	produto.precoPromocional.replace(produto.precoPromocional.find("."), 1, "");
		for (k = 0; k < produto.precoPromocional.size(); k++) {
			if (produto.precoPromocional[k] == '.') {
				real = produto.precoPromocional.substr(0, k);
				centavos = produto.precoPromocional.substr(k + 1, produto.precoPromocional.size());
			}
		}
		if (centavos.size() == 1) {
			centavos = centavos + "0";
		}
		produto.precoPromocional = real + centavos;
		while (produto.precoPromocional.size() < 12) {
			produto.precoPromocional = "0" + produto.precoPromocional;
		}
		cout << produto.precoPromocional << endl;
		produto.pmc = produto.precoPromocional;
		//		while (produto.pmc.size() < 12  ){
		//			produto.pmc = "0" + produto.pmc;
		//		}
				//produto.precoPromocional = produto.precoPromocional.substr(0,10) + produto.precoPromocional.substr(11,2);
				//produto.pmc = produto.pmc.substr(0,10) + produto.pmc.substr(11,2);
		cout << produto.precoPromocional << endl;

		if (DEBUG) { vTrace("Campos formatados."); }
		if (DEBUG) { vTrace("Linha final: " + produto.ean + produto.descricao + produto.laboratorio + produto.precoPromocional + produto.bloqueado + produto.pmc); }

		if (DEBUG) { vTrace("Abrindo arquivo de Produto!"); }
		arquivoProduto.open("Produto.txt");

		if (!arquivoProduto.is_open()) {
			if (DEBUG) { vTrace("Nao foi possivel abrir o arquivo Produto."); }
			arquivoProduto.clear();
			return 1;
		}
		else {

			arquivoProduto << produto.ean << produto.descricao << produto.laboratorio << produto.precoPromocional << produto.bloqueado << produto.pmc << endl;
			if (DEBUG) { vTrace("Arquivo de Produto gravado."); }

		}
		return 0;
	}
	else {
		return 1;
	}
	return 0;
}

void closeSocket() {
	if (DEBUG) { vTrace("Fechando conexao socket."); }

	closesocket(socket_desc);
	if (DEBUG) { vTrace("Conexao finalizada com sucesso."); }

}

int socketClient(string ipServidor, int portaServidor) {

	WSADATA wsa;
	SOCKET s;
	string retornoErro;
	struct sockaddr_in server;
	if (DEBUG) { vTrace("Inicializando socket."); }

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		retornoErro = WSAGetLastError();
		if (DEBUG) { vTrace("Falhou - Erro Codigo:" + retornoErro); }

		//if (DEBUG) { vTrace("Falhou - Erro Codigo: %d" + WSAGetLastError()); }
		return 1;
	}
	if (DEBUG) { vTrace("Iniciado OK."); }
	socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(socket_desc == -1){
		retornoErro = WSAGetLastError();
		if (DEBUG) { vTrace("Falhou - Erro Codigo:" + retornoErro); }
		if (DEBUG) { vTrace("Nao foi possivel criar socket"); }
		return 1;
	}
	if (DEBUG) { vTrace("Socket criado."); }
	server.sin_addr.s_addr = inet_addr(ipServidor.c_str());
	server.sin_family = AF_INET;
	server.sin_port = htons(portaServidor);

	if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		if (DEBUG) { vTrace("Falhou -Erro ao conectar em " + ipServidor); }
		return 1;
	}
	if (DEBUG) { vTrace("Conectado com sucesso."); }
	return 0;
}
