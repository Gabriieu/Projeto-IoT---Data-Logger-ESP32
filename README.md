# 📡 IoT Portal – MVC + ESP32 + FIWARE

Sistema completo de monitoramento e controle IoT utilizando **ASP.NET MVC**, **ESP32** e **FIWARE**, permitindo acompanhamento em tempo real de sensores, visualização gráfica e alertas automáticos.

---

## 🧠 Visão Geral do Projeto

Este projeto integra três camadas principais:

* 🌐 **Portal Web MVC (.NET)** – Interface de monitoramento e controle
* 🔌 **ESP32** – Dispositivo IoT com sensores
* ☁️ **FIWARE** – Plataforma intermediária para gerenciamento de contexto

O sistema é capaz de:

* Exibir temperatura, umidade e luminosidade em tempo real
* Definir limites críticos para cada sensor
* Emitir alertas visuais
* Acionar buzzer automaticamente ao ultrapassar limites
* Visualizar localização do dispositivo em mapa
* Mostrar previsão do tempo local

---

## ⚙️ Tecnologias Utilizadas

### Dashboard

* ASP.NET MVC
* Entity Framework Core
* SQL Server
* Bootstrap 5
* Chart.js
* Leaflet.js
* JavaScript

### IoT

* ESP32
* MQTT

### Plataforma

* FIWARE Orion Context Broker
* FIWARE IoT Agent MQTT
* FIWARE STH-Comet

---

## 📋 Funcionalidades

✔ Monitoramento de sensores em tempo real
✔ Exibição gráfica com histórico
✔ Alertas visuais de limite
✔ Buzzer automático em situação crítica
✔ Geolocalização do dispositivo
✔ Previsão meteorológica integrada

---

## 🔌 Firmware ESP32 (Resumo)

O ESP32 envia dados via MQTT:

E recebe comandos:

* on → Liga LED
* off → Desliga LED
* buzzer_on → Ativa buzzer
* buzzer_off → Desativa buzzer

---

## 🔔 Sistema de Alertas

O sistema verifica continuamente se algum valor ultrapassa o limite configurado.

Se ultrapassar:

```json
{
  "buzzer_on": {
    "type": "command",
    "value": "buzzer_on"
  }
}
```

Caso contrário:

```json
{
  "buzzer_off": {
    "type": "command",
    "value": "buzzer_off"
  }
}
```

---

## 📊 Telas do Sistema

* ✅ Lista de dispositivos
* ✅ Tela de detalhes com gráfico em tempo real
* ✅ Mapa interativo
* ✅ Clima local
* ✅ Alertas visuais

---

## Autores

![Autores](images/alunos.jpg)

---

## Dashboard do Sistema

![Dashboard do Portal IoT](images/dashboard.jpg)

---

## Gráficos

![Temperatura](images/temperature.jpg)
![Humidade](images/humidity.jpg)
![Luminosidade](images/luminosity.jpg)

---