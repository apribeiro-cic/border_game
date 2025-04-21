# Jogo das Bordas 🎮

Este é um jogo desenvolvido para a placa de desenvolvimento **BitDogLab**, baseada no microcontrolador **Raspberry Pi Pico W**.  
O objetivo é simples, mas desafiador: mova o quadrado 8x8 exibido no display usando o joystick e posicione-o dentro da borda antes que o tempo acabe! O jogo irá ficar cada vez mais difícil conforme o tempo passa.

---

## 📌 Sobre o Projeto

O jogo foi criado como **projeto de revisão prática** da **2ª fase da residência tecnológica EmbarcaTech**.  
Ele integra diversos periféricos da placa BitDogLab, servindo como exercício completo de revisão.

---

## 🕹️ Como funciona

- Ao iniciar, pressione o **Botão A** para começar.
- Use o **joystick** para movimentar um quadrado 8×8 no display OLED.
- A cada 3 segundos, uma nova **borda** aparece em posição aleatória.
- Você precisa **mover o quadrado para dentro da borda** antes do tempo acabar.
- Se conseguir, ganha um ponto. Caso contrário: Game Over!
- O objetivo é alcançar **15 pontos** e vencer o jogo.

---

## 📁 Utilização

Atendendo aos novos requisitos de organização da 2° fase da residência, o arquivo CMakeList está mais enxuto, sendo necessário importar o projeto através da aba de Import Project da extensão do Raspberry Pi no VSCode. 
Segue imagens de instrução:

Clique na barra lateral em **Raspberry Pi Pico Project** e em **Import Project**

![image](https://github.com/user-attachments/assets/4b1ed8c7-6730-4bfe-ae1f-8a26017d1140)


Selecione o diretório e clique em **Import** (Lembre-se de usar a versão 2.1.0 do Pico SDK)

![image](https://github.com/user-attachments/assets/6348c657-9639-4218-88d1-5614b6eb2c2c)


Pronto! Resta apenas **compilar** e **rodar** o código, tendo a placa **BitDogLab** conectada.
