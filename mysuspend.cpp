// mysuspend.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "PsList.h"

constexpr int SLEEP_TIMEOUT = 1 * 1000;
constexpr int SUSPEND_TIMEOUT = 45 * 1000;

enum class option
{
	NONE = 0,
	HELP = 1, // print help infomation
	SLEEP_TIMEOUT = 2, // set timeout value to sleep between process enumeration
	SUSPEND_TIMEOUT = 3, // set timeout value before program suspend 
	OUTPUT = 4, // set output file name
};

enum class magic
{
	SAVED = 1, // String stream saved
};

static std::map<option, details> opt_list = {
	{option::HELP,            {"--help",    "Print help information"}},
	{option::SLEEP_TIMEOUT,   {"--sleep",   "Set timeout value (ms) to sleep between process enumeration" }},
	{option::SUSPEND_TIMEOUT, {"--suspend", "Set timeout value (ms) before program suspend"}},
	{option::OUTPUT,          {"--output",  "Set output file name"}},
};

void MAGIC(magic n, int* cpuInfo)
{
	__cpuid(cpuInfo, 0x4711 | (unsigned)(n) << 16);
}

void help()
{
	std::cout << "pssuspend OPTIONS appname" << std::endl
		<< std::endl
		<< "wait for application and suspend it" << std::endl
		<< std::endl
		<< "OPTIONS" << std::endl << std::endl;
	for (auto i = opt_list.begin(); i != opt_list.end(); ++i)
	{
		details cmd = std::get<1>(*i);
		std::cout << "  " << std::get<0>(cmd) << std::endl << "     "
			<< std::get<1>(cmd) << "" << std::endl << std::endl;
	}
}

int main(int argc, char** argv)
{
	PsList psl;

	std::string psName;
	std::string filename;
	uint32_t sleep_timeout = SLEEP_TIMEOUT;
	uint32_t suspend_timeout = SUSPEND_TIMEOUT;

	option opt = option::NONE;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg(argv[i]);
		if (arg == std::get<0>(opt_list[option::SLEEP_TIMEOUT]))
		{
			opt = option::SLEEP_TIMEOUT;
		}
		else
			if (arg == std::get<0>(opt_list[option::SUSPEND_TIMEOUT]))
			{
				opt = option::SUSPEND_TIMEOUT;
			}
			else
				if (arg == std::get<0>(opt_list[option::OUTPUT]))
				{
					opt = option::OUTPUT;
				}
				else

					if (arg == std::get<0>(opt_list[option::HELP]))
					{
						help();
						return 0;
					}
					else
					{
						switch (opt)
						{
						case option::SLEEP_TIMEOUT:
							sleep_timeout = stoi(arg); break;

						case option::SUSPEND_TIMEOUT:
							suspend_timeout = stoi(arg); break;

						case option::OUTPUT:
							filename = arg; break;

						default:
							psName = arg; break;
						}
						opt = option::NONE;
					}
	}

	if (psName.empty())
	{
		std::cerr << "ERROR: Missing argument \"appname\"!" << std::endl;
		return 1;
	}
	std::cout << "Waiting for \"" + psName << "\"" << std::endl;
	bool running = true;
	while (running)
	{
		int pid = psl.find(psName);
		if (pid != -1)
		{
			running = false;
			std::cout << "Wait for the timer gone " << suspend_timeout
				<< " ms to suspend " << psName << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(suspend_timeout));
			DebugActiveProcess(pid);
			std::cout << "\"" << psName << "\" suspended" << std::endl;

			auto start = Time::now();

			psl.lsTree(true);

			auto stop = Time::now();

			fsec lsTime = stop - start;

			std::stringstream ss;

			start = Time::now();
			ss << psl;
			stop = Time::now();

			fsec plsTime = stop - start;
			stop = start = Time::now();

			if (filename.empty())
			{
				std::cout << ss.str() << std::endl;
			}
			else
			{
				std::ofstream fs;
				start = Time::now();
				fs.open(filename);
				fs << ss.str() << std::endl;
				fs.close();
				stop = Time::now();
				std::cout << "Saved" << std::endl;
			}
			fsec saveTime = stop - start;
			std::cerr << "lsTree duration: " << lsTime.count() << "s\n";
			std::cerr << "lsSymbols duration: " << plsTime.count() << "s\n";
			std::cerr << "Saving duration: " << saveTime.count() << "s\n";

			std::cout << "\"" << psName << "\" resumed" << std::endl;
			DebugActiveProcessStop(pid);
			return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_timeout));
	}

	return 0;


}

