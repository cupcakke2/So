A nossa solução 1.1 contêm a solução da etapa 1.1 com sinais.
Ou seja, é um server que atende pedidos de um cliente ao mesmo tempo e que quando o cliente sai da sessão quer seja por
ctr+C ou por DISCONNECT imprime uma mensagem a dizer que o cliente se desconectou.

A nossa soluçao 1.2 contêm uma solução de um server que pode interagir com até 10 (definido por MAX_SESSION_COUNT) clientes ao mesmo tempo,
sempre que um cliente se desconecta o server também faz print de uma mensagem e continua a receber commandos. Usa semáforos, threads gestoras e threads leitoras de .jobs como o enuciado sugere.

As duas soluções são mostradas caso ocorra algum erro grave com a solução 1.2, nesse caso a solução 1.1 serviria para provar que conseguimos
implementar as funcionalidades. Caso apenas uma solução possa ser avaliada deverá ser a solução 1.2.