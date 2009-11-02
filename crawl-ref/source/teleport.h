#ifndef TELEPORT_H
#define TELEPORT_H

bool random_near_space(const coord_def& origin, coord_def& target,
                       bool allow_adjacent = false, bool restrict_LOS = true,
                       bool forbid_dangerous = true,
                       bool forbid_sanctuary = false);

#endif
