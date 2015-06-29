/************************************************************

*

*			TRABALHO DE REDES - PROF JULIO ESTRELLA

*

*			Lucas Marques Rovere  		8139750

*

*			Luiz Felipe Machado Votto   8504006

*

************************************************************/



#include <stdio.h>

#include <stdlib.h>

#include <pthread.h>

#include <string.h>

#include <sys/socket.h>

#include <sys/types.h>

#include <netinet/in.h>

#include <netdb.h>

#include <arpa/inet.h>

#include <unistd.h>

#include <errno.h>

#include <semaphore.h>



#define MAXC 100

#define PORT 6969



typedef struct{

	char IP[32];

	char nome[32];



} contato;



typedef struct{



	int tipo;

	contato emissor;

	char destinatario[32];

	char conteudo[1000];



} pkt_envio;



typedef struct list{



	struct list* next;

	pkt_envio* mensagem;



} buffer;



int quit_flag = 0, cont_list_tam = 0, inbox_tam = 0, filaenvio_tam = 0;

buffer *inbox_inicio = NULL, *inbox_fim = NULL;

buffer *filaenvio_inicio = NULL, *filaenvio_fim = NULL;

contato eu;

contato lista_contatos[MAXC];

sem_t inbox_m, queue_m;



void *menu_f();

void add_contato();

void list_contato();

void exc_contato();

void enviar_msg();

void ver_msg(int);

void sair();



void *send_f();



void *receive_f();



void inbox_add(pkt_envio*);

void inbox_rem(pkt_envio**);

void queue_add(pkt_envio*);

void queue_rem(pkt_envio**);



void confirmar(char*);

void excluir_c(char*);



void inicializa_lista();

int add_contato_lista(contato);

int rem_contato_lista(int);

int procura_nome(char*);



int main(){



	pthread_t send, receive, menu;



	inicializa_lista();



	sem_init(&inbox_m, 0, 1);

	sem_init(&queue_m, 0, 1);



	printf("Digite seu nome de usuario:\n");

	scanf("%s", eu.nome);



	if (pthread_create(&send, 0, (void *) send_f, (void *) 0) != 0) {

		printf("Error creating thread producer! Exiting! \n");

		exit(0);

	}

	if (pthread_create(&receive, 0, (void *) receive_f, (void *) 0) != 0) {

		printf("Error creating thread producer! Exiting! \n");

		exit(0);

	}

	if (pthread_create(&menu, 0, (void *) menu_f, (void *) 0) != 0) {

		printf("Error creating thread producer! Exiting! \n");

		exit(0);

	}



	pthread_join(menu, 0);

	pthread_kill(send, 0);

	pthread_kill(receive, 0);



	sair();



	return 0;

}



//================================================================================//



void *menu_f(){



	int op;



	while(quit_flag == 0){



		system("clear");



		printf("%d mensagens nao lidas.\n", inbox_tam);

		printf("Digite:\n");

		printf("0 para adicionar contato;\n");

		printf("1 para listar contatos;\n");

		printf("2 para excluir contato;\n");

		printf("3 para enviar mensagem;\n");

		printf("4 para ver mensagens;\n");

		printf("5 para atualizar;\n");

		printf("6 para sair\n");



		scanf("%d", &op);



		if(op == 0)

			add_contato();

		else if(op == 1)

			list_contato();

		else if(op == 2)

			exc_contato();

		else if(op == 3)

			enviar_msg();

		else if(op == 4)

			ver_msg(0);

		else if(op == 6){

			quit_flag = 1;

		}

	}



	return NULL;

}



void add_contato(){

	pkt_envio *solicitacao;



	char IP[32];

	int aux;



	system("clear");

	printf("O IP do novo contato:\n");



	scanf("%s", IP);



	solicitacao = (pkt_envio*) malloc (sizeof(pkt_envio));



	solicitacao->emissor = eu;

	solicitacao->tipo = 1;

	solicitacao->conteudo[0] = '\0';

	strcpy(solicitacao->destinatario, IP);



	sem_wait(&queue_m);

	queue_add(solicitacao);

	sem_post(&queue_m);



	return;

}



