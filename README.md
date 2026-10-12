# Editor de Escenas 3D - Producto Academico 4

Aplicacion academica C++17 / OpenGL 2.1 de escritorio. Se abre vacia y permite crear y editar escenas durante la ejecucion. Incluye las dependencias fuente; no requiere descargarlas al configurar CMake.

## Abrir en CLion (Windows)

1. Extrae todo el ZIP en una ruta sencilla, por ejemplo `C:/Proyectos/Editor3D`. No ejecutes desde el ZIP.
2. En CLion selecciona **Open** y abre la carpeta que contiene `CMakeLists.txt`.
3. Si CLion pregunta, elige **Open as Project** y acepta el proyecto de confianza.
4. En **Settings > Build, Execution, Deployment > Toolchains**, utiliza el **MinGW incluido con CLion**, con CMake y Ninja incluidos. Si ya tienes un toolchain C++ funcional, puedes conservarlo. La disponibilidad del MinGW incluido depende de tu instalacion de CLion.
5. Espera la configuracion CMake. En el selector junto a Run elige **Editor3D**, no CoreTests.
6. Pulsa **Build** y luego **Run**. Debe aparecer la ventana del editor, inicialmente vacia.

No necesitas FreeGLUT, GLAD, Unity, Blender, Python ni instalar ImGui aparte. GLFW crea un contexto OpenGL 2.1 de compatibilidad; el backend de ImGui usado tambien es OpenGL2. Windows debe contar con un controlador de video que soporte OpenGL 2.1. No se utiliza un contexto core 3.3.

La ruta de recursos se establece en CMake al compilar. Si mueves la carpeta, vuelve a configurar y compilar. Para distribuir a otra PC entrega el proyecto y recompila; este paquete no contiene un ejecutable Windows comprobado.

## Si hay errores

- `No CMAKE_CXX_COMPILER`: configura el toolchain de C++ en CLion; un editor por si solo no es un compilador.
- GLFW no crea la ventana: revisa el controlador de video / soporte OpenGL, especialmente en maquinas virtuales.
- No encuentra `assets`: extrae el ZIP completo y recarga CMake.
- El programa cierra con un error: ejecutalo desde CLion y revisa la consola.
- Aparece CoreTests en vez del editor: cambia el target de ejecucion a Editor3D.
- OBJ no abre: introduce una ruta real al archivo, sin comillas. El boton comienza con la piramide incluida.
- Guardado: `escena.json` se crea en el directorio de trabajo de la configuracion Run; el mensaje informa la ruta absoluta. Puedes introducir una ruta completa a una carpeta existente.

## Uso

- Botones Cubo / Esfera / Cilindro / Plano: agregan objetos independientes.
- Clic en la lista o clic izquierdo sobre la geometria: selecciona.
- Propiedades: nombre, posicion, rotacion en grados, escala local por eje.
- Duplicar: copia el objeto seleccionado, conservando su padre y propiedades; no duplica sus descendientes.
- Eliminar: elimina el seleccionado y todos sus descendientes.
- Padre: asigna otra entidad o Sin padre. Los ciclos se rechazan. Cambiar de padre conserva la transformacion LOCAL; puede cambiar la posicion mundial.
- Material: color, ambiental, difusa, especular, brillo y tres texturas del proyecto.
- Iluminacion: despliega el apartado del panel derecho para cambiar posicion, color, ambiente global e intensidad.
- Camara: boton derecho y raton para orbitar; rueda para zoom; WASD desplaza; Q/E cambia altura; F enfoca seleccionado. El cursor debe estar sobre el viewport.
- Ortogonal / Wireframe / Cuadricula: controles superiores.
- Animacion: activar, elegir propiedad, fijar Desde/Hasta y duracion; despues Reproducir. Los valores son absolutos LOCALES e interpolados linealmente. Rotaciones Euler; no se implementa interpolacion de cuaterniones.
- Pausar conserva el tiempo. Detener vuelve al estado editable original y tiempo cero. Reiniciar lleva la vista animada a tiempo cero y conserva el estado de reproduccion.
- OBJ: acepta posiciones, normales y UV; triangula poligonos por abanico y soporta indices negativos. Para poligonos concavos conviene exportar triangulado. No importa archivos MTL.
- Colisiones: activa detectar y resaltar. Muestra pares y contorno rojo. Es AABB mundial conservador; puede indicar contacto aunque los triangulos no se toquen. No hay respuesta fisica.
- Guardar / Cargar: persistencia JSON de geometria OBJ, objetos, materiales, jerarquia, animaciones y luz. Los OBJ quedan embebidos, por lo que cargar no depende del archivo original. No guarda estado de camara ni tiempo de reproduccion.
- Ejemplo: crea un piso, una base animada, un cilindro hijo y una esfera. Nueva escena borra la escena en memoria sin confirmacion: guarda antes si deseas conservarla.

## Estructura

- `src/main.cpp`: ventana, bucle, temporizacion y ciclo de vida.
- `src/Editor.*`: controles de interfaz y acciones.
- `src/Scene.*`: entidades, transformaciones, jerarquias, animacion, colisiones y JSON.
- `src/Mesh.*`: primitivas e importacion OBJ.
- `src/Renderer.*`: camara, proyeccion, iluminacion, materiales, texturas y seleccion por rayos.
- `assets/`: tres texturas PPM y modelo OBJ.
- `tests/core_tests.cpp`: comprobaciones de comportamiento independientes de la GPU.
- `docs/`: informe PDF y Markdown, matriz de requisitos, video MP4 sin narracion y guion de exposicion.
- `vendor/`: GLFW 3.4, Dear ImGui 1.91.8, GLM 1.0.1 y nlohmann/json 3.11.3; licencias originales incluidas.

## Compilacion por terminal

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

En generadores de multiples configuraciones utiliza `--config Release` al compilar y `ctest -C Release`. En Linux se necesitan cabeceras OpenGL/X11, XRandR, Xinerama, XCursor y XI; en Ubuntu: `sudo apt install build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`.

