# Laboratorio de Sistemas Operativos y Distribuidos

Este proyecto es parte de un laboratorio de la materia **Sistemas Operativos y Distribuidos**. El objetivo es aprender y aplicar conceptos de sincronización de procesos utilizando memoria compartida y semáforos en un entorno de programación en C.

## Descripción

El programa simula un sistema de ataques a un monstruo utilizando diferentes tipos de ataques como espada, maza y flecha. Se emplean semáforos y memoria compartida para coordinar la comunicación entre procesos que atacan y un proceso que controla al monstruo.

### Funcionalidades Principales

- **Ataques Disponibles:** Espada, maza y flecha.
- **Sincronización:** Uso de semáforos para coordinar los turnos de ataque entre procesos.
- **Comunicación:** Uso de memoria compartida para almacenar la información sobre el estado del monstruo y los ataques.

## Requisitos

- Sistema operativo basado en UNIX (Linux o macOS).
- Compilador GCC.

## Compilación y Ejecución

1. Ejecutar el monstruo:

   ```bash
   ./server_alu_120056
   ```

1. Compila el código fuente con el siguiente comando:

   ```bash
   gcc -o lab_soyd guerrero_w_shm_sem.c
   ```

2. Ejecuta el programa con:

   ```bash
   ./lab_soyd
   ```

## Archivos Importantes

- **`guerrero_w_shm_sem.c`:** Código fuente principal que contiene la lógica de los ataques y la coordinación entre procesos.
- **`server_alu_120056`:** Ejecutable perteneciente al mosntruo.

## Detalles Técnicos

- **Memoria Compartida:** Se utiliza para almacenar la salud del monstruo y el estado de los ataques.
- **Semáforos:** Coordinan el acceso a los recursos compartidos, asegurando que los procesos no interfieran entre sí.
- **Códigos de Color ANSI:** Se utilizan para mostrar mensajes de forma visual en la terminal.

## Autor

- Nombre del autor: Franco Leon
- Correo electrónico: francoleondev@gmail.com