void list_contato(){



	int i = 0;



	system("clear");



	for(i=0; i<=cont_list_tam; i++)

		if(lista_contatos[i].IP[0] != '0')

			printf("%s <%s>\n", lista_contatos[i].nome, lista_contatos[i].IP);



	printf("0 para continuar.\n");

	scanf("%d", &i);



	return;

}



void exc_contato(){



	char nome[32];



	system("clear");



	printf("Digite o nome do contato que deseja excluir:\n");

	scanf("%s", nome);



	excluir_c(nome);



	return;

}



void enviar_msg(){



	int num_contatos, i;

	char IPs[MAXC][32];

	char nome[32];

	pkt_envio **nova_msg;

	char *aux_buffer;



	system("clear");



	aux_buffer = (char*) malloc (999*sizeof(char));



	printf("Para quantos contatos a mensagem sera enviada?\n");

	scanf("%d", &num_contatos);



	for(i=0; i<num_contatos; i++){

		printf("Digite o nome do contato %d de %d:\n", i+1, num_contatos);

		printf("0 para cancelar.\n");

		scanf("%s", nome);

		if(procura_nome(nome) != -1){

			strcpy(IPs[i], lista_contatos[procura_nome(nome)].IP);

		}

		else{

			IPs[i][0] = '0';

		}

	}



	nova_msg = (pkt_envio**) malloc (num_contatos*sizeof(pkt_envio*));



	printf("Digite a mensagem (Max 1000 caracteres):\n");

	fflush(stdout);

	fflush(stdin);

	getchar();

	fgets(aux_buffer, 1000, stdin);





	for(i=0; i<num_contatos; i++){

		if(IPs[i][0] != '0'){

			nova_msg[i] = (pkt_envio*) malloc (sizeof(pkt_envio));



			strcpy(nova_msg[i]->destinatario, IPs[i]);

			nova_msg[i]->emissor = eu;

			nova_msg[i]->tipo = 0;

			nova_msg[i]->conteudo[0] = '>';

			strcat(nova_msg[i]->conteudo, aux_buffer);



			sem_wait(&queue_m);

			queue_add(nova_msg[i]);

			sem_post(&queue_m);

		}

	}



	return;

}



void ver_msg(int n){



	int op;



	pkt_envio *nova_msg = NULL;



	if(n == inbox_tam){

		nova_msg = NULL;

	}

	else{

		sem_wait(&inbox_m);

		inbox_rem(&nova_msg);

		sem_post(&inbox_m);

	}



	system("clear");



	if(nova_msg == NULL){

		printf("Nao ha mensagens novas.\n");



		printf("0 para continuar.\n");

		scanf("%d", &op);



		return;

	}



	if(nova_msg->tipo == 0){

		printf("[%d/%d]\n", n+1, inbox_tam+1);

		printf("Mensagem enviada por:\n");

		printf("%s <%s>\n\n", nova_msg->emissor.nome, nova_msg->emissor.IP);

		printf("%s\n\n", nova_msg->conteudo);

	}

	else if(nova_msg->tipo == 1){

		printf("[%d/%d]\n", n+1, inbox_tam+1);

		printf("Voce foi adicionado por:\n");

		printf("%s <%s>\n\n", nova_msg->emissor.nome, nova_msg->emissor.IP);



		printf("0 para aceitar e 1 para recusar\n");

		scanf("%d", &op);



		if(op == 0){

			add_contato_lista(nova_msg->emissor);

			confirmar(nova_msg->emissor.IP);

		}

	}

	else if(nova_msg->tipo == 2){

		printf("[%d/%d]\n", n+1, inbox_tam+1);

		printf("Voce foi excluido por:\n");

		printf("%s <%s>\n\n", nova_msg->emissor.nome, nova_msg->emissor.IP);



		rem_contato_lista(procura_nome(nova_msg->emissor.nome));

	}

	else if(nova_msg->tipo == 3){

		printf("[%d/%d]\n", n+1, inbox_tam+1);

		printf("\n%s <%s> confirmou sua solicitacao.\n\n", nova_msg->emissor.nome, nova_msg->emissor.IP);



		add_contato_lista(nova_msg->emissor);

	}



	printf("0 para ver proxima mensagem.\n");

	printf("1 para retornar ao menu principal.\n");



	scanf("%d", &op);



	if(op == 0){

		ver_msg(n);

	}



	return;

}



