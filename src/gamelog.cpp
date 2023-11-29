#include <iostream>
#include <string>
#include "physfs.h"
#include "morph.h"


static PHYSFS_file *logFile = nullptr;

std::string logLevelName(int logLevel);


void logMessage(int logLevel, std::string message) {
    if (logFile == nullptr) {
        logFile = PHYSFS_openWrite("game.log");
        if (!logFile) {
            std::cerr << "Failed to open logfile!\n";
            return;
        }
    }
    message = logLevelName(logLevel) + "  " + message + "\n";
    PHYSFS_writeBytes(logFile, message.c_str(), message.size());
}

std::string logLevelName(int logLevel) {
    switch (logLevel) {
        case LOG_INFO: return "INFO";
        case LOG_WARN: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}