
#include "morph.h"

World::World() 
: map(nullptr), currentTurn(0), disableFOV(false)
{ }
World::~World() {
    if (map) delete map;
}
void World::addMessage(const std::string &text) {
    messages.push_back(LogMessage{text});
}
void World::tick() {
    if (map) {
        map->tick(*this);
        ++currentTurn;
        map->doActorFOV(player);
    }
}
