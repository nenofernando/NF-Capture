# NF Capture — LIVE R4

## Instalar a atualização

1. Feche a Mixa e as outras DAWs.
2. Abra NF-Capture-R4-Installer.pkg e conclua a instalação para o usuário atual.
3. Reabra a Mixa e refaça a busca de plugins, se necessário. A interface deve indicar R4.

O instalador está configurado para atualizar Capture e Probe, em VST3 e AU, dentro de ~/Library/Audio/Plug-Ins. Os quatro bundles são substituídos nas respectivas pastas VST3 e Components. Não há scripts de instalação nem remoção de outros plugins. O instalador é local, sem assinatura Developer ID Installer ou notarização Apple. Os executáveis dos plugins têm assinatura ad-hoc e são universais Intel / Apple Silicon.

Se usar o pacote manual, substitua AMBOS os componentes. Não misture R3 com R4: o sinal LIVE mudou. O identificador de versão dos bundles continua 0.2.0; a revisão R4 é identificada na interface. O recibo do instalador é 0.2.4.

## O que foi corrigido

A R3 dividia uma janela longa de multisseno que podia misturar posições diferentes dos knobs. Isso gerava serrilhado durante os ajustes. A R4 usa impulsos periódicos isolados, em períodos completos, em vez do chiado contínuo. O sinal LIVE tem pico de -30,1 dBFS. Cliques podem continuar audíveis: ainda é um sinal de teste.

Há duas situações distintas no gráfico:

- PREVIA: resposta curta por impulso usada para acompanhar ajustes. Está identificada como não confirmada. Não pode ser exportada nem usada como referência A.
- LIVE confirmado: duas aquisições independentes consecutivas foram compatíveis. Os valores publicados são os próprios bins medidos; não há suavização numérica em frequência para esconder erros.

Durante uma transição, uma resposta longa ou instável pode não gerar uma prévia apropriada. Nesse caso, a última curva confirmada fica na tela com identificação explícita. Sem uma curva confirmada anterior, o gráfico aguarda. Não é prometida resposta instantânea ou precisa enquanto o sistema muda rapidamente.

A confirmação exige diferenças máximas de 0,10 dB e 0,5 grau entre aquisições consecutivas na faixa útil; fase abaixo de -100 dB não participa desse teste. Esse critério mede repetibilidade, não prova linearidade nem certifica exatidão absoluta. A prévia depende da concentração da energia da resposta em torno do pico dominante, em até aproximadamente 50 ms por lado, limitada pelo período. Não é uma identificação instantânea dos parâmetros internos do plugin.

## Usar na Mixa

1. Em uma pista sem arquivos de áudio, coloque: NF Capture Probe → plugin em teste → NF Capture.
2. Use o MESMO perfil no Probe e no Capture.
3. Para mexer nos knobs, comece por Rapido / ajustar nos dois componentes.
4. Ative o transporte da DAW e LIVE nos dois componentes.
5. Aguarde a primeira confirmação. Ajuste o plugin; acompanhe a prévia ou a última curva confirmada, conforme indicado.
6. Para conferir picos estreitos nos graves, pare os knobs e selecione Preciso / validar nos dois. Aguarde a confirmação.
7. Hold congela a última curva confirmada. Retomar continua a aquisição. Guardar A e Exportar CSV ficam disponíveis para dados confirmados ou congelados explicitamente com Hold.

Faça inicialmente um teste com o plugin em bypass: a curva confirmada deve estar em 0 dB. Depois ative o plugin. Desative ruído analógico, saturação e modulação para comparar uma resposta de EQ linear. Em um equalizador dinâmico, desative a compressão para comparar com sua curva estática.

O Probe LIVE substitui o áudio recebido pelo sinal de teste. Controle a monitoração depois do Capture; não altere o nível entre os componentes. Ao terminar, pare LIVE nos dois.

## Perfis e tempo

Em 44,1 kHz:

