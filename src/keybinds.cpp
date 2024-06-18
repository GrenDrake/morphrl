#include <cstdint>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "physfs.h"
#include "BearLibTerminal.h"
#include "morph.h"



std::map<int, std::string> actionNames{
    { ACT_FULLQUIT,     "Quit to OS" },
    { ACT_NONE,         "None" },
    { ACT_MENU,         "Return to Menu" },
    { ACT_LOG,          "Message Log" },
    { ACT_CHARINFO,     "Character Info" },
    { ACT_INVENTORY,    "View Inventory" },
    { ACT_EXAMINETILE,  "Examine Map" },
    { ACT_CHANGEFLOOR,  "Ascend or Descend" },
    { ACT_MOVE,         "Move" },
    { ACT_WAIT,         "Wait" },
    { ACT_TAKEITEM,     "Take Item" },
    { ACT_REST,         "Rest Until Healed" },
    { ACT_INTERACTTILE, "Interact" },
    { ACT_USEABILITY,   "Use Ability" },

    { ACT_DBG_FULLHEAL, "Full Heal (DEBUG)" },
    { ACT_DBG_TELEPORT, "Teleport (DEBUG)" },
    { ACT_DBG_ADDITEM, "Add Item (DEBUG)" },
    { ACT_DBG_ADDMUTATION, "Add Mutation (DEBUG)" },
    { ACT_DBG_ADDSTATUS, "Add Status Effect (DEBUG)" },
    { ACT_DBG_DISABLEFOV, "Disable FOV (DEBUG)" },
    { ACT_DBG_KILLADJ, "Kill Adjacent Actors (DEBUG)" },
    { ACT_DBG_TUNNEL, "Make Tunnel (DEBUG)" },
    { ACT_DBG_MAPWACTORS, "Write Map and Actors to File  (DEBUG)" },
    { ACT_DBG_MAP, "Write Map to file (DEBUG)" },
    { ACT_DBG_XP, "Give XP (DEBUG)" },
};

std::map<int, std::string> keyNames{
    { 0,            "<NONE>" },
    { TK_A,         "A" },
    { TK_B,         "B" },
    { TK_C,         "C" },
    { TK_D,         "D" },
    { TK_E,         "E" },
    { TK_F,         "F" },
    { TK_G,         "G" },
    { TK_H,         "H" },
    { TK_I,         "I" },
    { TK_J,         "J" },
    { TK_K,         "K" },
    { TK_L,         "L" },
    { TK_M,         "M" },
    { TK_N,         "N" },
    { TK_O,         "O" },
    { TK_P,         "P" },
    { TK_Q,         "Q" },
    { TK_R,         "R" },
    { TK_S,         "S" },
    { TK_T,         "T" },
    { TK_U,         "U" },
    { TK_V,         "V" },
    { TK_W,         "W" },
    { TK_X,         "X" },
    { TK_Y,         "Y" },
    { TK_Z,         "Z" },
    { TK_PERIOD,    "." },
    { TK_COMMA,     "," },
    { TK_SPACE,     "<SPACE>" },
    { TK_LEFT,      "<LEFT>" },
    { TK_RIGHT,     "<RIGHT>" },
    { TK_UP,        "<UP>" },
    { TK_DOWN,      "<DOWN>" },
    { TK_ESCAPE,    "<ESCAPE>" },
    { TK_CLOSE,     "<CLOSE>" },
    { TK_F1,        "<F1>" },
    { TK_F2,        "<F2>" },
    { TK_F3,        "<F3>" },
    { TK_F4,        "<F4>" },
    { TK_F5,        "<F5>" },
    { TK_F6,        "<F6>" },
    { TK_F7,        "<F7>" },
    { TK_F8,        "<F8>" },
    { TK_F9,        "<F9>" },
    { TK_F10,       "<F10>" },
    { TK_F11,       "<F11>" },
    { TK_F12,       "<F12>" },
};


std::vector<KeyBinding> keyBindings{
    {   { TK_CLOSE },               ACT_FULLQUIT,        Direction::Unknown, MODE_ALL },

    {   { TK_ESCAPE },              ACT_MENU,           Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_L },                   ACT_LOG,            Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_C },                   ACT_CHARINFO,       Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_I },                   ACT_INVENTORY,      Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_X },                   ACT_EXAMINETILE,    Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_COMMA, TK_PERIOD },    ACT_CHANGEFLOOR,    Direction::Unknown, MODE_NORMAL },
    {   { TK_LEFT },                ACT_MOVE,           Direction::West,    MODE_NORMAL },
    {   { TK_RIGHT },               ACT_MOVE,           Direction::East,    MODE_NORMAL },
    {   { TK_UP },                  ACT_MOVE,           Direction::North,   MODE_NORMAL },
    {   { TK_DOWN },                ACT_MOVE,           Direction::South,   MODE_NORMAL },
    {   { TK_SPACE },               ACT_WAIT,           Direction::Unknown, MODE_NORMAL },
    {   { TK_T, TK_G },             ACT_TAKEITEM,       Direction::Unknown, MODE_NORMAL },
    {   { TK_R },                   ACT_REST,           Direction::Unknown, MODE_NORMAL },
    {   { TK_O },                   ACT_INTERACTTILE,   Direction::Unknown, MODE_NORMAL },
    {   { TK_A },                   ACT_USEABILITY,     Direction::Unknown, MODE_NORMAL },

#ifdef DEBUG
    {   { TK_F1 },                  ACT_DBG_FULLHEAL,   Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_F2 },                  ACT_DBG_TELEPORT,   Direction::Unknown, MODE_NORMAL },
    {   { TK_F3 },                  ACT_DBG_ADDITEM,    Direction::Unknown, MODE_NORMAL },
    {   { TK_F4 },                  ACT_DBG_ADDMUTATION,Direction::Unknown, MODE_NORMAL },
    {   { TK_F5 },                  ACT_DBG_ADDSTATUS,  Direction::Unknown, MODE_NORMAL },
    {   { TK_F6 },                  ACT_DBG_DISABLEFOV, Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_F7 },                  ACT_DBG_KILLADJ,    Direction::Unknown, MODE_NORMAL },
    {   { TK_F8 },                  ACT_DBG_TUNNEL,     Direction::Unknown, MODE_NORMAL },
    {   { TK_F10 },                 ACT_DBG_MAPWACTORS, Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_F11 },                 ACT_DBG_MAP,        Direction::Unknown, MODE_DEAD|MODE_NORMAL },
    {   { TK_F12 },                 ACT_DBG_XP,         Direction::Unknown, MODE_NORMAL },
#endif
};


const std::string STR_UNKNOWN_ACTION = "Unknown Action";
const KeyBinding NO_KEY{ 0, ACT_NONE };


const KeyBinding& getBindingForKey(int keyPressed, unsigned currentMode) {
    if (keyPressed == 0) return NO_KEY;

    for (const KeyBinding &binding : keyBindings) {
        if ((binding.forMode & currentMode) != currentMode) continue;
        if (binding.key[0] == keyPressed) return binding;
        if (binding.key[1] == keyPressed) return binding;
        if (binding.key[2] == keyPressed) return binding;
    }
    return NO_KEY;
}

const std::string& getNameForAction(int action) {
    auto iter = actionNames.find(action);
    if (iter == actionNames.end()) return STR_UNKNOWN_ACTION;
    return iter->second;
}

const std::string& getNameForKey(int key) {
    static std::string unknownKeyStr;
    auto iter = keyNames.find(key);
    if (iter == keyNames.end()) {
        unknownKeyStr = "<" + std::to_string(key) + ">";
        return unknownKeyStr;
    }
    return iter->second;
}

