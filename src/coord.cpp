#include <cmath>
#include <ostream>

#include "morph.h"

Direction randomDirection() {
    int i = globalRNG.upto(8);
    return static_cast<Direction>(i + 2);
}


Direction randomCardinalDirection() {
    switch(globalRNG.upto(4)) {
        case 0: return Direction::North;
        case 1: return Direction::East;
        case 2: return Direction::South;
        case 3: return Direction::West;
        default: return Direction::North;
    }
}


Direction rotate90(Direction d) {
    switch(d) {
        case Direction::Unknown:
        case Direction::Here:       return d;
        case Direction::North:      return Direction::East;
        case Direction::Northeast:  return Direction::Southeast;
        case Direction::East:       return Direction::South;
        case Direction::Southeast:  return Direction::Southwest;
        case Direction::South:      return Direction::West;
        case Direction::Southwest:  return Direction::Northwest;
        case Direction::West:       return Direction::North;
        case Direction::Northwest:  return Direction::Northeast;
        default:
            return d;
    }
}

Direction rotate45(Direction d) {
    switch(d) {
        case Direction::Unknown:
        case Direction::Here:       return d;
        case Direction::North:      return Direction::Northeast;
        case Direction::Northeast:  return Direction::East;
        case Direction::East:       return Direction::Southeast;
        case Direction::Southeast:  return Direction::South;
        case Direction::South:      return Direction::Southwest;
        case Direction::Southwest:  return Direction::West;
        case Direction::West:       return Direction::Northwest;
        case Direction::Northwest:  return Direction::North;
        default:
            return d;
    }
}

Direction unrotate45(Direction d) {
    switch(d) {
        case Direction::Unknown:
        case Direction::Here:       return d;
        case Direction::North:      return Direction::Northwest;
        case Direction::Northwest:  return Direction::West;
        case Direction::West:       return Direction::Southwest;
        case Direction::Southwest:  return Direction::South;
        case Direction::South:      return Direction::Southeast;
        case Direction::Southeast:  return Direction::East;
        case Direction::East:       return Direction::Northeast;
        case Direction::Northeast:  return Direction::North;
        default:
            return d;
    }
}


std::ostream& operator<<(std::ostream &out, Direction d) {
    switch(d) {
        case Direction::Unknown:    out << "unknown";   break;
        case Direction::Here:       out << "here";      break;
        case Direction::North:      out << "north";     break;
        case Direction::Northeast:  out << "northeast"; break;
        case Direction::East:       out << "east";      break;
        case Direction::Southeast:  out << "southeast"; break;
        case Direction::South:      out << "south";     break;
        case Direction::Southwest:  out << "southwest"; break;
        case Direction::West:       out << "west";      break;
        case Direction::Northwest:  out << "northwest"; break;
        default: out << "(unknown direction " << static_cast<int>(d) << ')';
    }
    return out;
}


bool Coord::operator==(const Coord &rhs) const {
    return x == rhs.x && y == rhs.y;
}

Coord Coord::shift(Direction d, int amount) const {
    switch(d) {
        case Direction::North:      return Coord(x,          y - amount);
        case Direction::Northeast:  return Coord(x + amount, y - amount);
        case Direction::East:       return Coord(x + amount, y);
        case Direction::Southeast:  return Coord(x + amount, y + amount);
        case Direction::South:      return Coord(x,          y + amount);
        case Direction::Southwest:  return Coord(x - amount, y + amount);
        case Direction::West:       return Coord(x - amount, y);
        case Direction::Northwest:  return Coord(x - amount, y - amount);
        case Direction::Unknown:
        case Direction::Here:
        default:
            return *this;
    }
}

Direction Coord::directionTo(const Coord &to) const {
    if (to == *this) return Direction::Here;

    double dx = to.x - x;
    double dy = to.y - y;
    double angleInRadians = std::atan2(dy, dx);
    double angleInDegrees = angleInRadians * 57.2957795131;
    while (angleInDegrees < 0) angleInDegrees += 360;
    while (angleInDegrees >= 360) angleInDegrees -= 360;
    // 0deg is pointing right

    if (angleInDegrees >= 337.5 || angleInDegrees <=  22.5) return Direction::East;
    if (angleInDegrees >=  22.5 && angleInDegrees <=  67.5) return Direction::Southeast;
    if (angleInDegrees >=  67.5 && angleInDegrees <= 112.5) return Direction::South;
    if (angleInDegrees >= 112.5 && angleInDegrees <= 157.5) return Direction::Southwest;
    if (angleInDegrees >= 157.5 && angleInDegrees <= 202.5) return Direction::West;
    if (angleInDegrees >= 202.5 && angleInDegrees <= 247.5) return Direction::Northwest;
    if (angleInDegrees >= 247.5 && angleInDegrees <= 292.5) return Direction::North;
    if (angleInDegrees >= 292.5 && angleInDegrees <= 337.5) return Direction::Northeast;
    return Direction::Here;
}

double Coord::distanceTo(const Coord &to) const {
    double dx = to.x - x;
    double dy = to.y - y;
    return sqrt(dx*dx + dy*dy);
}

std::string Coord::toString() const {
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

std::ostream& operator<<(std::ostream &out, const Coord &where) {
    out << where.toString();
    return out;
}
