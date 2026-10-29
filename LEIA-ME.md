# NF Capture — beta 0.2 LIVE / revisão visual R2

NF Audio Tools. Interface roxa mais clara com títulos centralizados. Mac Intel e Apple Silicon: VST3 e AU.

## Atualizar a instalação

Feche a Mixa antes de substituir os plugins. Copie os dois arquivos .vst3 da pasta Mac para ~/Library/Audio/Plug-Ins/VST3 ou os dois arquivos .component para ~/Library/Audio/Plug-Ins/Components. Substitua a versão anterior. Reabra a Mixa e refaça a busca de plugins, se necessário. Não misture Probe 0.1 com Capture 0.2: atualize ambos.

Os arquivos têm assinatura local, sem notarização Apple. Não há instalação automática.

## Tamanho da interface

Arraste a alça no canto inferior direito para aumentar ou diminuir. A interface inteira mantém as proporções, incluindo fontes, gráfico e botões. O intervalo permitido é de 70% a 160% do tamanho padrão.

Dê dois cliques no logo NF, no canto superior esquerdo, para voltar ao tamanho padrão. Funciona também após reduzir a interface. Capture: 1000 × 660; Probe: 1000 × 340. A alteração desta revisão é visual; a identificação de versão do plugin permanece 0.2.0.

## Curva acompanhando os knobs (LIVE)

1. Crie uma pista sem arquivos de áudio, com ganho em 0 dB.
2. Insira nesta ordem: NF Capture Probe → seu plugin → NF Capture.
3. Ative o transporte da DAW, confirmando que a pista está processando áudio.
4. No Probe, clique em LIVE: iniciar.
5. No NF Capture, clique em LIVE: iniciar.
6. Mexa nos knobs do seu plugin: a curva violeta acompanha a resposta medida. Não precisa enviar impulsos manualmente.
7. Clique em Hold no NF Capture para congelar a curva. Clique em Retomar para voltar a acompanhar os knobs.
8. Use Guardar A para guardar uma referência na janela e comparar com os próximos ajustes. Exporte em CSV para preservar os valores.
9. Ao terminar, pare LIVE nos dois componentes. O Probe em LIVE substitui o áudio recebido pelo sinal de medição.

Comece com o plugin em teste em bypass: a resposta deve ser 0 dB e fase 0. Depois ative seu plugin. Mantenha a pista sem áudio e sem outros geradores. Desative seno, ruído analógico, modulação, saturação e ganho automático durante medições de EQ linear. O sinal de teste é audível; reduza o volume de monitoração durante o trabalho, sem alterar o nível entre Probe e Capture.

O LIVE usa um impulso de amplitude 0,25 a cada 8192 amostras. A 48 kHz, a leitura atualiza aproximadamente 5,86 vezes/s, com resolução de 5,86 Hz/bin. A 44,1 kHz: 5,38 vezes/s; a 96 kHz: 11,72 vezes/s. A interface pode acrescentar até um intervalo de atualização visual (cerca de 67 ms). A latência do plugin em teste também atrasa a leitura. Durante uma mudança, uma janela pode conter uma mistura das configurações anterior e nova; aguarde a estabilização para exportar.

## Captura única de alta resolução

Pare o LIVE no Probe. No Capture clique em Captura unica e, no Probe, em Enviar 1 impulso. Aguarde a aquisição de 65536 amostras (aproximadamente 1,37 s a 48 kHz). A captura única oferece 0,7324 Hz/bin a 48 kHz, mas não acompanha os knobs automaticamente. Não envie um segundo impulso enquanto a aquisição estiver em andamento. Aguarde terminar antes de mudar de modo.

## Gráfico e CSV

- Violeta: resposta em dB, escala de -36 a +36 dB.
- Turquesa: curva de referência A, mesma escala.
- Rosa: fase relativa, de -180 a +180 graus, com saltos de fase.
- Cinza: espectro, fora do modo LIVE, com janela Hann e escala de -120 a 0 dBFS.
- Eixo horizontal: 20 Hz a 20 kHz. Use taxa de pelo menos 44,1 kHz.

No LIVE, a fase é relativa à amostra de maior amplitude absoluta do período recebido. Na captura única, é relativa ao início detectado. Os dois métodos removem referências de tempo diferentes; não compare diretamente suas fases. Não há medição de latência absoluta. A magnitude em LIVE é calculada pela FFT de um período completo e normalizada pelo impulso de 0,25. A fase pode mudar de referência caso o pico dominante da resposta mude.

O CSV usa a resolução da captura atual. A curva A preserva sua própria grade, permitindo sobrepor uma captura única e uma leitura LIVE. Só estão disponíveis bins igualmente espaçados; a curva desenhada não acrescenta resolução entre eles.

## Limitações

- Mede apenas o canal esquerdo/mono. O analisador deixa os dois canais de áudio passarem sem alteração.
- LIVE mede a resposta periódica em regime estável de sistemas lineares e invariantes no tempo. Não identifica saturação, compressão, modulação ou ruído de forma equivalente a uma curva de EQ.
- O LIVE não reconhece automaticamente o Probe nem valida a origem do sinal. Música, seno ou outro sinal podem produzir curvas inválidas. Use exclusivamente o Probe em LIVE na cadeia dedicada.
- O Hold congela a visualização; o Probe continua enviando o sinal até que seja parado no próprio Probe.
- Curvas e referência A ficam somente na janela atual. Exporte antes de fechar. O modo LIVE é preservado ao reabrir a interface e ao salvar/restaurar as instâncias. Isso permite retomar a medição quando a DAW recria a cadeia ao trocar o plugin em teste. Em sessões salvas pela versão anterior, ative LIVE nos dois componentes uma vez após atualizar. O seno volta desligado ao restaurar a sessão.
- Captura única inicia acima do limiar de 0,000001 (-120 dBFS). Ruído e coeficientes iniciais abaixo do limiar podem prejudicar a leitura. Caudas que excedem o intervalo são truncadas.
- Não reproduz a equalização capturada, não calcula THD, não mede latência absoluta, não importa presets e ainda não tem captura calibrada de hardware.
- Não há binário Windows nesta entrega. O funcionamento dentro da Mixa ainda precisa ser confirmado.

## Código-fonte

Requer CMake 3.22+, compilador C++17 e Xcode no Mac. JUCE está incluído com sua licença original. A distribuição comercial deve cumprir a licença escolhida do JUCE.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'
cmake --build build --config Release -j 4
```

NFVerify verifica captura única, resposta LIVE, mudanças de ganho, Hold e retorno da atualização. NFVerifyProbe verifica impulso único, seno e a periodicidade LIVE com diferentes tamanhos de bloco.
