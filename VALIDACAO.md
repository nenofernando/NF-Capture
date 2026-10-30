# Validação — NF Capture R4

## Correção testada

O LIVE passou do multisseno da R3 para impulsos isolados de amplitude 0,03125, em períodos completos independentes. A confirmação compara os próprios bins de duas aquisições consecutivas: até 0,10 dB de diferença em magnitude e até 0,5 grau de fase na faixa útil (fase desconsiderada abaixo de -100 dB). Não há média ou suavização em frequência dos dados confirmados.

Uma prévia de resposta curta pode acompanhar ajustes antes dessa confirmação. Ela é identificada na interface e não é exportável. A indicação é baseada na energia da resposta temporal fora de uma região de até aproximadamente ±50 ms em torno do pico, limitada a um terço do período: a energia exterior deve ser inferior a 2% da energia que não pertence à amostra do pico dominante. Isso é um critério de suporte curto, não uma prova de invariância no tempo ou de linearidade.

## Testes internos

- Ganhos de +30 e -30 dB a 103,816 Hz / Q 24, nos três perfis em 44,1 / 48 / 96 kHz. Todos os bins úteis comparados à resposta matemática após estabilização; tolerância de 0,02 dB.
- Captura única FIR em nove combinações de taxas de amostragem e tamanhos de bloco.
- Atenuação de -6,0206 dB e fase relativa zero, com deslocamento circular do sinal.
- Continuidade amostra a amostra dos impulsos nos três perfis através de blocos de 64 / 511 / 1024 amostras.
- Mudanças de frequência, ganho e Q durante uma sequência de processamento com a memória do filtro preservada. Seis mudanças de frequência instáveis foram rejeitadas como curvas confirmadas em cada perfil; quatro permitiram prévia de resposta curta.
- Mudança de parâmetros 30 ms depois do início de uma aquisição: a janela mista foi rejeitada como confirmação nos três perfis e o analisador se recuperou após estabilizar.
- Ao rejeitar uma aquisição, o vetor confirmado anterior permaneceu inalterado. Depois de estabilizar, a confirmação foi retomada, com erro inferior a 0,02 dB em relação ao filtro final.
- Integração da interface: prévia acompanha a mudança de ganho antes de confirmar; dados confirmados permanecem separados; exportação fica desativada enquanto a leitura é apenas prévia.
- Hold/Retomar, restauração de LIVE e perfil, recuperação após overflow real do FIFO e passagem de áudio inalterada no analisador.
- Redimensionamento, alça, coordenadas de clique e restauração pelo logo nos dois componentes.

Esses testes usam filtros e sinais sintéticos; não são um teste dentro da Mixa nem com o binário do NF Q Dynamics mostrado nas capturas. A confirmação atesta repetibilidade dentro dos limites definidos, não exatidão absoluta e não o comportamento instantâneo de um sistema que muda rapidamente ou periodicamente.

## Instalador

O pacote .pkg usa o domínio CurrentUserHomeDirectory, sem instalação em LocalSystem. Contém apenas os quatro bundles VST3/AU de Capture e Probe, com substituição de suas pastas e sem scripts de instalação. As arquiteturas Intel e Apple Silicon e as assinaturas ad-hoc dos bundles foram verificadas. O instalador não possui assinatura Developer ID Installer nem notarização Apple.

O conteúdo e os metadados do pacote foram inspecionados sem executar a instalação. Instalação e funcionamento desta revisão ainda precisam ser confirmados pelo usuário na Mixa. Windows VST3 não foi compilado.
