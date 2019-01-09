#include "JM.h"

#include <iostream>
#include <string>



namespace jm
{

#define RESET  "\x1B[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

	void JM_EXPORT JMLog::PlatformLogMessage(uint level, const char* message)
	{
#ifndef JM_PLATFORM_LINUX
        std::cout << message;
#else
		switch (level)
		{
		case JM_LOG_LEVEL_FATAL:
			printf("%s", BOLDRED);
			break;
		case JM_LOG_LEVEL_ERROR:
			printf("%s", RED);
			break;
		case JM_LOG_LEVEL_WARN:
			printf("%s", YELLOW);
			break;
		case JM_LOG_LEVEL_INFO:
			printf("%s", CYAN);
			break;
		}

		printf("%s%s", message, RESET);
#endif
	}
}
