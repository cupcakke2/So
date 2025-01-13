A nossa solução 1.1 contêm a solução da etapa 1.1 com sinais.
Ou seja, é um server que atende pedidos de um cliente ao mesmo tempo e que quando o cliente sai da sessão quer seja por
ctr+C ou por DISCONNECT imprime uma mensagem a dizer que o cliente se desconectou e espera que outro cliente entre.

A nossa soluçao 1.2 contêm uma solução de um server que pode interagir com até 10 (definido por MAX_SESSION_COUNT) clientes ao mesmo tempo,
sempre que um cliente se desconecta o server também faz print de uma mensagem e continua a receber commandos. Usa semáforos, threads gestoras e threads leitoras de .jobs como o enuciado sugere.

A solução 1.1 é apenas mostrada para caso haja algum erro grave na solução 1.2 nós podermos demonstrar que implementámos as funcionalidades.
Se a solução 1.2 conseguir ser executada com sucesso a solução 1.1 deverá ser ignorada. 