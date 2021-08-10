
#include <nlohmann/json.hpp>
#include <ropufu/benchmark.hpp>
#include <ropufu/noexcept_json.hpp>
#include <ropufu/random/normal_sampler_512.hpp>

#include <concepts>  // std::totally_ordered
#include <cstddef>   // std::size_t
#include <fstream>   // std::ifstream
#include <ios>       // std::make_error_code, std::io_errc
#include <iostream>  // std::cout, std::endl
#include <random>    // std::mt19937_64
#include <stdexcept> // std::string
#include <string>    // std::string
#include <system_error> // std::error_code, std::errc
#include <vector>    // std::vector

#include "config.hpp"
#include "llr_transform.hpp"
#include "monte_carlo.hpp"
#include "process.hpp"
#include "stopping_time.hpp"
#include "stopping_time_factory.hpp"
#include "writer.hpp"

#include "processes/iid_process.hpp"
#include "stopping_times/cusum.hpp"
#include "stopping_times/finite_moving_average.hpp"
#include "stopping_times/sliding_cusum.hpp"

using config_type = ropufu::sequential::intermittent::config;

template <std::totally_ordered t_value_type>
using stopping_t = ropufu::sequential::intermittent::stopping_time<t_value_type>;

template <std::totally_ordered t_value_type>
using process_t = ropufu::sequential::intermittent::process<t_value_type>;

template <std::totally_ordered t_value_type>
using transform_t = ropufu::sequential::intermittent::llr_transform<t_value_type>;

config_type read_config(const std::string& file_name, std::error_code& ec) noexcept
{
	std::filesystem::path subfolder = ".";
	std::filesystem::path path = subfolder / file_name;
	config_type result {};
	
	try
	{
		std::ifstream config_stream(path);
		nlohmann::json j {};
		config_stream >> j;
		if (!ropufu::noexcept_json::try_get(j, result)) ec = std::make_error_code(std::errc::bad_message);
	}
	catch (const std::exception&)
	{
		ec = std::make_error_code(std::io_errc::stream);
	} // catch (...)

	return result;
} // read_config(...)

void clean_up(const std::string& file_name, std::error_code& ec) noexcept
{
	std::filesystem::path subfolder = ".";
	std::filesystem::path path = subfolder / file_name;
	std::filesystem::remove(path, ec);
} // clean_up(...)

template <std::totally_ordered t_value_type, typename t_rule_collection_type>
void run_length(const std::string& file_name,
	std::size_t count_simulations,
	process_t<t_value_type>& proc,
	const transform_t<t_value_type>& transform,
	t_rule_collection_type& rules)
{
	using monte_carlo_type = ropufu::sequential::intermittent::monte_carlo<t_value_type>;
	using rule_type = stopping_t<t_value_type>;
	using matrix_type = ropufu::aftermath::algebra::matrix<std::int32_t>;

	ropufu::benchmark timer{};

	std::cout << "Simulating stopping times:" << std::endl;
	for (const rule_type* r : rules) std::cout << (*r) << std::endl;
	std::cout << std::endl;

	timer.tic();
	monte_carlo_type mc {count_simulations};
	std::vector<t_value_type> max_average_run_length {};
	std::vector<matrix_type> pmfs = mc.run_length(proc, transform, rules, max_average_run_length);
	std::cout << "Simulation completed in " << timer.toc() << " seconds." << std::endl;
	std::cout << std::endl;

	std::cout << "Writing output..." << std::endl;
	timer.tic();
	std::size_t i = 0;
	for (rule_type* r : rules)
	{
		// matrix_type pmf = mc.run_length(proc, transform, *r);
		std::cout << "ARL(" << r->name() << ") = " << max_average_run_length[i] << std::endl;
		ropufu::sequential::intermittent::write_mat(file_name, *r, pmfs[i]);
		++i;
	} // for (...)
	std::cout << "Elapsed time: " << timer.toc() << " seconds." << std::endl;
	std::cout << std::endl;
} // run_length(...)

int main()
{
	static constexpr std::size_t count_simulations = 1'000;
	using engine_type = std::mt19937_64;
	using value_type = double;

	using sampler_type = ropufu::aftermath::random::normal_sampler_512<engine_type, value_type>;
	using distribution_type = typename sampler_type::distribution_type;
	using llr_transform_type = ropufu::sequential::intermittent::llr_transform<value_type>;
	using process_type = ropufu::sequential::intermittent::iid_process<engine_type, sampler_type>;

	using thresholds_type = typename stopping_t<value_type>::thresholds_type;

	using fma_type = ropufu::sequential::intermittent::finite_moving_average<value_type>;
	using cusum_type = ropufu::sequential::intermittent::cusum<value_type>;
	using sliding_cusum_type = ropufu::sequential::intermittent::sliding_cusum<value_type>;

	std::string config_file_name = "config.json";
	std::string mat_file_name = "simulator.mat";
	distribution_type dist{}; // Standard normal.
	llr_transform_type transform{1.0, -0.5}; // log(L) = (1.0) X - 0.5.
	process_type process {dist};

	ropufu::sequential::intermittent::stopping_time_factory<value_type> rule_factory {};


	std::error_code ec {};
	config_type config = read_config(config_file_name, ec);
	if (ec.value() != 0)
	{
		std::cout << "Reading configuration file failed." << std::endl;
		return 1;
	} // if (...)

	for (const nlohmann::json& j : config.stopping_times())
		if (!rule_factory.try_make(j))
		{
			std::cout << "Failed to parse stopping time." << std::endl;
			std::cout << j << std::endl;
			return 2;
		} // if (...)

	clean_up(mat_file_name, ec);
	if (ec.value() != 0)
	{
		std::cout << "Clean up failed." << std::endl;
		return 3;
	} // if (...)

	run_length(mat_file_name, count_simulations, process, transform, rule_factory);
	return 0;
} // main(...)