| Perfil | Amostras por período | Espaçamento dos bins | Período | Primeira confirmação: mínimo aproximado |
|---|---:|---:|---:|---:|
| Rapido / ajustar | 8192 | 5,383 Hz | 0,186 s | 0,372 s |
| Equilibrado | 65536 | 0,673 Hz | 1,486 s | 2,972 s |
| Preciso / validar | 262144 | 0,168 Hz | 5,944 s | 11,889 s |

Depois da confirmação, cada nova aquisição compatível pode atualizar a leitura uma vez por período. A interface é repintada até 15 vezes/s, mas isso não aumenta a quantidade de medições independentes. Uma mudança exige novas aquisições compatíveis; filtros com caudas longas podem demorar mais para estabilizar.

Em 48 kHz, os períodos são 0,171 / 1,365 / 5,461 s, e os espaçamentos são 5,859 / 0,732 / 0,183 Hz. Em 96 kHz, os números de amostras dobram para manter os mesmos espaçamentos e durações de 48 kHz. Instâncias novas começam no perfil Rápido; sessões anteriores preservam o perfil salvo. Confira o perfil nos dois componentes após atualizar.

## Leitura e exportação

Ganho é a visualização padrão, em dB. Ganho + fase abre um painel separado em graus. A fase é relativa ao pico dominante da resposta; não representa latência absoluta. Espectro usa dBFS e fica disponível com LIVE do analisador parado.

A escala automática vai até ±96 dB; há escalas manuais. O desenho preserva mínimos e máximos medidos entre colunas de pixels, mas não inventa resolução entre bins. Um pico situado entre bins ainda pode ser subestimado: use Preciso.

O cursor informa a frequência real do bin mais próximo. A prévia continua identificada como não confirmada. O CSV usa a curva confirmada existente no momento do clique em Exportar, mesmo que a medição mude enquanto a janela de arquivo estiver aberta.

## Interface e retomada

Arraste a alça inferior direita para variar de 70% a 160%. Dois cliques no logo NF restauram Capture para 1000 × 660 ou Probe para 1000 × 340.

LIVE e perfil ficam salvos com a instância. Ao recriar a cadeia, são necessárias novas aquisições para confirmar a resposta. Isso depende também da DAW restaurar o estado e o roteamento. Quedas de amostras invalidam a aquisição incompleta e reiniciam a confirmação. Hold mantém a curva, mas não interrompe o gerador do Probe.

## Captura única manual

É um modo distinto do LIVE. Pare LIVE no Probe e no Capture, deixe a pista em silêncio, arme Captura unica e envie um único impulso manual pelo Probe. Espere 65536 amostras. Não arme esse modo com o LIVE do Probe ativo.

O impulso manual continua com amplitude 0,25. A captura é normalizada por esse valor e inicia acima do limiar de 0,000001. Ruído, coeficientes anteriores ao limiar e caudas truncadas podem prejudicar o resultado. A fase é relativa ao início detectado, uma referência diferente da usada em LIVE.

## Limites

Mede somente o canal esquerdo/mono. O analisador deixa o áudio dos dois canais inalterado. Não calcula THD, não mede latência absoluta, não reproduz a equalização capturada e ainda não oferece captura calibrada de hardware. Curvas e referência A ficam na janela atual: exporte antes de fechar.

A R4 foi verificada com testes internos, incluindo mudanças de parâmetros durante as aquisições. Ainda requer confirmação dentro da Mixa. Repetibilidade e prévia curta não garantem a identificação correta de um sistema não linear, modulado ou periodicamente variável. Windows VST3 não foi compilado nesta entrega.

## Compilar

Requer CMake 3.22+, C++17, Xcode no Mac e JUCE 8.0.12, com sua licença aplicável.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
cmake --build build --config Release -j 4
```

NFVerify e NFVerifyProbe executam os testes internos. Consulte VALIDACAO.md.
