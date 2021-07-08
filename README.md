Projeto de um espelho inteligente, que fará interação com o usuário, indicando informações como clima, data, hora, lembretes, como compromissos ou frases motivacionais, o espelho fará a identificação de presença do usuário, a partir de um sensor de presença, em seguida, é feito o reconhecimento facial, após a identificação, um sistema processado na Raspberry Pi 3, será acionado e as informações serão mostradas.

Para a interface foi utilizado o código Smart Mirror, disponibilizado em https://github.com/MichMich/MagicMirror.

Código main e shell para chamado do app Smart Mirror

Código main com GPIO para identificar o valor do sensor de presença e entrar no aplicativo do Smart Mirror. Utiliza processos pais e filho para analisar o movimento depois de 5 min da inicialização e depois a cada 2 min. Esses processos controlam o espelho para inicializar e sair do espelho, uma maneira de economizar energia, já que o magic mirror e o reconhecimento facial utilizam muito o processamento da raspberry.

<p align="center">
  <img src="https://github.com/bgabiz/Sistemas-Embarcados/blob/master/Banco/Images/Test.png?raw=true" alt="Face_Reco"/>
</p>
