Código main e shell para chamado do app Smart Mirror

Código main com GPIO para identificar o valor do sensor de precensa e entrar no aplicativo do Smart Mirror. Utiliza processos pais e filho para analisar movimento depois de 5 min da inicialização e depois a cada 2 min. Esses processos controlam o espelho para inicializar e sair do espelho, uma maneira de economizar energia, já que o magic mirro e o reconhecimento facial utilizam muito fortemente o processamento da raspberry.


<p align="center">
  <img src="https://github.com/PedrinAugusto/Sistemas_Embarcados/tree/master/Banco/Images/Test.png?raw=true" alt="Face_Reco"/>
</p>
