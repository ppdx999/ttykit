#ifndef TTYKIT_LAYOUT_H
#define TTYKIT_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

// Rectangular area
typedef struct {
    uint16_t x;      // Left column
    uint16_t y;      // Top row
    uint16_t width;  // Width in cells
    uint16_t height; // Height in cells
} Rect;

// Split direction
typedef enum {
    DIRECTION_HORIZONTAL,  // Split left-to-right
    DIRECTION_VERTICAL     // Split top-to-bottom
} Direction;

// Constraint type
typedef enum {
    CONSTRAINT_LENGTH,   // Fixed size
    CONSTRAINT_PERCENT,  // Percentage of total (0-100)
    CONSTRAINT_RATIO,    // Ratio (value1/value2)
    CONSTRAINT_MIN,      // Minimum size, can grow
    CONSTRAINT_MAX,      // Maximum size, fills up to limit
    CONSTRAINT_FILL      // Fill remaining space (weighted)
} ConstraintType;

// Size constraint
typedef struct {
    ConstraintType type;
    uint16_t value1;  // Primary value
    uint16_t value2;  // Denominator for RATIO
} Constraint;

// Helper macros for creating constraints
#define CONSTRAINT_LEN(n)       ((Constraint){CONSTRAINT_LENGTH, (n), 0})
#define CONSTRAINT_PCT(n)       ((Constraint){CONSTRAINT_PERCENT, (n), 0})
#define CONSTRAINT_RATIO(a, b)  ((Constraint){CONSTRAINT_RATIO, (a), (b)})
#define CONSTRAINT_MIN_VAL(n)   ((Constraint){CONSTRAINT_MIN, (n), 0})
#define CONSTRAINT_MAX_VAL(n)   ((Constraint){CONSTRAINT_MAX, (n), 0})
#define CONSTRAINT_FILL_W(w)    ((Constraint){CONSTRAINT_FILL, (w), 0})

// Create a Rect from width and height (positioned at origin)
Rect rect_from_size(uint16_t width, uint16_t height);

// Check if Rect has zero area
int rect_is_empty(Rect r);

// Get intersection of two Rects
Rect rect_intersection(Rect a, Rect b);

// Split an area into multiple regions based on constraints
// Returns 0 on success, -1 on error (overflow, invalid ratio, etc.)
int layout_split(
    Rect area,
    Direction direction,
    const Constraint *constraints,
    size_t num_constraints,
    Rect *out_rects
);

#endif // TTYKIT_LAYOUT_H
