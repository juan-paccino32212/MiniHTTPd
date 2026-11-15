# MiniHTTPd

Servidor web HTTP/1.1 básico desarrollado en lenguaje C para Linux, como proyecto de la asignatura de Computación Distribuida.

MiniHTTPd permite atender múltiples clientes de forma concurrente mediante **sockets TCP no bloqueantes y el mecanismo `epoll`**, procesar solicitudes HTTP y servir archivos estáticos.

## Características

- Servidor TCP sobre IPv4.
- Puerto de escucha: `8080`.
- Manejo de múltiples clientes mediante `epoll`.
- Operaciones de lectura y escritura no bloqueantes.
- Procesamiento de solicitudes HTTP/1.1.
- Soporte para el método `GET`.
- Validación del encabezado `Host`.
- Manejo de conexiones persistentes (`Keep-Alive`).
- Soporte para múltiples solicitudes en una misma conexión.
- Envío de archivos estáticos HTML, CSS, JavaScript, imágenes y otros formatos compatibles.
- Detección del tipo MIME según la extensión del archivo.
- Respuestas con códigos de estado HTTP, incluyendo `200`, `400`, `403`, `404`, `405` y `500`.
- Validación de rutas para evitar ataques de Directory Traversal.
- Límites para el tamaño de las solicitudes y sus encabezados.

## Tecnologías utilizadas

- Lenguaje C.
- Linux / WSL2.
- Sockets BSD.
- `epoll` para multiplexación de entrada/salida.
- GNU Compiler Collection (GCC).
- GNU Make.

## Arquitectura del proyecto

El servidor está organizado en cinco módulos principales:

| Módulo | Responsabilidad |
|---|---|
| `main.c` | Punto de entrada del programa. |
| `server.c` | Creación del servidor, gestión de clientes, sockets y ciclo de eventos `epoll`. |
| `http.c` | Análisis y validación de solicitudes HTTP. |
| `files.c` | Resolución segura de rutas y lectura de archivos. |
| `mime.c` | Identificación del tipo MIME de los archivos. |

Los archivos de cabecera correspondientes se encuentran en `include/`.

### Estructura de directorios

```text
MiniHTTPd/
├── Makefile
├── README.md
├── include/
│   ├── files.h
│   ├── http.h
│   ├── mime.h
│   └── server.h
├── src/
│   ├── files.c
│   ├── http.c
│   ├── main.c
│   ├── mime.c
│   └── server.c
├── www/
│   ├── index.html
│   ├── style.css
│   └── image.png
└── build/
```

La carpeta `build/` se genera automáticamente durante la compilación y contiene los archivos objeto y las dependencias generadas por GCC.

## Compilación

### Requisitos

Se necesita un entorno Linux con:

- GCC.
- GNU Make.
- Bibliotecas estándar de Linux y C.

### Construir el proyecto

Desde el directorio raíz:

```bash
make
```

El ejecutable generado será:

```text
minihttpd
```

### Ejecutar el servidor

```bash
make run
```

También se puede ejecutar directamente:

```bash
./minihttpd
```

El servidor escuchará en el puerto `8080`.

Para detenerlo, utiliza `Ctrl + C`.

### Compilación en modo debug

```bash
make debug
```

Este objetivo recompila el proyecto con las opciones `-g -O0`.

### Limpiar archivos generados

```bash
make clean
```

Elimina el ejecutable y la carpeta `build/`.

## Pruebas

Con el servidor en ejecución, se pueden realizar solicitudes desde otra terminal.

### Solicitar la página principal

```bash
curl -i http://127.0.0.1:8080/
```

### Solicitar una hoja de estilos

```bash
curl -i http://127.0.0.1:8080/style.css
```

### Solicitar una imagen

```bash
curl -i http://127.0.0.1:8080/image.png
```

### Probar un recurso inexistente

```bash
curl -i http://127.0.0.1:8080/no-existe.html
```

### Probar un método no soportado

```bash
curl -i -X POST http://127.0.0.1:8080/
```

### Probar conexiones persistentes

Se realizaron pruebas mediante un script de Python para enviar varias solicitudes HTTP a través de una misma conexión TCP.

Las pruebas verificaron:

- Una solicitud individual.
- Tres solicitudes consecutivas en una conexión.
- Cinco solicitudes consecutivas en una conexión.
- Cierre de conexión mediante `Connection: close`.

Las pruebas obtuvieron respuestas HTTP `200 OK` y comprobaron la recepción correcta de los cuerpos de respuesta.

## Consideraciones y limitaciones

MiniHTTPd es una implementación educativa de un servidor HTTP/1.1 básico. No pretende sustituir a servidores web de producción.

Entre sus limitaciones se encuentran:

- Solo implementa el método `GET`.
- No implementa HTTPS ni HTTP/2.
- No procesa cuerpos de solicitudes ni soporta `Transfer-Encoding: chunked`.
- La disponibilidad de tipos MIME depende de las extensiones incluidas en su tabla.
- Los archivos se leen completamente en memoria antes de enviarse al cliente.

## Autor

Proyecto académico desarrollado para la asignatura de Computación Distribuida.
