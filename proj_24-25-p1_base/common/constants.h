// constantes partilhadas entre cliente e servidor
#define MAX_SESSION_COUNT 1  // num max de sessoes no server, 1 default, necessario alterar mais tarde
#define STATE_ACCESS_DELAY_US  // delay a aplicar no server
#define MAX_PIPE_PATH_LENGTH 40 // tamanho max do caminho do pipe
#define MAX_STRING_SIZE 40
#define MAX_NUMBER_SUB 10
#define MAX_CONNECT_MESSAGE_SIZE ((size_t)121)  //tamanho max da mensagem de conexao 
#define MAX_CONNECT_RESPONSE_SIZE 3 //tamanho max da responsta da mensagem de conexao (op_code + char result + '\0')
#define MAX_SUBSCRIBE_MESSAGE_SIZE 42 //tamanho max da mensagem de subscrição
#define MAX_SUBSCRIBE_RESPONSE_SIZE 3 //tamanho max da resposta da mensagem de subscrição (op_code + char result + '\0)
#define MAX_UNSUBSCRIBE_MESSAGE_SIZE 42 //tamanho max da mensagem de subscrição
#define MAX_UNSUBSCRIBE_RESPONSE_SIZE 3 //tamanho max da resposta da mensagem de subscrição (op_code + char result + '\0)
#define MAX_REQUEST_SIZE 42 //tamanho max para qualquer request
#define MAX_KEY_SIZE 41 //tamanho max de um key
#define MAX_DISCONECT_MESSAGE_SIZE ((size_t) 2) //tamanho max da messagem de desconexao (op_code) + '\0'
#define MAX_DISCONECT_RESPONSE_SIZE ((size_t) 3) //tamanho max da messagem de desconexao (op_code + char result + '\0')