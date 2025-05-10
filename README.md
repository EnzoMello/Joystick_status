# 🕹️ Joystick Status - Leitura e Envio via Wi-Fi

Este projeto tem como objetivo realizar a leitura dos eixos **X** e **Y** de um joystick analógico conectado ao microcontrolador **RP2040 (BitDogLab)**. A posição lida é processada para determinar a **direção** (Norte, Sul, Leste, Oeste e diagonais) e os dados são enviados a cada segundo para um **servidor web via conexão Wi-Fi**, sendo exibidos em uma **interface HTML**.

---

## 🔧 Tecnologias Utilizadas

- **C (Pico SDK)** – Leitura dos dados analógicos do joystick
- **RP2040 (BitDogLab)** – Microcontrolador com suporte a multitarefa
- **Wi-Fi (ESP via UART ou módulo nativo)** – Comunicação com o servidor
- **Protocolo TCP/IP** – Envio dos dados em formato JSON
- **HTML/CSS** – Visualização dos dados em tempo real

---

## 📌 Funcionalidades

- Leitura contínua dos eixos analógicos X e Y do joystick
- Determinação da **direção** com base na posição (Ex: Norte, Nordeste, etc.)
- Geração de um objeto JSON com os valores dos eixos e direção
- Envio periódico dos dados ao servidor via requisição HTTP
- Exibição dos valores e da direção atual em uma **página HTML**

---

## 💡 Direções possíveis da "rosa dos ventos":

| Direção   | Condição dos Eixos X e Y |
|-----------|---------------------------|
| Norte     | Y > limiar superior       |
| Sul       | Y < limiar inferior       |
| Leste     | X > limiar superior       |
| Oeste     | X < limiar inferior       |
| Nordeste  | X > limiar sup & Y > limiar sup |
| Sudeste   | X > limiar sup & Y < limiar inf |
| Noroeste  | X < limiar inf & Y > limiar sup |
| Sudoeste  | X < limiar inf & Y < limiar inf |

---

## 🚀 Como executar o projeto

1. Clone o repositório:
```bash
git clone https://github.com/EnzoMello/Joystick_status.git
