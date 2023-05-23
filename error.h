#pragma once

#include <string>

typedef unsigned int ErrCode;

#define NOERR 0
#define ERR_NON_FATAL 110
#define ERR_FATAL 120
#define CFG_ERR_NON_FATAL 210
#define CFG_ERR_KEYBIND 211
#define CFG_ERR_FATAL 220
#define CMD_ERR_NON_FATAL 310
#define CMD_ERR_NOT_FOUND 311
#define CMD_ERR_WRONG_ARGS 312
#define CMD_ERR_FATAL 320
struct Err
{
	ErrCode code;
	std::string errorMessage;
};
