# Validação — NF Capture beta 0.2 LIVE / R2

## Testes internos aprovados

- Captura única: resposta complexa de um FIR h=[0,5; 0,25] comparada à resposta analítica H(w)=0,5+0,25 exp(-jw), com erro absoluto inferior a 0,00001 em todos os bins positivos antes de Nyquist.
- Nove combinações: 44,1 / 48 / 96 kHz com blocos de 64 / 511 / 1024 amostras.
- Passagem de áudio idêntica amostra a amostra nos dois canais do analisador.
- LIVE: magnitude e fase do FIR conferidas com a resposta analítica, com tolerância de 0,001 dB e 0,001 grau, incluindo deslocamentos de 0, 127 e 8191 amostras dentro da janela de 8192.
- Integração da interface com a captura: alimentação do processador com frames sucessivos, leitura real do FIFO e atualização da curva após mudanças de ganho. Aumento de ganho de 6,0206 dB e redução de 12,0412 dB conferidos em todos os bins positivos úteis, com tolerância de 0,0001 dB.
- Hold: os vetores da curva permaneceram inalterados durante a pausa; a leitura retomou os novos valores ao sair de Hold.
- Probe: impulso único de amplitude 0,25 em ambos os canais, sem repetição no bloco seguinte; seno de 1 kHz a amplitude 0,125 em 48 kHz.
- Probe LIVE: impulsos de 0,25 a cada 8192 amostras, conferidos amostra a amostra através de blocos de 64, 511 e 1024 amostras.
- Interface roxa mais clara com nomes centralizados. Redimensionamento e restauração pelo logo verificados em ambas as instâncias. A imagem mostra o filtro sintético usado nos testes e uma curva de referência.

Os testes de interface chamam a rotina de atualização diretamente; não são um teste dentro da DAW nem uma garantia de temporização sob carga. A frequência de leitura LIVE é aproximadamente Fs/8192, limitada também pelo temporizador gráfico de 15 Hz e pela latência do plugin em teste.

## Revisão R2

- Capture e Probe redimensionados para larguras de 700, 1400 e 1600, com proporções preservadas.
- Alça presente no canto inferior direito e teste de clique nos controles após escalar a interface.
- Dois cliques na área do logo restauram o tamanho padrão; clique duplo fora da área não o altera.
- Renderizações revisadas nos tamanhos reduzido, padrão e ampliado.
- Estado LIVE do analisador e sinal periódico do Probe preservados após salvar o estado, recriar o processador e preparar o áudio novamente. A nova interface do analisador recupera o modo LIVE.
- Formato de estado atualizado com leitura dos estados da versão anterior. Estados antigos não continham o modo LIVE; exigem ativação manual inicial.

## Limites e validação externa

A Mixa ainda não foi usada para validar esta versão. Na tentativa anterior, o pluginval do ambiente encerrou sem relatório utilizável; não há aprovação externa por pluginval. Não há compilação ou validação Windows nesta entrega.

Os testes verificam o caminho digital em condições controladas. Não certificam medições com ruído, saturação, compressão, modulação ou sinal de teste incorreto. A fase é relativa ao pico do período em LIVE, e ao início detectado na captura única. Não é uma medição de latência absoluta.

Consulte LEIA-ME.md para instalar os dois componentes atualizados e ativar LIVE no Probe e no Capture.
