#include <boost/bind.hpp>
#include <iostream>
#include <map>

#include "asserts.hpp"
#include "foreach.hpp"
#include "unit_test.hpp"

#include "SDL.h"

namespace test {

namespace {
typedef std::map<std::string, UnitTest> TestMap;
TestMap& get_test_map()
{
	static TestMap map;
	return map;
}

typedef std::map<std::string, BenchmarkTest> BenchmarkMap;
BenchmarkMap& get_benchmark_map()
{
	static BenchmarkMap map;
	return map;
}

typedef std::map<std::string, CommandLineBenchmarkTest> CommandLineBenchmarkMap;
CommandLineBenchmarkMap& get_cl_benchmark_map()
{
	static CommandLineBenchmarkMap map;
	return map;
}

typedef std::map<std::string, UtilityProgram> UtilityMap;
UtilityMap& get_utility_map()
{
	static UtilityMap map;
	return map;
}

}

int register_test(const std::string& name, UnitTest test)
{
	get_test_map()[name] = test;
	return 0;
}

int register_utility(const std::string& name, UtilityProgram utility)
{
	get_utility_map()[name] = utility;
	return 0;
}

bool run_tests(const std::vector<std::string>* tests)
{
	std::vector<std::string> all_tests;
	if(!tests) {
		for(TestMap::const_iterator i = get_test_map().begin(); i != get_test_map().end(); ++i) {
			all_tests.push_back(i->first);
		}

		tests = &all_tests;
	}

	int npass = 0, nfail = 0;
	foreach(const std::string& test, *tests) {
		try {
			get_test_map()[test]();
			std::cerr << "TEST " << test << " PASSED\n";
			++npass;
		} catch(failure_exception&) {
			std::cerr << "TEST " << test << " FAILED!!\n";
			++nfail;
		}
	}

	if(nfail) {
		std::cerr << npass << " TESTS PASSED, " << nfail << " TESTS FAILED\n";
		return false;
	} else {
		std::cerr << "ALL " << npass << " TESTS PASSED\n";
		return true;
	}
}

int register_benchmark(const std::string& name, BenchmarkTest test)
{
	get_benchmark_map()[name] = test;
	return 0;
}

int register_benchmark_cl(const std::string& name, CommandLineBenchmarkTest test)
{
	get_cl_benchmark_map()[name] = test;
	return 0;
}

namespace {
void run_benchmark(const std::string& name, BenchmarkTest fn)
{
	//run it once without counting it to let any initialization code be run.
	fn(1);

	std::cerr << "RUNNING BENCHMARK " << name << "...\n";
	const int MinTicks = 2000;
	for(int64_t nruns = 10; ; nruns *= 10) {
		const int start_time = SDL_GetTicks();
		fn(nruns);
		const int64_t time_taken_ms = SDL_GetTicks() - start_time;
		if(time_taken_ms >= MinTicks || nruns > 1000000000) {
			int64_t time_taken = time_taken_ms*1000000LL;
			int time_taken_units = 0;
			int64_t time_taken_per_iter = time_taken/nruns;
			int time_taken_per_iter_units = 0;
			while(time_taken > 10000 && time_taken_units < 3) {
				time_taken /= 1000;
				time_taken_units++;
			}

			while(time_taken_per_iter > 10000 && time_taken_per_iter_units < 3) {
				time_taken_per_iter /= 1000;
				time_taken_per_iter_units++;
			}

			const char* units[] = {"ns", "us", "ms", "s"};
			std::cerr << "BENCH " << name << ": " << nruns << " iterations, " << time_taken_per_iter << units[time_taken_per_iter_units] << "/iteration; total, " << time_taken << units[time_taken_units] << "\n";
			return;
		}
	}
}
}

void run_benchmarks(const std::vector<std::string>* benchmarks)
{
	std::vector<std::string> all_benchmarks;
	if(!benchmarks) {
		for(BenchmarkMap::const_iterator i = get_benchmark_map().begin(); i != get_benchmark_map().end(); ++i) {
			all_benchmarks.push_back(i->first);
		}

		benchmarks = &all_benchmarks;
	}

	foreach(const std::string& benchmark, *benchmarks) {
		std::string::const_iterator colon = std::find(benchmark.begin(), benchmark.end(), ':');
		if(colon != benchmark.end()) {
			//this benchmark has a user-supplied argument
			const std::string bench_name(benchmark.begin(), colon);
			const std::string arg(colon+1, benchmark.end());
			run_command_line_benchmark(bench_name, arg);
		} else {
			run_benchmark(benchmark, get_benchmark_map()[benchmark]);
		}
	}
}

void run_command_line_benchmark(const std::string& benchmark_name, const std::string& arg)
{
	run_benchmark(benchmark_name, boost::bind(get_cl_benchmark_map()[benchmark_name], _1, arg));
}

void run_utility(const std::string& utility_name, const std::vector<std::string>& arg)
{
	UtilityProgram util = get_utility_map()[utility_name];
	ASSERT_LOG(util, "Unknown utility: '" << utility_name << "'");
	util(arg);
}

}
