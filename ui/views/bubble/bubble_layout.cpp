#include "ui/views/bubble/bubble_layout.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

// ── compute_layout ────────────────────────────────────────────────────────────

std::vector<BubblePos> compute_layout(
    const MathNode* cur,
    const std::vector<float>& draw_radii,
    float                     center_r)
{
    const int N = (int)cur->children.size();
    std::vector<BubblePos> pos(N);
    if (N == 0) return pos;

    // ── Paso 1: ordenar hijos alfabéticamente por label ───────────────────────
    // El orden es estable (no depende de pesos ni de sesión anterior).
    // `order[k]` = índice original en cur->children del k-ésimo nodo en orden alfa.
    std::vector<int> order(N);
    for (int i = 0; i < N; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return cur->children[a]->label < cur->children[b]->label;
        });

    // Gap proporcional al radio máximo para que escale con burbujas grandes
    float max_child_r = *std::max_element(draw_radii.begin(), draw_radii.end());
    const float gap = std::max(10.0f, max_child_r * 0.20f);

    // ── Paso 2: distribuir en anillos ─────────────────────────────────────────
    // Dentro de cada anillo los nodos van en el orden alfabético establecido.
    struct Ring {
        float orbit_r;
        std::vector<int> members;  // índices originales, en orden alfabético
    };
    std::vector<Ring> rings;

    float orbit = center_r + max_child_r + gap * 2.5f;
    int   placed = 0;

    while (placed < N) {
        Ring ring;
        ring.orbit_r = orbit;

        float used_arc = 0.0f;
        float circ = 2.0f * PI * orbit;

        for (int i = placed; i < N; i++) {
            int   idx = order[i];
            float needed = 2.0f * draw_radii[idx] + gap * 2.0f;
            if (used_arc + needed > circ - gap && !ring.members.empty()) break;
            used_arc += needed;
            ring.members.push_back(idx);
        }

        placed += (int)ring.members.size();
        rings.push_back(ring);

        float ring_max_r = 0.0f;
        for (int idx : ring.members)
            ring_max_r = std::max(ring_max_r, draw_radii[idx]);
        orbit += ring_max_r * 2.0f + gap * 3.5f;
    }

    // ── Paso 3: ángulos equidistantes por anillo ──────────────────────────────
    // Empezamos en -90° (arriba) y avanzamos en sentido horario.
    // slot_idx = posición dentro del anillo (0 = primero arriba, sentido horario).
    for (int ri = 0; ri < (int)rings.size(); ri++) {
        auto& ring = rings[ri];
        int   M = (int)ring.members.size();
        float step = (M > 1) ? (2.0f * PI / M) : 0.0f;
        float start = -PI * 0.5f;

        for (int k = 0; k < M; k++) {
            int   idx = ring.members[k];
            float angle = start + k * step;
            pos[idx].x = ring.orbit_r * cosf(angle);
            pos[idx].y = ring.orbit_r * sinf(angle);
            pos[idx].r = draw_radii[idx];
            pos[idx].node_idx = idx;
            pos[idx].ring_idx = ri + 1;   // anillos numerados desde 1
            pos[idx].slot_idx = k;        // posición horaria dentro del anillo
        }
    }

    // ── Paso 4: spring-relaxation (separación de colisiones) ─────────────────
    const int   ITERS = 40;
    const float PUSH_STRENGTH = 0.75f;

    for (int iter = 0; iter < ITERS; iter++) {
        float cooling = 1.0f - (float)iter / (ITERS * 1.5f);

        for (int i = 0; i < N; i++) {
            for (int j = i + 1; j < N; j++) {
                float dx = pos[j].x - pos[i].x;
                float dy = pos[j].y - pos[i].y;
                float dist2 = dx * dx + dy * dy;
                float min_dist = pos[i].r + pos[j].r + gap;

                if (dist2 < min_dist * min_dist && dist2 > 0.0001f) {
                    float dist = sqrtf(dist2);
                    float overlap = min_dist - dist;
                    float nx = dx / dist, ny = dy / dist;
                    float push = overlap * PUSH_STRENGTH * cooling * 0.5f;
                    pos[i].x -= nx * push;  pos[i].y -= ny * push;
                    pos[j].x += nx * push;  pos[j].y += ny * push;
                }
            }
        }

        // Re-proyección suave a la órbita de cada anillo
        for (int ri = 0; ri < (int)rings.size(); ri++) {
            for (int idx : rings[ri].members) {
                float len = sqrtf(pos[idx].x * pos[idx].x + pos[idx].y * pos[idx].y);
                if (len > 0.001f) {
                    float correction = (rings[ri].orbit_r - len) * 0.12f;
                    pos[idx].x += (pos[idx].x / len) * correction;
                    pos[idx].y += (pos[idx].y / len) * correction;
                }
            }
        }
    }

    return pos;
}