void sair(){



	pkt_envio *aux = NULL;



	sem_wait(&inbox_m);

	inbox_rem(&aux);

	sem_post(&inbox_m);



	while(aux != NULL){

		free(aux);

		inbox_rem(&aux);

	}



	queue_rem(&aux);



	while(aux != NULL){

		free(aux);

		queue_rem(&aux);

	}



	return;

}



//================================================================================//



void *send_f(){

	int sock;

	struct hostent *host;

	struct sockaddr_in server_addr;



	pkt_envio *msg = NULL;



	while(quit_flag == 0){



		if(filaenvio_tam == 0){

			sleep(3);

		}

		else {

			sem_wait(&queue_m);

			queue_rem(&msg);

			sem_post(&queue_m);



			if(msg != NULL){

				if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1);

				else

				{

					host = gethostbyname(msg->destinatario);



					server_addr.sin_family = AF_INET;

					server_addr.sin_port = htons(PORT);

					server_addr.sin_addr = *((struct in_addr *)host->h_addr);

					bzero(&(server_addr.sin_zero),8);



					if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1);

					else

					{

						send(sock, &(msg->tipo), sizeof(int), 0);

						send(sock, msg->conteudo, 1000, 0);

						send(sock, msg->emissor.nome, 32, 0);

					}



					free(msg);



					close(sock);

				}

			}

		}

	}



	return NULL;

}



//================================================================================//



void *receive_f(){

	int sock, connected, true = 1;

	struct sockaddr_in server_addr, client_addr;

	int sin_size;



	pkt_envio *msg = NULL;



	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)

	{

		perror("Erro no Socket");

		exit(1);

	}



	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true,sizeof(int)) == -1)

	{

		perror("Erro setsockopt");

		exit(1);

	}



	server_addr.sin_family = AF_INET;

	server_addr.sin_port = htons(PORT);

	server_addr.sin_addr.s_addr = INADDR_ANY;

	bzero(&(server_addr.sin_zero), 8);



	strcpy(eu.IP, inet_ntoa(server_addr.sin_addr));



	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)

	{

		perror("Nao foi possivel realizar o bind");

		exit(1);

	}



	if (listen(sock, 10) == -1)

	{

		perror("Erro de Listen");

		exit(1);

	}



	while(quit_flag == 0){

		sin_size = sizeof(struct sockaddr_in);

		connected = accept(sock, (struct sockaddr *)&client_addr, (socklen_t*) &sin_size);



		msg = (pkt_envio*) malloc (sizeof(pkt_envio));



		recv(connected, &(msg->tipo), sizeof(int), 0);

		recv(connected, msg->conteudo, 1000, 0);

		strcpy(msg->destinatario, eu.IP);

		strcpy(msg->emissor.IP, inet_ntoa(client_addr.sin_addr));

		recv(connected, msg->emissor.nome, 32, 0);



		if(msg->tipo == 0 && procura_nome(msg->emissor.nome) == -1);

		else{

			sem_wait(&inbox_m);

			inbox_add(msg);

			sem_post(&inbox_m);

		}



		close(connected);

	}



	return NULL;

}



//================================================================================//



void confirmar(char *IP){



	pkt_envio *nova_msg;



	nova_msg = (pkt_envio*) malloc (sizeof(pkt_envio));



	nova_msg->tipo = 3;

	nova_msg->emissor = eu;

	strcpy(nova_msg->destinatario, IP);

	nova_msg->conteudo[0] = '\0';



	sem_wait(&queue_m);

	queue_add(nova_msg);

	sem_post(&queue_m);



	return;

}



