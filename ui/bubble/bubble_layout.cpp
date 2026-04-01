#include "bubble_layout.hpp"
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

    // ── Paso 1: ordenar hijos de mayor a menor radio ──────────────────────────
    std::vector<int> order(N);
    for (int i = 0; i < N; i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return draw_radii[a] > draw_radii[b];
        });

    // Gap proporcional al radio máximo para que escale con burbujas grandes
    float max_child_r = *std::max_element(draw_radii.begin(), draw_radii.end());
    const float gap = std::max(10.0f, max_child_r * 0.20f);

    // ── Paso 2: distribuir en anillos ─────────────────────────────────────────
    struct Ring {
        float orbit_r;
        std::vector<int> members;
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
            // Cada nodo ocupa diámetro + gap por cada lado
            float needed = 2.0f * draw_radii[idx] + gap * 2.0f;
            // Solo añadir si hay espacio real; si el anillo ya tiene miembros
            // y no cabe el siguiente, cerrar el anillo
            if (used_arc + needed > circ - gap && !ring.members.empty()) break;
            used_arc += needed;
            ring.members.push_back(idx);
        }

        placed += (int)ring.members.size();
        rings.push_back(ring);

        // Siguiente anillo: avanzar por el radio máximo de este anillo + gap
        float ring_max_r = 0.0f;
        for (int idx : ring.members)
            ring_max_r = std::max(ring_max_r, draw_radii[idx]);
        orbit += ring_max_r * 2.0f + gap * 3.5f;
    }

    // ── Paso 3: ángulos equidistantes por anillo ──────────────────────────────
    for (auto& ring : rings) {
        int   M = (int)ring.members.size();
        float angle_step = (M > 1) ? (2.0f * PI / M) : 0.0f;
        float start = -PI * 0.5f;

        for (int k = 0; k < M; k++) {
            int   idx = ring.members[k];
            float angle = start + k * angle_step;
            pos[idx].x = ring.orbit_r * cosf(angle);
            pos[idx].y = ring.orbit_r * sinf(angle);
            pos[idx].r = draw_radii[idx];
            pos[idx].node_idx = idx;
        }
    }

    // ── Paso 4: spring-relaxation (separación de colisiones) ─────────────────
    // Más iteraciones para radios grandes; fuerza de push más fuerte
    const int   ITERS = 40;
    const float PUSH_STRENGTH = 0.75f;

    for (int iter = 0; iter < ITERS; iter++) {
        // Enfriar la fuerza gradualmente para converger
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
        for (auto& ring : rings) {
            for (int idx : ring.members) {
                float len = sqrtf(pos[idx].x * pos[idx].x + pos[idx].y * pos[idx].y);
                if (len > 0.001f) {
                    float correction = (ring.orbit_r - len) * 0.12f;
                    pos[idx].x += (pos[idx].x / len) * correction;
                    pos[idx].y += (pos[idx].y / len) * correction;
                }
            }
        }
    }

    return pos;
}