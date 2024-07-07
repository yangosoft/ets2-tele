// ets2tele.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>
#include <thread>
#include <chrono>
// Windows stuff.

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>
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
};

#pragma pack(pop)

/**
 * @brief Handle of the memory mapping.
 */
HANDLE memory_mapping = NULL;

/**
 * @brief Block inside the shared memory.
 */
telemetry_state_t* shared_memory = NULL;

/**
 * @brief Deinitialize the shared memory objects.
 */
void deinitialize_shared_memory(void)
{
	if (shared_memory) {
		UnmapViewOfFile(shared_memory);
		shared_memory = NULL;
	}

	if (memory_mapping) {
		CloseHandle(memory_mapping);
		memory_mapping = NULL;
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

int main()
{
    std::cout << "Hello World!\n";
	auto ret = initialize_shared_memory();
	if (!ret) {
		std::cout << "Cannot read from shared memory\n";
		return -1;
	}


	while (true) {
		std::cout << (shared_memory->speedometer_speed * 3600)/1000 << "km/h\n";
		
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

	}
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln
