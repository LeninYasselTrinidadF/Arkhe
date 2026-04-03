# Integración de kbnav_search / kbnav_info

Los tres archivos nuevos son:
  ui/key_controls/kbnav_search.hpp + .cpp
  ui/key_controls/kbnav_info.hpp   + .cpp

A continuación se listan los cambios mínimos necesarios en los archivos existentes.

---

## 1. app.hpp — añadir `kb_trigger` a LatexRenderJob

Busca la struct `LatexRenderJob` (o equivalente) y agrega:

```cpp
bool kb_trigger = false;   // ← NEW: teclado solicita render
```

---

## 2. info_description.cpp — consumir kb_trigger en el botón Render

En la sección que dibuja el botón "Render LaTeX", cambia:

```cpp
// ANTES:
if (btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    launch_latex_render(state, tex_target, cached_raw);

// DESPUÉS:
bool kb_fire = state.latex_render.kb_trigger;
state.latex_render.kb_trigger = false;
if ((btn_hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || kb_fire)
    launch_latex_render(state, tex_target, cached_raw);
```

Y registra el botón para la navegación por teclado **después** de dibujarlo:

```cpp
#include "key_controls/kbnav_info.hpp"
// ...
kbnav_info_register_render(btn_r);
```

---

## 3. info_crossrefs.cpp — registrar tarjetas de crossref y mathlib hits

### En `draw_mathlib_hit_card` — al final, antes del if(hov&&click):

```cpp
#include "key_controls/kbnav_info.hpp"
// ...
// Registrar para teclado (rect = card completo)
Rectangle card_r = { (float)rx, (float)ry, (float)cw, (float)ch };
kbnav_info_register_crossref(card_r, hit.module, InfoItemKind::MathlibHitCard);
```

### En `draw_crossrefs_block` — para cada card MSC:

```cpp
Rectangle card_r = { (float)rx, (float)ry, (float)card_w, (float)card_h };
kbnav_info_register_crossref(card_r, code, InfoItemKind::CrossrefCard);
```

---

## 4. info_resources.cpp — registrar recursos reales

En el bloque `else` de `draw_resources_block` (recursos reales),
al final del bucle por cada recurso:

```cpp
#include "key_controls/kbnav_info.hpp"
// ...
Rectangle card_r = { (float)rx, (float)ry, (float)card_w, (float)card_h };
bool is_web = (r.kind == "link");
kbnav_info_register_resource(card_r, r.content, is_web);
```

---

## 5. info_panel.cpp — frame begin + handle + draw

```cpp
#include "key_controls/kbnav_info.hpp"
```

Al inicio de `draw_info_panel`, ANTES de cualquier dibujo:

```cpp
kbnav_info_begin_frame();
```

Después del EndScissorMode y antes de dibujar el scrollbar:

```cpp
// Manejar teclado zona Info
const int SPLIT_MIN = TOOLBAR_H + 160;
const int SPLIT_MAX = SH() - 120;
kbnav_info_handle(state, SPLIT_MIN, SPLIT_MAX, g_split_y);

// Dibujar indicador de ítem activo
kbnav_info_draw();
```

---

## 6. search_panel.cpp — handle + draw

```cpp
#include "key_controls/kbnav_search.hpp"
```

Al INICIO de `draw_search_panel`, antes de cualquier dibujo:

```cpp
kbnav_search_handle(state, search_root);
```

Justo antes de `draw_search_field` para el campo local, captura la Y:

```cpp
int field_y_local = y;
```

Justo antes de `draw_search_field` para el campo loogle, captura la Y:

```cpp
int field_y_loogle = ly;   // 'ly' es la variable usada en el bloque loogle
```

Y captura las coords del botón Buscar:

```cpp
// La línea original:
Rectangle btn = { (float)(px + pw - btn_w), (float)ly, ... };
// Extrae:
int loogle_btn_x = px + pw - btn_w;
```

Al final de `draw_search_panel`, después de todo el contenido:

```cpp
kbnav_search_draw(px, pw,
                  field_y_local,
                  field_y_loogle,
                  field_h,
                  loogle_btn_x, btn_w);
```

Además, para que kbnav escriba en los buffers, el bloque de captura de
teclado de la sub-zona local debe ceder el paso cuando la zona Search está
activa por teclado:

```cpp
// ANTES (condición original):
if (panel_active) { ... leer teclado en search_buf ... }

// DESPUÉS: añadir OR con kbnav activo en Search:
if (panel_active || g_kbnav.in(FocusZone::Search)) {
    // El campo local solo procesa teclado si search_idx == 0
    bool local_kb_focus = g_kbnav.in(FocusZone::Search)
                          ? (g_kbnav.search_idx == 0)
                          : local_focus;   // lógica original
    if (local_kb_focus) { ... }
}
```

Sin embargo, como `kbnav_search_handle` ya escribe directamente en los
buffers, la forma más limpia es simplemente no duplicar la lógica y dejar
que el módulo kbnav_search sea la única fuente cuando la zona está activa.
En ese caso, guarda-restaura `local_focus = false` cuando el teclado toma
el control:

```cpp
// Al inicio del bloque de captura de teclado local:
if (g_kbnav.in(FocusZone::Search)) {
    local_focus  = false;   // kbnav_search_handle ya escribió
    loogle_focus = false;
}
```

---

## 7. CMakeLists / build — añadir los .cpp nuevos

```cmake
target_sources(arkhe PRIVATE
    ui/key_controls/kbnav_search.cpp
    ui/key_controls/kbnav_info.cpp
)
```

---

## Resumen de llamadas por frame

```
main loop
├── overlay::clear()
├── kbnav_handle_global(...)          ← ya existente
├── kbnav_search_handle(state, root)  ← NEW (antes de draw_search_panel)
│
├── BeginDrawing()
│   ├── draw_bubble_view / draw_dep_view
│   │   └── (selector canvas ya integrado en cada view)
│   ├── draw_search_panel(...)
│   │   └── kbnav_search_draw(...)   ← NEW (al final, dentro del panel)
│   ├── draw_info_panel(...)
│   │   ├── kbnav_info_begin_frame() ← NEW (inicio)
│   │   ├── ... registro de ítems durante el draw ...
│   │   ├── kbnav_info_handle(...)   ← NEW (post-scissor)
│   │   └── kbnav_info_draw()        ← NEW (post-scissor)
│   ├── draw_toolbar(...)
│   └── kbnav_draw_indicator()       ← ya existente
└── EndDrawing()
```
