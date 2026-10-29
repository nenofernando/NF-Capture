# NF Capture — beta 0.2 LIVE / revisão R3

NF Audio Tools. VST3 e AU universais para Mac Intel e Apple Silicon. A revisão R3 melhora o método de medição, a resolução, o gráfico e as indicações de estado. O identificador de versão dos bundles permanece 0.2.0; a interface identifica R3.

## Atualizar

Feche a Mixa. Substitua AMBOS os componentes: NF Capture e NF Capture Probe. Não misture Probe R2 com Capture R3: o sinal LIVE mudou. Copie os arquivos .vst3 para ~/Library/Audio/Plug-Ins/VST3 ou .component para ~/Library/Audio/Plug-Ins/Components. Reabra a DAW e refaça a busca, se necessário. Os arquivos têm assinatura local e não têm notarização Apple.

## Medir acompanhando os knobs

1. Use uma pista sem arquivos de áudio, com ganho em 0 dB.
2. Ordem dos inserts: NF Capture Probe → plugin em teste → NF Capture.
3. Escolha o MESMO perfil nos menus dos dois componentes. Comece por Equilibrado. Para picos estreitos nos graves, use Preciso nos dois.
4. Ative o transporte da DAW e LIVE nos dois componentes.
5. Aguarde o indicador de preparação chegar a 100% e a curva estabilizar. Ajuste os knobs do seu plugin.
6. Hold congela a leitura. Retomar volta a acompanhar os ajustes. Guardar A mantém uma curva de referência na janela. Exportar CSV preserva os bins medidos.
7. Ao terminar, pare LIVE no Probe e no Capture.

Comece com o plugin em teste em bypass: a resposta deve indicar 0 dB. Depois ative o plugin. Desative ruído analógico, saturação e modulação para medir EQ linear. No caso de um equalizador dinâmico, desative a ação de compressão para comparar com sua curva estática.

O sinal de teste é audível. O Probe em LIVE substitui o áudio recebido por um multisseno periódico de aproximadamente -36,12 dBFS RMS. Ajuste a monitoração após o Capture; não altere o nível entre Probe e Capture. Boosts e processamentos não lineares podem exceder 0 dBFS; o analisador sinaliza o nível recebido, sem afirmar que o plugin tenha efetivamente clipado.

## Perfis: precisão e tempo de resposta

Em 44,1 kHz:

| Perfil | Amostras por janela | Espaçamento dos bins | Duração da janela |
|---|---:|---:|---:|
| Rápido | 8192 | 5,3833 Hz | 0,186 s |
| Equilibrado | 65536 | 0,6729 Hz | 1,486 s |
| Preciso | 262144 | 0,1682 Hz | 5,944 s |

Em 48 kHz: 5,8594 / 0,7324 / 0,1831 Hz por bin, respectivamente. Em 96 kHz, as janelas dobram de tamanho para manter os mesmos espaçamentos e durações de 48 kHz. Acima de 64 kHz, os tamanhos usados são 16384 / 131072 / 524288.

A interface atualiza até 15 vezes por segundo usando janelas sobrepostas. Isso não transforma uma janela de 5,94 s numa medição independente a cada 67 ms: após mudar o EQ, a janela ainda contém parte do áudio anterior. Quanto maior a precisão, mais tempo é necessário para refletir integralmente o novo ajuste. Aguarde estabilizar antes de exportar. “Estável entre janelas” indica pequenas mudanças entre leituras consecutivas; não certifica linearidade, ausência de distorção ou exatidão absoluta.

## Ganho, fase, espectro e cursor

- Ganho é a visualização padrão. A curva violeta usa dB; a curva A usa turquesa.
- Ganho + fase abre um painel separado em graus. Não confunda fase com ganho. A fase é relativa ao pico dominante da resposta estimada; não mede latência absoluta.
- Espectro mostra níveis por frequência em dBFS com o LIVE do analisador parado. Não é a curva de transferência de EQ.
- Escala auto adapta a faixa vertical em passos de 12 dB, até ±96 dB. Há também escalas manuais.
- Passe o mouse pelo gráfico: o cursor mostra a frequência real do bin mais próximo e seu ganho. No modo de fase, mostra também graus.
- O desenho liga os pontos e preserva mínimos/máximos medidos dentro de cada coluna de pixels. Essa apresentação não inventa resolução entre bins. Um pico situado entre bins ainda pode ser subestimado; use Preciso.
- O CSV contém os bins reais da captura, incluindo frequências fora da faixa visível até Nyquist. A curva A conserva sua própria grade e taxa de amostragem.

## Tamanho e aparência

Interface roxa, com nomes centralizados. Arraste a alça no canto inferior direito para variar entre 70% e 160%, preservando as proporções. Dois cliques no logo NF restauram 1000 × 660 no Capture ou 1000 × 340 no Probe.

## Captura única por impulso

Este é um modo diferente do LIVE. Pare LIVE no Probe e no Capture primeiro. Com silêncio na pista, clique em Captura unica no Capture e envie um único impulso pelo Probe. Aguarde 65536 amostras. Não arme a captura com o multisseno LIVE ainda ativo: isso não é uma resposta ao impulso válida.

O impulso tem amplitude 0,25 e a resposta é normalizada por esse valor. A captura inicia acima de 0,000001 (-120 dBFS); ruído e coeficientes anteriores ao limiar podem prejudicar a leitura. Caudas que excedem a janela são truncadas. A fase nesse modo é relativa ao início detectado, uma referência diferente da usada em LIVE.

## Retomada e limites

- O modo LIVE e o perfil ficam salvos com as instâncias. Ao recriar uma cadeia, a janela volta a ser preenchida antes de medir. O funcionamento depende também de a DAW restaurar o estado e manter o roteamento.
- Sessões antigas não continham todos esses campos: ative LIVE e confira os perfis uma vez após atualizar. O seno volta desligado ao restaurar a sessão.
- Perdas de amostras no buffer ou reinicialização do processamento invalidam a janela anterior e iniciam uma nova preparação.
- Hold congela a leitura, mas o Probe continua gerando áudio até ser parado.
- Curvas e referência A ficam apenas na janela atual. Exporte antes de fechar.
- Mede somente o canal esquerdo/mono. O analisador mantém os dois canais de áudio inalterados.
- Mede sistemas lineares em regime estável. Saturação, compressores, modulação, ruído ou um perfil diferente no Probe invalidam uma comparação com uma curva de EQ estática.
- Não calcula THD, não mede latência absoluta, não reproduz a equalização capturada e ainda não possui captura calibrada de hardware.
- A atualização foi validada com testes internos. Ainda precisa de confirmação na Mixa. Windows VST3 não foi compilado nesta entrega.

## Compilar

Requer CMake 3.22+, C++17, Xcode no Mac e JUCE 8.0.12. Respeite a licença do JUCE.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
cmake --build build --config Release -j 4
```

Os alvos NFVerify e NFVerifyProbe executam os testes internos. Veja VALIDACAO.md.