void excluir_c(char *nome){



	pkt_envio *excluir;



	int num = procura_nome(nome);



	if(num == -1){

		printf("Contato nao existe.\n");

		return;

	}



	excluir = (pkt_envio*) malloc (sizeof(pkt_envio));



	excluir->tipo = 2;

	excluir->emissor = eu;

	strcpy(excluir->destinatario, lista_contatos[num].IP);

	excluir->conteudo[0] = '\0';



	sem_wait(&queue_m);

	queue_add(excluir);

	sem_post(&queue_m);



	rem_contato_lista(num);

}



//================================================================================//



void inicializa_lista(){



	int i;



	for(i=0; i<MAXC; i++)

		lista_contatos[i].IP[0] = '0';



	return;

}



int add_contato_lista(contato novo){



	int i;



	if(cont_list_tam == MAXC){

		printf("Lista de contatos cheia.\n");



		return 0;

	}



	for(i=0; i<MAXC; i++){

		if(lista_contatos[i].IP[0] == '0'){}

		else if(strcmp(lista_contatos[i].nome, novo.nome) == 0){

			printf("Contato ja consta na lista\n");

			return 0;

		}

	}



	for(i=0; i<MAXC; i++){

		if(lista_contatos[i].IP[0] == '0'){

			lista_contatos[i] = novo;



			printf("Contato %s <%s>\n", novo.nome, novo.IP);

			printf("Adicionado com sucesso\n;");



			cont_list_tam++;



			break;

		}

	}



	return 1;

}



int rem_contato_lista(int num){



	if(num == -1) return -1;



	lista_contatos[num].IP[0] = '0';



	return 0;

}



int procura_nome(char* nome){

	int i;



	for(i=0; i<MAXC; i++){

		if(strcmp(lista_contatos[i].nome, nome) == 0){

			return i;

		}

	}



	return -1;

}



//================================================================================//



void inbox_add(pkt_envio* mensagem){



	if(inbox_fim == NULL){

		inbox_fim = (buffer*) malloc (sizeof(buffer));

		inbox_inicio = inbox_fim;

		inbox_fim->next = NULL;

		inbox_fim->mensagem = mensagem;

	}

	else{

		inbox_fim->next = (buffer*) malloc (sizeof(buffer));

		inbox_fim = inbox_fim->next;

		inbox_fim->next = NULL;

		inbox_fim->mensagem = mensagem;

	}



	inbox_tam++;



	return;

}



void inbox_rem(pkt_envio** retorno){



	if(inbox_inicio == NULL){

		*retorno = NULL;

		return;

	}



	buffer* aux;



	*retorno = (pkt_envio*) malloc (sizeof(pkt_envio));

	**retorno = *inbox_inicio->mensagem;



	aux = inbox_inicio;

	inbox_inicio = inbox_inicio->next;



	if(inbox_inicio == NULL){

		inbox_fim = NULL;

	}



	free(aux->mensagem);

	free(aux);



	inbox_tam--;



	return;

}



void queue_add(pkt_envio* mensagem){



	if(filaenvio_fim == NULL){

		filaenvio_fim = (buffer*) malloc (sizeof(buffer));

		filaenvio_inicio = filaenvio_fim;

		filaenvio_fim->next = NULL;

		filaenvio_fim->mensagem = mensagem;

	}

	else{

		filaenvio_fim->next = (buffer*) malloc (sizeof(buffer));

		filaenvio_fim = filaenvio_fim->next;

		filaenvio_fim->next = NULL;

		filaenvio_fim->mensagem = mensagem;

	}



	filaenvio_tam++;



	return;

}



void queue_rem(pkt_envio** retorno){



	if(filaenvio_inicio == NULL) {

		*retorno = NULL;

		return;

	}



	buffer* aux;



	*retorno = (pkt_envio*) malloc (sizeof(pkt_envio));

	**retorno = *filaenvio_inicio->mensagem;



	aux = filaenvio_inicio;

	if(filaenvio_fim == filaenvio_inicio){

		filaenvio_fim = NULL;

		filaenvio_inicio = NULL;

	}

	else

		filaenvio_inicio = filaenvio_inicio->next;



	free(aux->mensagem);

	free(aux);



	filaenvio_tam--;



	return;

}

