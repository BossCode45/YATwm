#pragma once

#include <string>

typedef unsigned int ErrCode;

#define NOERR 0
#define ERR_NON_FATAL 110
#define ERR_FATAL 120
#define ERR_CFG_NON_FATAL 210
#define ERR_CFG_FATAL 220

struct Err
{
	ErrCode code;
	std::string errorMessage;
};
