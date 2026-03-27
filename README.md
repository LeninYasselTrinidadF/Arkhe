# Arkhe

**Arkhe** es un explorador visual y editor de estructuras matemáticas formales. El proyecto combina la precisión de la lógica formal con una interfaz dinámica basada en física de partículas y grafos, utilizando un motor de renderizado de alto rendimiento para navegar a través de taxonomías complejas.

## Características Principales

### Navegación Multimodal
Visualización de nodos optimizada en tres modos principales:
* **MSC2020**: Exploración de áreas de investigación según la clasificación de la **AMS** (Mathematics Subject Classification).
* **Mathlib**: Navegación por la jerarquía de archivos y declaraciones de la librería formal de **Lean4**.
* **Estándar**: Temarios académicos personalizados que conectan conceptos entre diversas librerías.

### Editor de Entradas Integrado
Permite la gestión y edición en tiempo real de los datos asociados a cada nodo matemático dentro del grafo.

## Estructura del Proyecto

El código se organiza de forma modular para facilitar la escalabilidad del sistema:

* **`/data`**: Cargadores (*loaders*) especializados para MSC, Mathlib y sistemas de referencias cruzadas.
* **`/ui`**: Componentes de la interfaz de usuario, incluyendo el *Toolbar*, paneles de búsqueda y paneles de información.
* **`/search`**: Motores de búsqueda lógica, incluyendo la integración con herramientas como **Loogle**.
* **`/assets`**:
    * **`/graphics`**: Almacenamiento de *sprites* e iconos del sistema.
    * **`/entries`**: Repositorio de archivos `.tex` y descripciones detalladas generadas por el editor.
