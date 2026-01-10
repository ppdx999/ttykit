#include "layout.h"

Rect rect_from_size(uint16_t width, uint16_t height) {
    return (Rect){0, 0, width, height};
}

int rect_is_empty(Rect r) {
    return r.width == 0 || r.height == 0;
}

Rect rect_intersection(Rect a, Rect b) {
    uint16_t x1 = a.x > b.x ? a.x : b.x;
    uint16_t y1 = a.y > b.y ? a.y : b.y;
    uint16_t x2_a = a.x + a.width;
    uint16_t x2_b = b.x + b.width;
    uint16_t y2_a = a.y + a.height;
    uint16_t y2_b = b.y + b.height;
    uint16_t x2 = x2_a < x2_b ? x2_a : x2_b;
    uint16_t y2 = y2_a < y2_b ? y2_a : y2_b;

    if (x1 >= x2 || y1 >= y2) {
        return (Rect){0, 0, 0, 0};
    }
    return (Rect){x1, y1, x2 - x1, y2 - y1};
}

int layout_split(
    Rect area,
    Direction direction,
    const Constraint *constraints,
    size_t num_constraints,
    Rect *out_rects
) {
    if (num_constraints == 0) {
        return 0;
    }

    uint16_t total_size = (direction == DIRECTION_HORIZONTAL)
                          ? area.width : area.height;

    // Temporary array for computed sizes
    uint16_t sizes[num_constraints];
    int is_flexible[num_constraints];  // 1 if Min/Max/Fill

    // Phase 1: Calculate fixed sizes and mark flexible constraints
    uint16_t fixed_total = 0;
    uint16_t fill_weight_total = 0;
    uint16_t min_total = 0;

    for (size_t i = 0; i < num_constraints; i++) {
        const Constraint *c = &constraints[i];
        sizes[i] = 0;
        is_flexible[i] = 0;

        switch (c->type) {
            case CONSTRAINT_LENGTH:
                sizes[i] = c->value1;
                fixed_total += sizes[i];
                break;

            case CONSTRAINT_PERCENT:
                sizes[i] = (uint16_t)((uint32_t)total_size * c->value1 / 100);
                fixed_total += sizes[i];
                break;

            case CONSTRAINT_RATIO:
                if (c->value2 == 0) {
                    return -1;  // Division by zero
                }
                sizes[i] = (uint16_t)((uint32_t)total_size * c->value1 / c->value2);
                fixed_total += sizes[i];
                break;

            case CONSTRAINT_MIN:
                sizes[i] = c->value1;  // Start with minimum
                min_total += c->value1;
                is_flexible[i] = 1;
                break;

            case CONSTRAINT_MAX:
                sizes[i] = 0;  // Start with 0, will fill up to max
                is_flexible[i] = 1;
                break;

            case CONSTRAINT_FILL:
                sizes[i] = 0;
                fill_weight_total += c->value1;
                is_flexible[i] = 1;
                break;
        }
    }

    // Phase 2: Check for overflow
    if (fixed_total + min_total > total_size) {
        return -1;  // Overflow
    }

    // Phase 3: Distribute remaining space to flexible constraints
    uint16_t remaining = total_size - fixed_total - min_total;

    // First pass: Fill up MAX constraints
    for (size_t i = 0; i < num_constraints && remaining > 0; i++) {
        if (constraints[i].type == CONSTRAINT_MAX) {
            uint16_t can_take = constraints[i].value1;
            if (can_take > remaining) {
                can_take = remaining;
            }
            sizes[i] = can_take;
            remaining -= can_take;
        }
    }

    // Second pass: Distribute to FILL constraints by weight
    if (fill_weight_total > 0 && remaining > 0) {
        uint16_t fill_remaining = remaining;
        for (size_t i = 0; i < num_constraints; i++) {
            if (constraints[i].type == CONSTRAINT_FILL) {
                uint16_t share = (uint16_t)((uint32_t)fill_remaining *
                                 constraints[i].value1 / fill_weight_total);
                sizes[i] = share;
                remaining -= share;
            }
        }
        // Give any rounding remainder to the last FILL
        for (size_t i = num_constraints; i > 0 && remaining > 0; i--) {
            if (constraints[i - 1].type == CONSTRAINT_FILL) {
                sizes[i - 1] += remaining;
                remaining = 0;
                break;
            }
        }
    }

    // Third pass: Give remaining to MIN constraints (proportionally)
    if (remaining > 0) {
        size_t min_count = 0;
        for (size_t i = 0; i < num_constraints; i++) {
            if (constraints[i].type == CONSTRAINT_MIN) {
                min_count++;
            }
        }
        if (min_count > 0) {
            uint16_t extra_each = remaining / min_count;
            uint16_t extra_remainder = remaining % min_count;
            for (size_t i = 0; i < num_constraints; i++) {
                if (constraints[i].type == CONSTRAINT_MIN) {
                    sizes[i] += extra_each;
                    if (extra_remainder > 0) {
                        sizes[i]++;
                        extra_remainder--;
                    }
                }
            }
        }
    }

    // Phase 4: Build output Rects
    uint16_t pos = 0;
    for (size_t i = 0; i < num_constraints; i++) {
        if (direction == DIRECTION_HORIZONTAL) {
            out_rects[i].x = area.x + pos;
            out_rects[i].y = area.y;
            out_rects[i].width = sizes[i];
            out_rects[i].height = area.height;
        } else {
            out_rects[i].x = area.x;
            out_rects[i].y = area.y + pos;
            out_rects[i].width = area.width;
            out_rects[i].height = sizes[i];
        }
        pos += sizes[i];
    }

    return 0;
}
