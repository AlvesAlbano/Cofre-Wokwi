# WOKWI
![image](https://github.com/AlvesAlbano/CofreWokwi/assets/148901882/e6e611b5-2696-40e6-a64b-6f2c10ec7fdb) 

Um cofre eletrônico desenvolvido no simulador WOKWI utilizando um microcontrolador ESP32 em conjunto com os seguintes componentes: teclado (Keypad), motor servo (Servo Motor), display de cristal líquido com comunicação I2C (LiquidCrystal I2C) e dois botões, os quais desempenham as funções de um cofre eletrônico. O cofre está integrado a dois brokers MQTT, permitindo a alteração da senha e o bloqueio remoto por meio do Node-RED. Além disso, o ESP32 publica mensagens em um broker MQTT para registro histórico no Node-RED.


# NODE RED
![image](https://github.com/AlvesAlbano/CofreWokwi/assets/148901882/fada0ae5-d3d8-4665-b8ea-0b7958d81147)

No Node-RED, foram utilizadas duas bibliotecas: 'node-red-dashboard' e 'node-red-node-ui-table', que representam, respectivamente, um painel de controle e uma função de tabela. No painel de controle, é possível bloquear e desbloquear remotamente o cofre, além de alterar sua senha. Na tabela, é possível visualizar o histórico de acesso.
