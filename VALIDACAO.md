# Validação — NF Capture R3

## Método e testes

O LIVE usa multisseno periódico determinístico, com excitação em todos os bins positivos, exceto DC/Nyquist. O analisador calcula Y/X usando a sequência realmente gerada, reconstrói a resposta periódica e remove a referência de tempo do pico dominante para exibir a fase relativa.

Testes aprovados na preparação da R3:

- Três perfis, em 44,1 / 48 / 96 kHz.
- Filtros de referência a 103,816 Hz, Q 24, ganhos de +30 e -30 dB. Todos os bins úteis foram comparados à resposta matemática dos coeficientes, após estabilização. Tolerância: 0,02 dB. O maior erro observado nessa comparação foi inferior a 0,00004 dB nos casos testados.
- Esse erro é de cálculo nos bins, não o erro entre o pico verdadeiro e um bin próximo. Para +30 dB em 103,816 Hz / Q 24 / 44,1 kHz: Rápido mediu máximo de +17,7312 dB; Equilibrado, +29,0738 dB; Preciso, +29,9892 dB. Em 48 e 96 kHz, Preciso mediu +29,9993 dB nesse caso.
- Ganho de -6,0206 dB e fase relativa zero com sinal deslocado circularmente, em todos os perfis e taxas. Tolerâncias: 0,001 dB e 0,01 grau.
- Silêncio e amostras NaN rejeitados.
- Recuperação após overflow real do FIFO: descarte da janela incompleta, nova preparação e retomada da leitura válida.
- Continuidade exata do gerador através de blocos de 64 / 511 / 1024, nos três perfis.
- Atualização LIVE com mudanças de ganho; Hold e Retomar; restauração de estado; passagem de áudio inalterada no analisador.
- Captura única por impulso: nove combinações de taxas e blocos; resposta FIR comparada à forma analítica.
- Redimensionamento, alça, coordenadas de clique e restauração pelo logo mantidos nas duas instâncias.
- Renderizações dos painéis de ganho e ganho + fase com uma cadeia sintética de três filtros, semelhante à demonstrada nas capturas do usuário. As imagens não são uma captura dentro da Mixa.

A fase é relativa ao pico dominante; não representa latência absoluta. Respostas de filtros não lineares ou variantes no tempo não são certificadas por esses testes. A indicação de estabilidade não é uma estimativa de coerência ou de erro metrológico.

## Verificação externa

Não houve validação desta revisão dentro da Mixa. A compatibilidade e a retomada após trocar o plugin intermediário dependem também do host restaurar corretamente os estados e o roteamento. O pluginval do ambiente não forneceu um relatório utilizável na tentativa anterior; não há aprovação externa por pluginval. Windows não foi compilado nem testado.
