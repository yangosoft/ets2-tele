// WebsocketPP
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <set>
// Windows stuff.

/*#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

// SDK
#include "scssdk_telemetry.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_telemetry_eut2.h"
#include "amtrucks/scssdk_ats.h"
#include "amtrucks/scssdk_telemetry_ats.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using server = websocketpp::server<websocketpp::config::asio>;
using con_list = std::set<connection_hdl, std::owner_less<connection_hdl>>;

con_list clients;
std::mutex mtx_clients;
server print_server;


void on_open(connection_hdl hdl) {
	std::unique_lock<std::mutex> lock(mtx_clients);
	clients.insert(hdl);
}

void on_close(connection_hdl hdl) {
	clients.erase(hdl);
}

json json_telemetry;



const size_t MAX_SUPPORTED_WHEEL_COUNT = 8;


#pragma pack(push)
#pragma pack(1)

/**
 * @brief The layout of the shared memory.
*/
struct telemetry_state_t
{
	scs_u8_t				running;					// Is the telemetry running or it is paused?

	scs_value_dplacement_t	ws_truck_placement;			// SCS_TELEMETRY_TRUCK_CHANNEL_world_placement

	scs_float_t				speedometer_speed;			// SCS_TELEMETRY_TRUCK_CHANNEL_speed
	scs_float_t				rpm;						// SCS_TELEMETRY_TRUCK_CHANNEL_engine_rpm
	scs_s32_t				gear;						// SCS_TELEMETRY_TRUCK_CHANNEL_engine_gear

	scs_float_t				steering;					// SCS_TELEMETRY_TRUCK_CHANNEL_effective_steering
	scs_float_t				throttle;					// SCS_TELEMETRY_TRUCK_CHANNEL_effective_throttle
	scs_float_t				brake;						// SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake
	scs_float_t				clutch;						// SCS_TELEMETRY_TRUCK_CHANNEL_effective_clutch

	scs_value_fvector_t		linear_valocity;			// SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_velocity
	scs_value_fvector_t		angular_velocity;			// SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_velocity
	scs_value_fvector_t		linear_acceleration;		// SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_acceleration
	scs_value_fvector_t		angular_acceleration;		// SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_acceleration
	scs_value_fvector_t		cabin_angular_velocity;		// SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_velocity
	scs_value_fvector_t		cabin_angular_acceleration;	// SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_acceleration

	scs_u32_t				wheel_count;				// SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_count
	scs_float_t				wheel_deflections[MAX_SUPPORTED_WHEEL_COUNT]; // SCS_TELEMETRY_TRUCK_CHANNEL_wheel_susp_deflection

	scs_float_t oil_temp;
	scs_float_t oil_pressure;
	scs_float_t water_temp;
	scs_float_t odometer;
	scs_float_t fuel;
	scs_float_t fuel_average_consum;
};

#pragma pack(pop)

/**
 * @brief Handle of the memory mapping.
 */
HANDLE memory_mapping = nullptr;

/**
 * @brief Block inside the shared memory.
 */
telemetry_state_t* shared_memory = nullptr;

/**
 * @brief Deinitialize the shared memory objects.
 */
void deinitialize_shared_memory(void)
{
	if (shared_memory) {
		UnmapViewOfFile(shared_memory);
		shared_memory = nullptr;
	}

	if (memory_mapping) {
		CloseHandle(memory_mapping);
		memory_mapping = nullptr;
	}
}

/**
 * @brief Initialize the shared memory objects.
 */
bool initialize_shared_memory(void)
{
	// Setup the mapping.

	const DWORD memory_size = sizeof(telemetry_state_t);
	memory_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, memory_size, "SCSTelemetryExample");
	if (!memory_mapping) {
		//log_line(SCS_LOG_TYPE_error, "Unable to create shared memory %08X", GetLastError());
		std::cout << "Cannot initialize shared memory\n";
		//deinitialize_shared_memory();
		return false;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		//log_line(SCS_LOG_TYPE_error, "Shared memory is already in use.");
		//deinitialize_shared_memory();
		std::cout << "Already exists!\n";
		//return false;
	}

	shared_memory = static_cast<telemetry_state_t*>(MapViewOfFile(memory_mapping, FILE_MAP_READ, 0, 0, 0));
	if (!shared_memory) {
		//log_line(SCS_LOG_TYPE_error, "Unable to map the view %08X", GetLastError());
		std::cout << "unable to map!\n";
		return false;
	}


	return true;
}

void broadcast_telemetry(telemetry_state_t* shared_memory) {
	std::unique_lock<std::mutex> lock(mtx_clients);

	json_telemetry["speedometer_speed"] = shared_memory->speedometer_speed;
	json_telemetry["rpm"] = shared_memory->rpm;
	json_telemetry["gear"] = shared_memory->gear;
	json_telemetry["throttle"] = shared_memory->throttle;
	json_telemetry["brake"] = shared_memory->brake;
	json_telemetry["oil_temp"] = shared_memory->oil_temp;
	json_telemetry["oil_pressure"] = shared_memory->oil_pressure;
	json_telemetry["water_temp"] = shared_memory->water_temp;
	json_telemetry["odometer"] = shared_memory->odometer;
	json_telemetry["fuel"] = shared_memory->fuel;
	json_telemetry["fuel_average_consum"] = shared_memory->fuel_average_consum;

	std::string data(json_telemetry.dump());
	for (auto client : clients) {
		print_server.send(client, data, websocketpp::frame::opcode::text);
	}
}

int main()
{
	std::cout << "Hello World!\n";
	auto ret = initialize_shared_memory();
	if (!ret) {
		std::cout << "Cannot read from shared memory\n";
		return -1;
	}

	json_telemetry["speedometer_speed"] = 0.0f;
	json_telemetry["rpm"] = 0.0f;
	json_telemetry["gear"] = 0;
	json_telemetry["throttle"] = 0.0f;
	json_telemetry["brake"] = 0.0f;
	json_telemetry["oil_temp"] = 0.0f;
	json_telemetry["oil_pressure"] = 0.0f;
	json_telemetry["water_temp"] = 0.0f;
	json_telemetry["odometer"] = 0.0f;
	json_telemetry["fuel"] = 0.0f;
	json_telemetry["fuel_average_consum"] = 0.0f;



	print_server.set_open_handler(&on_open);
	print_server.set_close_handler(&on_close);

	//print_server.set_message_handler(&on_message);
	print_server.set_access_channels(websocketpp::log::alevel::all);
	print_server.set_error_channels(websocketpp::log::elevel::all);

	print_server.init_asio();
	print_server.listen(9002);
	print_server.start_accept();

	std::thread 		th_srv([&] { print_server.run();  });
	


	while (true) {
		
		std::cout << (shared_memory->speedometer_speed * 3600) / 1000 << " km/h\n";
		broadcast_telemetry(shared_memory);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		
	}

	th_srv.join();
}
