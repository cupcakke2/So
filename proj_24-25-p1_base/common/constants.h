// constantes partilhadas entre cliente e servidor
#define MAX_SESSION_COUNT 1  // num max de sessoes no server, 1 default, necessario alterar mais tarde
#define STATE_ACCESS_DELAY_US  // delay a aplicar no server
#define MAX_PIPE_PATH_LENGTH 40 // tamanho max do caminho do pipe (39 caracteres + '\0')
#define MAX_STRING_SIZE 40
#define MAX_NUMBER_SUB 10
#define MAX_CONNECT_MESSAGE_SIZE 122 //tamanho max da mensagem de conexao (39 caracteres * 3 + 5 pelas 3 | mais o opcode e '\0')
